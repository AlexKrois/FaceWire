#pragma once
#include "landmarks.hpp"
#include <memory>
#include <string>

namespace cam {
class Detector {
public:
    Detector(const std::string &runtime, const std::string &model);
    ~Detector();
    Detector(const Detector &) = delete;
    Detector &operator=(const Detector &) = delete;
    bool detect(const uint8_t *rgba, int width, int height, int64_t timestamp, Landmarks &out);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
