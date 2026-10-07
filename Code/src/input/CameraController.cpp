#include "jely/input/CameraController.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace jely {
CameraController::CameraController() { reset(); }
void CameraController::reset() { target_={0,1.1f,0};yaw_=0.65f;pitch_=0.48f;distance_=12.3f;rebuild(); }
void CameraController::rebuild() {
    camera_.position={target_.x+distance_*std::cos(pitch_)*std::sin(yaw_),target_.y+distance_*std::sin(pitch_),target_.z+distance_*std::cos(pitch_)*std::cos(yaw_)};
    camera_.target=target_; camera_.up={0,1,0};camera_.fovy=42;camera_.projection=CAMERA_PERSPECTIVE;
}
void CameraController::update(bool pointerBlocked) {
    CameraInput input;
    if(!pointerBlocked) {
        Vector2 delta=GetMouseDelta();
        if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) input.orbit=delta;
        if(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) input.pan=delta;
        input.wheel=GetMouseWheelMove();
        input.movement.x=float(IsKeyDown(KEY_D))-float(IsKeyDown(KEY_A));
        input.movement.y=float(IsKeyDown(KEY_W))-float(IsKeyDown(KEY_S));
    }
    apply(input,std::min(GetFrameTime(),0.05f));
    if(IsKeyPressed(KEY_C)) reset();
}
void CameraController::apply(const CameraInput& input,float dt) {
    yaw_-=input.orbit.x*0.006f;pitch_=std::clamp(pitch_+input.orbit.y*0.006f,0.08f,1.48f);
    Vector3 forward=Vector3Normalize(Vector3Subtract(camera_.target,camera_.position));
    Vector3 right=Vector3Normalize(Vector3CrossProduct(forward,camera_.up));
    Vector3 up=Vector3Normalize(Vector3CrossProduct(right,forward));
    float scale=distance_*0.0016f;
    target_=Vector3Add(target_,Vector3Add(Vector3Scale(right,-input.pan.x*scale),Vector3Scale(up,input.pan.y*scale)));
    target_.y=std::clamp(target_.y,-0.5f,6.0f);
    distance_=std::clamp(distance_*std::exp(-input.wheel*0.10f),3.5f,26.0f);
    float speed=std::clamp(dt,0.0f,0.05f)*distance_*0.35f;
    target_=Vector3Add(target_,Vector3Add(Vector3Scale(right,input.movement.x*speed),Vector3Scale({forward.x,0,forward.z},input.movement.y*speed)));
    rebuild();
}
}
