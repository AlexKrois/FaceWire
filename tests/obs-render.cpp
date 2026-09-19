// Headless integration test using the installed libobs and its D3D11 renderer.
#include <obs.h>
#include <graphics/vec4.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
struct Fixture {
    uint32_t width=0,height=0;
    gs_texture_t *portrait=nullptr,*black=nullptr;
    std::atomic<bool> blank{false};
} fixture;
struct Frames {
    std::mutex mutex;
    std::condition_variable wake;
    std::vector<uint8_t> pixels;
    int count=0,green=0;
} frames;
void receive(void *,video_data *frame) {
    std::lock_guard lock(frames.mutex);
    frames.pixels.resize(size_t(fixture.width)*fixture.height*4);
    frames.green=0;
    for (uint32_t y=0;y<fixture.height;++y) {
        auto *row=frame->data[0]+size_t(y)*frame->linesize[0];
        std::memcpy(frames.pixels.data()+size_t(y)*fixture.width*4,row,fixture.width*4);
        for (uint32_t x=0;x<fixture.width;++x) {
            const auto *p=row+x*4;
            if (p[1]>200 && p[0]<40 && p[2]<40) ++frames.green;
        }
    }
    ++frames.count;
    frames.wake.notify_all();
}
void expect_green(bool visible) {
    std::unique_lock lock(frames.mutex);
    const auto after=frames.count+10;
    if (!frames.wake.wait_for(lock,std::chrono::seconds(8),[&] {
        return frames.count>=after && (visible ? frames.green>30 : frames.green==0);
    })) throw std::runtime_error(visible ? "No overlay in OBS output" : "Overlay did not disappear");
    std::cout<<"OBS frames="<<frames.count<<", green pixels="<<frames.green<<'\n';
}
void save_frame(const std::string &path) {
    std::lock_guard lock(frames.mutex);
    std::ofstream image(path,std::ios::binary);
    image<<"P6\n"<<fixture.width<<' '<<fixture.height<<"\n255\n";
    for (size_t i=0;i<frames.pixels.size();i+=4) image.write(reinterpret_cast<char *>(frames.pixels.data()+i),3);
}
int main(int argc,char **argv) {
    if (argc!=6) return 2; // OBS root, plugin DLL, plugin data, RGBA fixture, output PPM
    try {
        std::ifstream file(argv[4],std::ios::binary);
        file.read(reinterpret_cast<char *>(&fixture.width),4);
        file.read(reinterpret_cast<char *>(&fixture.height),4);
        if (!file || !fixture.width || !fixture.height || fixture.width>4096 || fixture.height>4096) throw std::runtime_error("Invalid fixture");
        std::vector<uint8_t> pixels(size_t(fixture.width)*fixture.height*4);
        file.read(reinterpret_cast<char *>(pixels.data()),pixels.size());
        if (!file) throw std::runtime_error("Truncated fixture");
        if (!obs_startup("de-DE",nullptr,nullptr)) throw std::runtime_error("OBS startup failed");
        const std::string data_path=std::string(argv[1])+"/data/libobs/";
        obs_add_data_path(data_path.c_str());
        const std::string graphics=std::string(argv[1])+"/bin/64bit/libobs-d3d11.dll";
        obs_video_info video{};
        video.graphics_module=graphics.c_str();
        video.fps_num=30;video.fps_den=1;
        video.base_width=video.output_width=fixture.width;
        video.base_height=video.output_height=fixture.height;
        video.output_format=VIDEO_FORMAT_BGRA;
        video.colorspace=VIDEO_CS_709;video.range=VIDEO_RANGE_FULL;
        video.scale_type=OBS_SCALE_BILINEAR;
        if (obs_reset_video(&video)!=OBS_VIDEO_SUCCESS) throw std::runtime_error("OBS D3D11 initialization failed");
        obs_module_t *module=nullptr;
        if (obs_open_module(&module,argv[2],argv[3])!=MODULE_SUCCESS || !obs_init_module(module)) throw std::runtime_error("Cannot load Cam Outlines into OBS");
        const char *display_name=obs_source_get_display_name("camoutlines_filter");
        if (!display_name || !std::strstr(display_name,"Gesichtslandmarks"))
            throw std::runtime_error("German locale missing from plugin data directory");
        obs_enter_graphics();
        const uint8_t *portrait=pixels.data();
        fixture.portrait=gs_texture_create(fixture.width,fixture.height,GS_RGBA,1,&portrait,0);
        std::fill(pixels.begin(),pixels.end(),0);
        for (size_t i=3;i<pixels.size();i+=4) pixels[i]=255;
        const uint8_t *black=pixels.data();
        fixture.black=gs_texture_create(fixture.width,fixture.height,GS_RGBA,1,&black,0);
        obs_leave_graphics();
        obs_source_info info{};
        info.id="cam_test_image";info.type=OBS_SOURCE_TYPE_INPUT;
        info.output_flags=OBS_SOURCE_VIDEO|OBS_SOURCE_CUSTOM_DRAW;
        info.get_name=[](void *){return "Test portrait";};
        info.create=[](obs_data_t *,obs_source_t *)->void * {return &fixture;};
        info.destroy=[](void *){};
        info.get_width=[](void *){return fixture.width;};
        info.get_height=[](void *){return fixture.height;};
        info.video_render=[](void *,gs_effect_t *) {
            auto *texture=fixture.blank ? fixture.black : fixture.portrait;
            auto *effect=obs_get_base_effect(OBS_EFFECT_DEFAULT);
            gs_effect_set_texture(gs_effect_get_param_by_name(effect,"image"),texture);
            while (gs_effect_loop(effect,"Draw")) gs_draw_sprite(texture,0,0,0);
        };
        obs_register_source(&info);
        auto *source=obs_source_create("cam_test_image","Portrait",nullptr,nullptr);
        auto *settings=obs_data_create();
        obs_data_set_int(settings,"color",0xff00ff00);
        auto *filter=obs_source_create("camoutlines_filter","Contours",settings,nullptr);
        if (!source || !filter) throw std::runtime_error("OBS source creation failed");
        obs_source_filter_add(source,filter);
        obs_set_output_source(0,source);
        video_scale_info conversion{};
        conversion.format=VIDEO_FORMAT_RGBA;
        conversion.width=fixture.width;conversion.height=fixture.height;
        conversion.colorspace=VIDEO_CS_709;conversion.range=VIDEO_RANGE_FULL;
        obs_add_raw_video_callback(&conversion,receive,nullptr);
        expect_green(true);
        {
            std::lock_guard lock(frames.mutex);
            std::ofstream image(argv[5],std::ios::binary);
            image<<"P6\n"<<fixture.width<<' '<<fixture.height<<"\n255\n";
            for (size_t i=0;i<frames.pixels.size();i+=4) image.write(reinterpret_cast<char *>(frames.pixels.data()+i),3);
        }
        const char *parts[]={"eyes","brows","nose","mouth","jaw","oval","iris","mesh"};
        obs_data_set_double(settings,"mesh_opacity",1.0);
        obs_data_set_double(settings,"mesh_thickness",1.5);
        for (const auto *part:parts) obs_data_set_bool(settings,part,false);
        obs_source_update(filter,settings);
        expect_green(false);
        for (const auto *part:parts) {
            for (const auto *key:parts) obs_data_set_bool(settings,key,key==part);
            obs_source_update(filter,settings);
            expect_green(true);
            if (std::strcmp(part,"oval")==0 || std::strcmp(part,"iris")==0 || std::strcmp(part,"mesh")==0)
                save_frame(std::string(argv[5])+"."+part+".ppm");
        }
        obs_data_set_double(settings,"mesh_opacity",0.0);
        obs_source_update(filter,settings);
        expect_green(false);
        obs_data_set_double(settings,"mesh_opacity",1.0);
        obs_source_update(filter,settings);
        expect_green(true);
        fixture.blank=true;
        expect_green(false);
        fixture.blank=false;
        expect_green(true);
        obs_source_set_enabled(filter,false);
        expect_green(false);
        obs_source_set_enabled(filter,true);
        expect_green(true);
        obs_remove_raw_video_callback(receive,nullptr);
        obs_set_output_source(0,nullptr);
        obs_source_filter_remove(source,filter);
        obs_source_release(filter);
        obs_source_release(source);
        obs_data_release(settings);
        obs_enter_graphics();
        gs_texture_destroy(fixture.portrait);gs_texture_destroy(fixture.black);
        obs_leave_graphics();
        obs_shutdown();
        std::cout<<"OBS integration: eight regions, mesh opacity, face loss, reacquisition and disable/enable passed\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
