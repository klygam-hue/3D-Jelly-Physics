#pragma once
#include <algorithm>
#include <cmath>

namespace jely {
struct Vec3 {
    double x{}, y{}, z{};
    constexpr Vec3() = default;
    constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    constexpr Vec3 operator+(Vec3 b) const { return {x+b.x,y+b.y,z+b.z}; }
    constexpr Vec3 operator-(Vec3 b) const { return {x-b.x,y-b.y,z-b.z}; }
    constexpr Vec3 operator-() const { return {-x,-y,-z}; }
    constexpr Vec3 operator*(double s) const { return {x*s,y*s,z*s}; }
    constexpr Vec3 operator/(double s) const { return *this*(1.0/s); }
    Vec3& operator+=(Vec3 b) { *this=*this+b; return *this; }
    Vec3& operator-=(Vec3 b) { *this=*this-b; return *this; }
    Vec3& operator*=(double s) { *this=*this*s; return *this; }
    constexpr double dot(Vec3 b) const { return x*b.x+y*b.y+z*b.z; }
    constexpr Vec3 cross(Vec3 b) const { return {y*b.z-z*b.y,z*b.x-x*b.z,x*b.y-y*b.x}; }
    double lengthSquared() const { return dot(*this); }
    double length() const { return std::sqrt(lengthSquared()); }
    Vec3 normalized(Vec3 fallback={0,1,0}) const { double l=length(); return l>1e-12?*this/l:fallback; }
    bool finite() const { return std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z); }
};
inline Vec3 operator*(double s, Vec3 v) { return v*s; }
inline Vec3 lerp(Vec3 a,Vec3 b,double t) { return a*(1-t)+b*t; }
inline Vec3 limited(Vec3 v,double maxLength) {
    double lengthSquared=v.lengthSquared();
    return lengthSquared>maxLength*maxLength?v*(maxLength/std::sqrt(lengthSquared)):v;
}
inline double signedVolume(Vec3 a,Vec3 b,Vec3 c,Vec3 d) { return (b-a).dot((c-a).cross(d-a))/6.0; }
}
