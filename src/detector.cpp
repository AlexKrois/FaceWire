#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#define MP_EXPORT
#include <mediapipe/tasks/c/vision/face_landmarker/face_landmarker.h>
#include "detector.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace cam {
struct Detector::Impl {
    HMODULE library = nullptr;
    MpFaceLandmarkerPtr tracker = nullptr;
    std::vector<char> model;
    decltype(&MpFaceLandmarkerCreate) create = nullptr;
    decltype(&MpFaceLandmarkerClose) close = nullptr;
    decltype(&MpFaceLandmarkerDetectForVideo) video = nullptr;
    decltype(&MpFaceLandmarkerCloseResult) free_result = nullptr;
    decltype(&MpImageCreateFromUint8Data) image = nullptr;
    decltype(&MpImageFree) free_image = nullptr;
    decltype(&MpErrorFree) free_error = nullptr;

    template<typename T> void symbol(T &fn, const char *name) {
        fn = reinterpret_cast<T>(GetProcAddress(library, name));
        if (!fn) throw std::runtime_error(std::string("MediaPipe symbol missing: ")+name);
    }
    void check(MpStatus status, char *error) {
        std::string message = error ? error : "MediaPipe operation failed";
        if (error) free_error(error);
        if (status != kMpOk) throw std::runtime_error(message);
    }
    ~Impl() {
        if (tracker && close) close(tracker, nullptr);
        if (library) FreeLibrary(library);
    }
};
Detector::Detector(const std::string &runtime, const std::string &model) : impl(std::make_unique<Impl>())
{
    auto &p = *impl;
    if (runtime.empty() || !std::filesystem::is_regular_file(std::filesystem::u8path(runtime)))
        throw std::runtime_error("Missing libmediapipe.dll in the plugin data folder; check the installation layout");
    if (model.empty() || !std::filesystem::is_regular_file(std::filesystem::u8path(model)))
        throw std::runtime_error("Missing face_landmarker.task in the plugin data folder; check the installation layout");
    const auto runtime_path = std::filesystem::absolute(std::filesystem::u8path(runtime)).make_preferred();
    p.library = LoadLibraryExW(runtime_path.c_str(), nullptr,
                              LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!p.library) throw std::runtime_error("Cannot load MediaPipe DLL (Windows error " + std::to_string(GetLastError()) + ")");
    p.symbol(p.create, "MpFaceLandmarkerCreate");
    p.symbol(p.close, "MpFaceLandmarkerClose");
    p.symbol(p.video, "MpFaceLandmarkerDetectForVideo");
    p.symbol(p.free_result, "MpFaceLandmarkerCloseResult");
    p.symbol(p.image, "MpImageCreateFromUint8Data");
    p.symbol(p.free_image, "MpImageFree");
    p.symbol(p.free_error, "MpErrorFree");
    std::ifstream file(std::filesystem::u8path(model), std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open face_landmarker.task");
    p.model.assign(std::istreambuf_iterator<char>(file), {});
    if (p.model.empty()) throw std::runtime_error("Face model is empty");
    FaceLandmarkerOptions options{};
    options.base_options.model_asset_buffer = p.model.data();
    options.base_options.model_asset_buffer_count = static_cast<unsigned>(p.model.size());
    options.running_mode = VIDEO;
    options.num_faces = 1;
    char *error = nullptr;
    const auto status = p.create(&options, &p.tracker, &error);
    p.check(status, error);
}
Detector::~Detector() = default;
bool Detector::detect(const uint8_t *rgba, int width, int height, int64_t timestamp, Landmarks &out)
{
    auto &p = *impl;
    MpImagePtr image = nullptr;
    char *error = nullptr;
    auto status = p.image(kMpImageFormatSrgba, width, height, rgba, width*height*4, &image, &error);
    p.check(status, error);
    FaceLandmarkerResult result{};
    error = nullptr;
    status = p.video(p.tracker, image, nullptr, timestamp, &result, &error);
    p.free_image(image);
    if (status != kMpOk) {
        p.free_result(&result);
        p.check(status, error);
    }
    if (error) p.free_error(error);
    bool found = result.face_landmarks_count > 0 && result.face_landmarks[0].landmarks_count >= out.size();
    if (found) {
        for (size_t i=0; i<out.size(); ++i) {
            const auto &point = result.face_landmarks[0].landmarks[i];
            out[i] = {point.x,point.y};
            if (!std::isfinite(point.x) || !std::isfinite(point.y)) found = false;
        }
    }
    p.free_result(&result);
    return found;
}
}
