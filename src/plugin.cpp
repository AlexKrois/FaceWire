#include <obs-module.h>
#include <graphics/vec4.h>
#include <util/platform.h>
#include "detector.hpp"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("camoutlines", "en-US")
MODULE_EXPORT const char *obs_module_description(void) { return "Face landmark outlines for camera sources"; }
MODULE_EXPORT const char *obs_module_name(void) { return "Cam Outlines"; }
MODULE_EXPORT const char *obs_module_author(void) { return "Cam Outlines contributors"; }

namespace {
constexpr uint64_t expiry_ns = 350000000;
const char *part_keys[] = {"eyes", "brows", "nose", "mouth", "jaw", "oval", "iris", "mesh"};
struct Settings {
    uint32_t parts = 31, color = 0xff60ff40;
    float thickness = 2.5f, smoothing = 0.35f;
    float mesh_thickness = 1.0f, mesh_opacity = 0.35f;
    int fps = 30, resolution = 640;
};
struct Job {
    std::vector<uint8_t> rgba;
    uint32_t width = 0, height = 0, source_width = 0, source_height = 0;
    uint64_t time = 0;
    float smoothing = 0;
};
struct Result {
    cam::Landmarks points{};
    bool found = false;
    uint64_t time = 0;
    uint32_t width = 0, height = 0;
};
struct Filter {
    obs_source_t *source;
    std::mutex mutex;
    std::condition_variable wake;
    Settings settings;
    Job job;
    Result result;
    bool stop = false, pending = false, busy = false;
    std::atomic<bool> ready{false};
    std::string status = "Starting";
    std::thread worker;
    gs_texrender_t *frame = nullptr, *scaled = nullptr;
    gs_stagesurf_t *stage = nullptr;
    uint32_t stage_width = 0, stage_height = 0;
    uint64_t last_capture = 0;
    bool rendered = false;
    uint32_t rendered_width = 0, rendered_height = 0;

