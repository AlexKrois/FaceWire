#include "landmarks.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
void require(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
int main() {
    try {
        std::vector<cam::Point> mesh;
        cam::segment(mesh,{0,0},{10,0},4);
        require(mesh.size()==6,"A segment needs two triangles");
        for (auto p:mesh) require(std::abs(p.y)==2,"Line must have the requested thickness");
        cam::segment(mesh,{0,0},{0,0},4);
        cam::segment(mesh,{0,0},{std::numeric_limits<float>::quiet_NaN(),0},4);
        require(mesh.size()==6,"Degenerate/invalid segments must not reach the GPU");
        cam::Landmarks points;
        for (size_t i=0;i<points.size();++i) points[i]={float(i)/478,0.5f};
        require(cam::geometry(points,0,1920,1080,3).empty(),"Disabling all parts must remove all lines");
        size_t total=0;
        for (uint32_t part=1;part<=16;part<<=1) {
            const auto vertices=cam::geometry(points,part,1920,1080,3);
            require(!vertices.empty(),"Every anatomical region must be drawable separately");
            total+=vertices.size();
        }
        require(total==cam::geometry(points,31,1920,1080,3).size(),"Region selection must be additive");
        require(cam::geometry(points,cam::Oval,1920,1080,3).size()==36*6,"Full outline must have 36 edges");
        require(cam::geometry(points,cam::Oval|cam::Jaw,1920,1080,3).size()==36*6,"Jaw and full outline must not draw shared edges twice");
        std::set<std::pair<int,int>> unique;
        for (auto edge:cam::mesh_edges) {
            require(edge.first>=0 && edge.first<edge.second && edge.second<468,"Invalid mesh edge");
            require(unique.insert(edge).second,"Repeated mesh edges cause excess opacity");
        }
        require(cam::geometry(points,cam::Mesh,1920,1080,1).size()==1322*6,"Mesh topology must draw all unique edges");
        for (int c:{468,473}) {
            points[c]={c==468?0.25f:0.75f,0.5f};
            points[c+1]={points[c].x+0.02f,0.5f};
            points[c+2]={points[c].x,0.52f};
            points[c+3]={points[c].x-0.02f,0.5f};
            points[c+4]={points[c].x,0.48f};
        }
        const auto rings=cam::geometry(points,cam::Iris,1000,1000,2);
        require(rings.size()==2*32*6,"Both iris rings must be closed, smooth contours");
        for (auto p:rings) {
            const float cx=p.x<500?250.0f:750.0f;
            const float radius=std::hypot(p.x-cx,p.y-500);
            require(radius>18.9f && radius<21.1f,"Iris contour radius or center is incorrect");
        }
        points[469]=points[471];
        require(cam::geometry(points,cam::Iris,1000,1000,2).size()==32*6,"Degenerate iris must not create invalid geometry");
        for (const auto &path:cam::paths()) for (int index:path.points) require(index>=0&&index<478,"Invalid model landmark index");
        std::cout<<"Geometry and region selection OK\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
