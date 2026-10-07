#pragma once
#include "raylib.h"
#include <array>

namespace jely {
// Each composition stage reads a different texture from its active draw target.
class SceneTargets {
public:
    ~SceneTargets();
    SceneTargets()=default;
    SceneTargets(const SceneTargets&)=delete;
    SceneTargets& operator=(const SceneTargets&)=delete;
    void resize(int width,int height);
    void copyLayer(int source,int destination) const;
    const RenderTexture2D& layer(int index) const { return layers_[index]; }
    const RenderTexture2D& backDepth() const { return backDepth_; }
    int width() const { return width_; }
    int height() const { return height_; }
    unsigned int generation() const { return generation_; }
private:
    std::array<RenderTexture2D,2> layers_{};
    RenderTexture2D backDepth_{};
    int width_{},height_{};
    unsigned int generation_{};
};
}
