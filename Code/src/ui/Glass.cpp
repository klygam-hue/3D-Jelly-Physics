#include "jely/ui/Glass.hpp"
#include "jely/render/PortableShader.hpp"
#include "rlgl.h"
#include <algorithm>
#include <stdexcept>

namespace jely {
namespace {
constexpr const char* blurFragment=R"glsl(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 direction;
out vec4 finalColor;
void main() {
    vec3 color=texture(texture0,fragTexCoord).rgb*0.227027;
    color+=(texture(texture0,fragTexCoord+direction*1.384615).rgb+texture(texture0,fragTexCoord-direction*1.384615).rgb)*0.316216;
    color+=(texture(texture0,fragTexCoord+direction*3.230769).rgb+texture(texture0,fragTexCoord-direction*3.230769).rgb)*0.070270;
    finalColor=vec4(color,1.0);
})glsl";
constexpr const char* glassFragment=R"glsl(#version 330
uniform sampler2D texture0;
uniform vec2 viewport;
uniform vec4 bounds;
uniform float radius;
uniform float opacity;
uniform vec2 pointer;
uniform float solid;
out vec4 finalColor;
float roundedDistance(vec2 p,vec2 halfSize,float r) {
    vec2 q=abs(p)-halfSize+r;
    return length(max(q,0.0))+min(max(q.x,q.y),0.0)-r;
}
void main() {
    vec2 screen=vec2(gl_FragCoord.x,viewport.y-gl_FragCoord.y);
    vec2 local=screen-bounds.xy-bounds.zw*0.5;
    float d=roundedDistance(local,bounds.zw*0.5,radius);
    float coverage=1.0-smoothstep(-1.0,1.0,d);
    if(coverage<=0.0)discard;
    vec2 edgeNormal=normalize(vec2(dFdx(d),-dFdy(d))+vec2(0.0001));
    float rim=exp(-abs(d)*0.38);
    vec2 offset=edgeNormal*(2.0+5.0*rim);
    vec2 uv=(gl_FragCoord.xy+vec2(offset.x,-offset.y))/viewport;
    vec3 back=texture(texture0,clamp(uv,vec2(0.002),vec2(0.998))).rgb;
    float luminance=dot(back,vec3(0.2126,0.7152,0.0722));
    back=mix(vec3(luminance),back,0.82);
    vec3 color=mix(back,vec3(0.085,0.115,0.155),0.48);
    float vertical=clamp((screen.y-bounds.y)/bounds.w,0.0,1.0);
    color+=vec3(0.085,0.100,0.120)*(1.0-vertical);
    float gleam=0.018*exp(-length(screen-pointer)/180.0);
    color+=gleam;
    color=mix(color,vec3(0.095,0.12,0.16),solid);
    float top=clamp(-edgeNormal.y*0.55-edgeNormal.x*0.2+0.35,0.0,1.0);
    color+=vec3(0.65,0.78,0.9)*rim*(0.08+0.26*top);
    finalColor=vec4(color,coverage*opacity);
})glsl";
bool invalid(Shader shader){return shader.id==0||shader.id==rlGetShaderIdDefault();}
void release(RenderTexture2D target){if(target.id)UnloadRenderTexture(target);}
}
Glass::Glass() {
    blur_=loadPortableShader(nullptr,blurFragment);material_=loadPortableShader(nullptr,glassFragment);
    if(invalid(blur_)||invalid(material_)) {
        if(!invalid(blur_))UnloadShader(blur_);
        if(!invalid(material_))UnloadShader(material_);
        throw std::runtime_error("Liquid Glass shaders could not be compiled");
    }
    direction_=GetShaderLocation(blur_,"direction");
    constexpr const char* names[]{"viewport","bounds","radius","opacity","pointer","solid"};
    for(int i=0;i<Count;i++)uniforms_[i]=GetShaderLocation(material_,names[i]);
}
Glass::~Glass(){release(resolved_);for(auto target:blurred_)release(target);UnloadShader(blur_);UnloadShader(material_);}
void Glass::resize(int width,int height,bool resolve) {
    if(width==width_&&height==height_&&(!resolve||resolved_.id))return;
    const int smallWidth=std::max(1,(width+3)/4),smallHeight=std::max(1,(height+3)/4);
    std::array<RenderTexture2D,2> fresh{};RenderTexture2D newResolved{};
    for(auto& target:fresh)target=LoadRenderTexture(smallWidth,smallHeight);
    if(resolve)newResolved=LoadRenderTexture(width,height);
    if(!fresh[0].id||!fresh[1].id||!rlFramebufferComplete(fresh[0].id)||!rlFramebufferComplete(fresh[1].id)||(resolve&&(!newResolved.id||!rlFramebufferComplete(newResolved.id)))) {
        for(auto target:fresh)release(target);
        release(newResolved);
        throw std::runtime_error("Liquid Glass framebuffer creation failed");
    }
    for(auto target:fresh){SetTextureFilter(target.texture,TEXTURE_FILTER_BILINEAR);SetTextureWrap(target.texture,TEXTURE_WRAP_CLAMP);}
    for(auto target:blurred_)release(target);
    release(resolved_);
    blurred_=fresh;resolved_=newResolved;width_=width;height_=height;++generation_;
    revision_=std::numeric_limits<std::uint64_t>::max();
}
void Glass::prepare(Texture2D scene,std::uint64_t revision,bool enabled) {
    if(!enabled)return;
    resize(GetRenderWidth(),GetRenderHeight(),scene.id==0);
    if(revision_==revision)return;
    if(!scene.id) {
        // Resolve main-framebuffer MSAA at matching dimensions before downsampling.
        rlDrawRenderBatchActive();rlBindFramebuffer(RL_READ_FRAMEBUFFER,0);rlBindFramebuffer(RL_DRAW_FRAMEBUFFER,resolved_.id);
        rlBlitFramebuffer(0,0,width_,height_,0,0,width_,height_,0x00004000);
        rlBindFramebuffer(RL_READ_FRAMEBUFFER,0);rlBindFramebuffer(RL_DRAW_FRAMEBUFFER,0);
        scene=resolved_.texture;
    }
    BeginTextureMode(blurred_[0]);ClearBackground(BLACK);
    DrawTexturePro(scene,{0,0,float(scene.width),-float(scene.height)},{0,0,float(blurred_[0].texture.width),float(blurred_[0].texture.height)},{0,0},0,WHITE);
    EndTextureMode();
    for(int pass=0;pass<2;pass++) {
        Vector2 direction=pass==0?Vector2{1.0f/blurred_[0].texture.width,0}:Vector2{0,1.0f/blurred_[0].texture.height};
        SetShaderValue(blur_,direction_,&direction,SHADER_UNIFORM_VEC2);
        BeginTextureMode(blurred_[1-pass]);BeginShaderMode(blur_);
        DrawTextureRec(blurred_[pass].texture,{0,0,float(blurred_[pass].texture.width),-float(blurred_[pass].texture.height)},{0,0},WHITE);
        EndShaderMode();EndTextureMode();
    }
    revision_=revision;++blurUpdates_;
}
void Glass::draw(Rectangle r,float radius,float opacity,Vector2 pointer,bool solid) {
    if(opacity<=0.001f)return;
    for(int layer=3;layer>0;layer--){
        const float spread=float(layer)*2;
        DrawRectangleRounded({r.x-spread,r.y+3-spread,r.width+2*spread,r.height+2*spread},2*radius/std::min(r.width+2*spread,r.height+2*spread),12,{0,0,0,static_cast<unsigned char>(8*opacity)});
    }
    const Vector2 scale{float(GetRenderWidth())/GetScreenWidth(),float(GetRenderHeight())/GetScreenHeight()};
    const Vector2 viewport{float(GetRenderWidth()),float(GetRenderHeight())};
    float bounds[]{r.x*scale.x,r.y*scale.y,r.width*scale.x,r.height*scale.y};
    pointer={pointer.x*scale.x,pointer.y*scale.y};radius*=std::min(scale.x,scale.y);
    const float opaque=solid?1.0f:0.0f;
    SetShaderValue(material_,uniforms_[Viewport],&viewport,SHADER_UNIFORM_VEC2);
    SetShaderValue(material_,uniforms_[Bounds],bounds,SHADER_UNIFORM_VEC4);
    SetShaderValue(material_,uniforms_[Radius],&radius,SHADER_UNIFORM_FLOAT);
    SetShaderValue(material_,uniforms_[Opacity],&opacity,SHADER_UNIFORM_FLOAT);
    SetShaderValue(material_,uniforms_[Pointer],&pointer,SHADER_UNIFORM_VEC2);
    SetShaderValue(material_,uniforms_[Solid],&opaque,SHADER_UNIFORM_FLOAT);
    BeginShaderMode(material_);
    if(blurred_[0].id)DrawTexturePro(blurred_[0].texture,{0,0,float(blurred_[0].texture.width),float(blurred_[0].texture.height)},r,{0,0},0,WHITE);
    else DrawRectangleRec(r,WHITE);
    EndShaderMode();
}
}
