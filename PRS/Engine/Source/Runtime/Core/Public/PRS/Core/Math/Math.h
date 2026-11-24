#pragma once
#include <cmath>
#include <array>

namespace PRS { namespace Math {
struct Vec3 { float x,y,z; };
struct Mat4 { std::array<float,16> m{}; };
inline float Dot(const Vec3&a,const Vec3&b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 Cross(const Vec3&a,const Vec3&b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float Length(const Vec3&v){return std::sqrt(Dot(v,v));}
}}