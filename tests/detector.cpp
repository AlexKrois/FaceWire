#include "detector.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char **argv) {
    if (argc<3) return 2;
    try {
        for (const auto *missing : {"", "nonexistent-mediapipe.dll"}) {
            bool rejected = false;
            try { cam::Detector invalid(missing,argv[2]); }
            catch (const std::exception &e) { rejected = std::string(e.what()).find("Missing libmediapipe.dll") != std::string::npos; }
            if (!rejected) throw std::runtime_error("Missing runtime did not produce an actionable installation error");
        }
        bool missing_model = false;
        try { cam::Detector invalid(argv[1],""); }
        catch (const std::exception &e) { missing_model = std::string(e.what()).find("Missing face_landmarker.task") != std::string::npos; }
        if (!missing_model) throw std::runtime_error("Missing model did not produce an actionable installation error");
        cam::Detector detector(argv[1],argv[2]);
        cam::Landmarks points;
        std::vector<uint8_t> black(256*256*4,0);
        for (size_t i=3;i<black.size();i+=4) black[i]=255;
        for (int i=0;i<3;++i) {
            if (detector.detect(black.data(),256,256,i*33,points)) throw std::runtime_error("False face on a blank frame");
        }
        if (argc==4) {
            std::ifstream input(argv[3],std::ios::binary);
            uint32_t dimensions[2]{};
            input.read(reinterpret_cast<char *>(dimensions),sizeof dimensions);
            if (!input || !dimensions[0] || !dimensions[1] || dimensions[0]>4096 || dimensions[1]>4096) throw std::runtime_error("Invalid RGBA fixture");
            std::vector<uint8_t> pixels(size_t(dimensions[0])*dimensions[1]*4);
            input.read(reinterpret_cast<char *>(pixels.data()),pixels.size());
            if (!input || !detector.detect(pixels.data(),dimensions[0],dimensions[1],100,points)) throw std::runtime_error("No face in portrait fixture");
            for (auto point:points) if (point.x<0 || point.x>1 || point.y<0 || point.y>1) throw std::runtime_error("Portrait landmark outside image");
            if (detector.detect(black.data(),256,256,133,points)) throw std::runtime_error("Face persisted after disappearing");
            std::cout<<"Portrait: 478 landmarks, then face loss OK\n";
        }
        std::cout<<"Native MediaPipe model initialization, video inference and cleanup OK\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
