#include "jely/render/SceneTargets.hpp"
#include "rlgl.h"
#include <stdexcept>

namespace jely {
SceneTargets::~SceneTargets() {
    for(auto layer:layers_)if(layer.id)UnloadRenderTexture(layer);
    if(backDepth_.id)UnloadRenderTexture(backDepth_);
}
void SceneTargets::resize(int width,int height) {
    if(width==width_&&height==height_)return;
    if(width<1||height<1)throw std::invalid_argument("Invalid render target dimensions");
    std::array<RenderTexture2D,3> fresh{};
    for(auto& target:fresh)target=LoadRenderTexture(width,height);
    for(auto target:fresh)if(!target.id||!rlFramebufferComplete(target.id)){
        for(auto allocated:fresh)if(allocated.id)UnloadRenderTexture(allocated);
        throw std::runtime_error("Transparency framebuffer creation failed");
    }
    for(auto target:fresh){SetTextureWrap(target.texture,TEXTURE_WRAP_CLAMP);SetTextureFilter(target.texture,TEXTURE_FILTER_BILINEAR);}
    // Packed depth must be decoded per texel: filtering encoded digits is invalid.
    SetTextureFilter(fresh[2].texture,TEXTURE_FILTER_POINT);
    for(auto old:layers_)if(old.id)UnloadRenderTexture(old);
    if(backDepth_.id)UnloadRenderTexture(backDepth_);
    layers_[0]=fresh[0];layers_[1]=fresh[1];backDepth_=fresh[2];
    width_=width;height_=height;++generation_;
}
void SceneTargets::copyLayer(int source,int destination) const {
    if(source==destination||source<0||source>1||destination<0||destination>1)throw std::invalid_argument("Invalid compositing layer");
    rlDrawRenderBatchActive();
    rlBindFramebuffer(RL_READ_FRAMEBUFFER,layers_[source].id);
    rlBindFramebuffer(RL_DRAW_FRAMEBUFFER,layers_[destination].id);
    constexpr int colorAndDepthMask=0x00004000|0x00000100;
    rlBlitFramebuffer(0,0,width_,height_,0,0,width_,height_,colorAndDepthMask);
    rlBindFramebuffer(RL_READ_FRAMEBUFFER,0);rlBindFramebuffer(RL_DRAW_FRAMEBUFFER,0);
}
}
