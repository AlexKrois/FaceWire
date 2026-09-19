#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <set>
#include "mesh-topology.hpp"

namespace cam {
struct Point { float x = 0, y = 0; };
using Landmarks = std::array<Point, 478>;
enum Part : uint32_t { Eyes = 1, Brows = 2, Nose = 4, Mouth = 8, Jaw = 16, Oval = 32, Iris = 64, Mesh = 128 };
struct Path { uint32_t part; std::vector<int> points; };
// MediaPipe landmark indices; paths follow the anatomical contours.
inline const std::vector<Path> &paths()
{
    static const std::vector<Path> value = {
        {Eyes, {33,7,163,144,145,153,154,155,133,173,157,158,159,160,161,246,33}},
        {Eyes, {263,249,390,373,374,380,381,382,362,398,384,385,386,387,388,466,263}},
        {Brows, {70,63,105,66,107}}, {Brows, {46,53,52,65,55}},
        {Brows, {300,293,334,296,336}}, {Brows, {276,283,282,295,285}},
        {Nose, {168,6,197,195,5,4,1}}, {Nose, {98,97,2,326,327}},
        {Mouth, {61,146,91,181,84,17,314,405,321,375,291,409,270,269,267,0,37,39,40,185,61}},
        {Mouth, {78,95,88,178,87,14,317,402,318,324,308,415,310,311,312,13,82,81,80,191,78}},
        {Jaw, {234,93,132,58,172,136,150,149,176,148,152,377,400,378,379,365,397,288,361,323,454}},
        {Oval, {10,338,297,332,284,251,389,356,454,323,361,288,397,365,379,378,400,377,152,148,176,149,150,136,172,58,132,93,234,127,162,21,54,103,67,109,10}}
    };
    return value;
}
inline void segment(std::vector<Point> &out, Point a, Point b, float width)
{
    const float dx = b.x-a.x, dy = b.y-a.y, length = std::hypot(dx,dy);
    if (!std::isfinite(length) || length < 0.001f || width <= 0) return;
    const float nx = -dy / length * width * 0.5f, ny = dx / length * width * 0.5f;
    const Point p{a.x+nx,a.y+ny}, q{a.x-nx,a.y-ny};
    const Point r{b.x+nx,b.y+ny}, s{b.x-nx,b.y-ny};
    out.insert(out.end(), {p,q,r,q,s,r});
}
inline void iris_ring(std::vector<Point> &vertices, const Landmarks &points,
                      int center, float width, float height, float thickness)
{
    const auto c=points[center];
    const auto a=points[center+1], b=points[center+2], d=points[center+3], e=points[center+4];
    // Opposite rim landmarks define two axes; sample an ellipse instead of a diamond.
    const Point u{(a.x-d.x)*width*0.5f,(a.y-d.y)*height*0.5f};
    const Point v{(b.x-e.x)*width*0.5f,(b.y-e.y)*height*0.5f};
    if (std::abs(u.x*v.y-u.y*v.x)<0.01f) return;
    auto at=[&](int index) {
        const float angle=index*6.28318530718f/32;
        return Point{c.x*width+u.x*std::cos(angle)+v.x*std::sin(angle),
                     c.y*height+u.y*std::cos(angle)+v.y*std::sin(angle)};
    };
    for (int i=0;i<32;++i) segment(vertices,at(i),at(i+1),thickness);
}
inline std::vector<Point> geometry(const Landmarks &points, uint32_t mask,
                                   float width, float height, float thickness)
{
    std::vector<Point> vertices;
    std::set<std::pair<int,int>> edges;
    if (mask & Mesh) edges.insert(mesh_edges.begin(),mesh_edges.end());
    for (const auto &path : paths()) {
        if (!(path.part & mask)) continue;
        for (size_t i=1; i<path.points.size(); ++i) {
            int a=path.points[i-1], b=path.points[i];
            if (a>b) std::swap(a,b);
            edges.emplace(a,b);
        }
    }
    for (const auto [a,b]:edges)
        segment(vertices,{points[a].x*width,points[a].y*height},
                {points[b].x*width,points[b].y*height},thickness);
    if (mask & Iris) {
        iris_ring(vertices,points,468,width,height,thickness);
        iris_ring(vertices,points,473,width,height,thickness);
    }
    return vertices;
}
} // namespace cam