    explicit Filter(obs_source_t *s) : source(s) {}
    ~Filter() {
        { std::lock_guard lock(mutex); stop = true; }
        wake.notify_one();
        if (worker.joinable()) worker.join();
        obs_enter_graphics();
        gs_stagesurface_destroy(stage);
        gs_texrender_destroy(scaled);
        gs_texrender_destroy(frame);
        obs_leave_graphics();
    }
    void run(std::string runtime, std::string model) noexcept {
        try {
            cam::Detector detector(runtime, model);
            { std::lock_guard lock(mutex); status = "Ready"; }
            ready = true;
            Result previous;
            int64_t last_timestamp = -1;
            for (;;) {
                Job input;
                {
                    std::unique_lock lock(mutex);
                    wake.wait(lock, [&] { return stop || pending; });
                    if (stop) break;
                    input = std::move(job);
                    pending = false;
                }
                Result next;
                next.time = input.time;
                next.width = input.source_width;
                next.height = input.source_height;
                const auto timestamp = std::max(last_timestamp+1, static_cast<int64_t>(input.time/1000000));
                last_timestamp = timestamp;
                next.found = detector.detect(input.rgba.data(), input.width, input.height, timestamp, next.points);
                if (next.found && previous.found && next.time-previous.time < expiry_ns &&
                    next.width == previous.width && next.height == previous.height) {
                    // Reset smoothing on large jumps (reacquisition / a different face).
                    const auto a = next.points[1], b = previous.points[1];
                    if (std::hypot(a.x-b.x,a.y-b.y) < 0.15f) {
                        for (size_t i=0;i<next.points.size();++i) {
                            next.points[i].x += (previous.points[i].x-next.points[i].x)*input.smoothing;
                            next.points[i].y += (previous.points[i].y-next.points[i].y)*input.smoothing;
                        }
                    }
                }
                previous = next;
                { std::lock_guard lock(mutex); result = next; busy = false; }
            }
        } catch (const std::exception &e) {
            blog(LOG_ERROR, "[camoutlines] %s", e.what());
            std::lock_guard lock(mutex);
            status = std::string("Error: ") + e.what();
            result = {};
            busy = false;
        } catch (...) {
            blog(LOG_ERROR, "[camoutlines] Unexpected tracker failure");
            std::lock_guard lock(mutex);
            status = "Error";
            result = {};
            busy = false;
        }
        ready = false;
    }
};

void update(void *data, obs_data_t *values)
{
    auto &s = *static_cast<Filter *>(data);
    Settings next;
    next.parts = 0;
    for (unsigned i=0;i<std::size(part_keys);++i) if (obs_data_get_bool(values,part_keys[i])) next.parts |= 1u<<i;
    next.color = static_cast<uint32_t>(obs_data_get_int(values,"color"));
    next.thickness = static_cast<float>(std::clamp(obs_data_get_double(values,"thickness"),1.0,15.0));
    next.smoothing = static_cast<float>(std::clamp(obs_data_get_double(values,"smoothing"),0.0,0.9));
    next.mesh_thickness = static_cast<float>(std::clamp(obs_data_get_double(values,"mesh_thickness"),0.5,4.0));
    next.mesh_opacity = static_cast<float>(std::clamp(obs_data_get_double(values,"mesh_opacity"),0.0,1.0));
    next.fps = static_cast<int>(std::clamp(obs_data_get_int(values,"fps"),5LL,60LL));
    next.resolution = static_cast<int>(std::clamp(obs_data_get_int(values,"resolution"),256LL,1280LL));
    std::lock_guard lock(s.mutex);
    s.settings = next;
}
void defaults(obs_data_t *values)
{
    for (unsigned i=0;i<std::size(part_keys);++i) obs_data_set_default_bool(values,part_keys[i],i<5);
    obs_data_set_default_double(values,"mesh_thickness",1.0);
    obs_data_set_default_double(values,"mesh_opacity",0.35);
    obs_data_set_default_int(values,"color",0xff60ff40);
    obs_data_set_default_double(values,"thickness",2.5);
    obs_data_set_default_double(values,"smoothing",0.35);
    obs_data_set_default_int(values,"fps",30);
    obs_data_set_default_int(values,"resolution",640);
}
obs_properties_t *properties(void *data)
{
    auto *props = obs_properties_create();
    for (const char *key : part_keys) obs_properties_add_bool(props,key,obs_module_text(key));
    obs_properties_add_color_alpha(props,"color",obs_module_text("Color"));
    obs_properties_add_float_slider(props,"thickness",obs_module_text("Thickness"),1,15,0.5);
    obs_properties_add_float_slider(props,"mesh_thickness",obs_module_text("MeshThickness"),0.5,4,0.5);
    obs_properties_add_float_slider(props,"mesh_opacity",obs_module_text("MeshOpacity"),0,1,0.05);
    obs_properties_add_float_slider(props,"smoothing",obs_module_text("Smoothing"),0,0.9,0.05);
    obs_properties_add_int_slider(props,"fps",obs_module_text("FPS"),5,60,1);
    auto *res = obs_properties_add_list(props,"resolution",obs_module_text("Resolution"),OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_INT);
    for (int size : {320,480,640,960,1280}) obs_property_list_add_int(res,std::to_string(size).c_str(),size);
    std::string status = "Starting";
    if (data) { auto &s=*static_cast<Filter *>(data); std::lock_guard lock(s.mutex); status=s.status; }
    obs_properties_add_text(props,"status",obs_module_text(status.c_str()),OBS_TEXT_INFO);
    obs_properties_add_text(props,"help",obs_module_text("Help"),OBS_TEXT_INFO);
    return props;
}
void *create(obs_data_t *values, obs_source_t *source)
{
    try {
        auto s = std::make_unique<Filter>(source);
        update(s.get(),values);
        char *runtime = obs_module_file("libmediapipe.dll");
        char *model = obs_module_file("face_landmarker.task");
        const std::string runtime_path = runtime ? runtime : "";
        const std::string model_path = model ? model : "";
        bfree(runtime); bfree(model);
        s->worker = std::thread(&Filter::run,s.get(),runtime_path,model_path);
        return s.release();
    } catch (const std::exception &e) {
        blog(LOG_ERROR,"[camoutlines] Cannot create filter: %s",e.what());
        return nullptr;
    }
}
void draw_texture(gs_texture_t *texture, uint32_t width, uint32_t height)
{
    auto *effect=obs_get_base_effect(OBS_EFFECT_DEFAULT);
    gs_effect_set_texture(gs_effect_get_param_by_name(effect,"image"),texture);
    while (gs_effect_loop(effect,"Draw")) gs_draw_sprite(texture,0,width,height);
}
bool capture(Filter &s, obs_source_t *target, uint32_t width, uint32_t height)
{
    if (!s.frame) s.frame=gs_texrender_create(GS_RGBA,GS_ZS_NONE);
    if (!s.frame) return false;
    gs_texrender_reset(s.frame);
    if (!gs_texrender_begin(s.frame,width,height)) return false;
    vec4 clear{};
    gs_clear(GS_CLEAR_COLOR,&clear,0,0);
    gs_ortho(0,static_cast<float>(width),0,static_cast<float>(height),-100,100);
    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE,GS_BLEND_ZERO);
    auto *parent=obs_filter_get_parent(s.source);
    const auto flags=obs_source_get_output_flags(parent);
    if (target==parent && !(flags & (OBS_SOURCE_CUSTOM_DRAW|OBS_SOURCE_ASYNC))) obs_source_default_render(target);
    else obs_source_video_render(target);
    gs_blend_state_pop();
    gs_texrender_end(s.frame);
    return true;
}
void submit(Filter &s, uint32_t width, uint32_t height, const Settings &settings, uint64_t now)
{
    if (!s.ready || now-s.last_capture < 1000000000ull/settings.fps) return;
    { std::lock_guard lock(s.mutex); if (s.busy) return; }
    s.last_capture=now;
    const float scale=std::min(1.0f,static_cast<float>(settings.resolution)/std::max(width,height));
    const uint32_t w=std::max(1u,static_cast<uint32_t>(width*scale));
    const uint32_t h=std::max(1u,static_cast<uint32_t>(height*scale));
    if (!s.scaled) s.scaled=gs_texrender_create(GS_RGBA,GS_ZS_NONE);
    if (!s.scaled) return;
    if (!s.stage || s.stage_width!=w || s.stage_height!=h) {
        gs_stagesurface_destroy(s.stage);
        s.stage=gs_stagesurface_create(w,h,GS_RGBA);
        s.stage_width=w; s.stage_height=h;
    }
    if (!s.stage) return;
    gs_texrender_reset(s.scaled);
    if (!gs_texrender_begin(s.scaled,w,h)) return;
    gs_ortho(0,static_cast<float>(w),0,static_cast<float>(h),-100,100);
    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE,GS_BLEND_ZERO);
    draw_texture(gs_texrender_get_texture(s.frame),w,h);
    gs_blend_state_pop();
    gs_texrender_end(s.scaled);
    gs_stage_texture(s.stage,gs_texrender_get_texture(s.scaled));
    Job next;
    next.rgba.resize(static_cast<size_t>(w)*h*4);
    uint8_t *pixels=nullptr;
    uint32_t stride=0;
    if (!gs_stagesurface_map(s.stage,&pixels,&stride)) return;
    for (uint32_t y=0;y<h;++y) std::memcpy(next.rgba.data()+static_cast<size_t>(y)*w*4,pixels+static_cast<size_t>(y)*stride,w*4);
    gs_stagesurface_unmap(s.stage);
    next.width=w; next.height=h; next.source_width=width; next.source_height=height;
    next.time=now; next.smoothing=settings.smoothing;
    { std::lock_guard lock(s.mutex); s.job=std::move(next); s.pending=true; s.busy=true; }
    s.wake.notify_one();
}
void render(void *data, gs_effect_t *)
{
    auto &s=*static_cast<Filter *>(data);
    auto *target=obs_filter_get_target(s.source);
    const auto width=obs_source_get_base_width(target), height=obs_source_get_base_height(target);
    if (!target || !width || !height) { obs_source_skip_video_filter(s.source); return; }
    Settings settings;
    Result result;
    { std::lock_guard lock(s.mutex); settings=s.settings; result=s.result; }
    if (!settings.parts) { obs_source_skip_video_filter(s.source); return; }
    if (!s.rendered || width!=s.rendered_width || height!=s.rendered_height) {
        if (!capture(s,target,width,height)) { obs_source_skip_video_filter(s.source); return; }
        s.rendered=true; s.rendered_width=width; s.rendered_height=height;
        try { submit(s,width,height,settings,os_gettime_ns()); }
        catch (const std::exception &e) { blog(LOG_ERROR,"[camoutlines] Capture failed: %s",e.what()); }
    }
    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE,GS_BLEND_INVSRCALPHA);
    draw_texture(gs_texrender_get_texture(s.frame),width,height);
    if (result.found && result.width==width && result.height==height && os_gettime_ns()-result.time < expiry_ns) {
        const auto draw_lines=[&](uint32_t parts,float thickness,float opacity) {
        if (!parts || opacity<=0) return;
        const auto vertices=cam::geometry(result.points,parts,static_cast<float>(width),static_cast<float>(height),thickness);
        if (!vertices.empty()) {
            auto *effect=obs_get_base_effect(OBS_EFFECT_SOLID);
            vec4 color;
            vec4_from_rgba(&color,settings.color);
            color.w*=opacity;
            // Premultiply to preserve source alpha through the filter chain.
            color.x*=color.w; color.y*=color.w; color.z*=color.w;
            gs_effect_set_vec4(gs_effect_get_param_by_name(effect,"color"),&color);
            while (gs_effect_loop(effect,"Solid")) {
                gs_render_start(true);
                for (const auto p:vertices) gs_vertex2f(p.x,p.y);
                gs_render_stop(GS_TRIS);
            }
        }
        };
        draw_lines(settings.parts & cam::Mesh,settings.mesh_thickness,settings.mesh_opacity);
        draw_lines(settings.parts & ~cam::Mesh,settings.thickness,1.0f);
    }
    gs_blend_state_pop();
}
}
bool obs_module_load(void)
{
    obs_source_info info{};
    info.id="camoutlines_filter";
    info.type=OBS_SOURCE_TYPE_FILTER;
    info.output_flags=OBS_SOURCE_VIDEO;
    info.get_name=[](void *) { return obs_module_text("FilterName"); };
    info.create=create;
    info.destroy=[](void *data) { delete static_cast<Filter *>(data); };
    info.update=update;
    info.get_defaults=defaults;
    info.get_properties=properties;
    info.video_tick=[](void *data,float) { static_cast<Filter *>(data)->rendered=false; };
    info.video_render=render;
    obs_register_source(&info);
    return true;
}
