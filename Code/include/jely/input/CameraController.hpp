#pragma once
#include "raylib.h"

namespace jely {
struct CameraInput {
    Vector2 orbit{},pan{};
    float wheel=0;
    Vector2 movement{};
};
class CameraController {
public:
    CameraController();
    void update(bool pointerBlocked);
    void reset();
    const Camera3D& camera() const { return camera_; }
    void apply(const CameraInput& input,float dt);
private:
    void rebuild();
    Camera3D camera_{};
    Vector3 target_{0,1.1f,0};
    float yaw_=0.65f,pitch_=0.48f,distance_=12.3f;
};
}
