#pragma once
#include "jely/math/Vec3.hpp"
#include <vector>

namespace jely {
struct SphereObstacle { Vec3 center; double radius; };
struct BoxObstacle { Vec3 center,halfSize; };
struct Scene {
    std::vector<SphereObstacle> spheres;
    std::vector<BoxObstacle> boxes;
    double halfExtent=8.0;
    double halfDepth=8.0; // halfExtent is the X half-width; both remain 8 for existing fixtures.
    void configure(int preset);
};
}
