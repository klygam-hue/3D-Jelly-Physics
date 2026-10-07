#pragma once
#include "raylib.h"
#include <array>
#include <cstdint>
#include <limits>

namespace jely {
// Shared GPU-only backdrop. Never samples a texture attached to the active target.
class Glass {
public:
    Glass();
    ~Glass();
    Glass(const Glass&)=delete;
    Glass& operator=(const Glass&)=delete;
    void prepare(Texture2D scene,std::uint64_t revision,bool enabled);
    void draw(Rectangle rectangle,float radius,float opacity,Vector2 pointer,bool solid=false);
    unsigned int generation() const {return generation_;}
    std::uint64_t blurUpdates() const {return blurUpdates_;}
private:
    void resize(int width,int height,bool resolve);
    Shader blur_{},material_{};
    int direction_=-1;
    enum Uniform {Viewport,Bounds,Radius,Opacity,Pointer,Solid,Count};
    std::array<int,Count> uniforms_{};
    RenderTexture2D resolved_{};
    std::array<RenderTexture2D,2> blurred_{};
    int width_{},height_{};
    unsigned int generation_{};
    std::uint64_t blurUpdates_{},revision_=std::numeric_limits<std::uint64_t>::max();
};
}
