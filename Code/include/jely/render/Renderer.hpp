#pragma once
#include "jely/render/Surface.hpp"
#include "jely/physics/PhysicsWorld.hpp"
#include "jely/scene/Appearance.hpp"
#include "jely/render/SceneTargets.hpp"
#include "raylib.h"
#include <memory>
#include <cstdint>

namespace jely {
inline Vector3 graphics(Vec3 v) { return {float(v.x),float(v.y),float(v.z)}; }
inline Vec3 physics(Vector3 v) { return {v.x,v.y,v.z}; }
class RenderMesh {
public:
    explicit RenderMesh(const SoftBody& body);
    ~RenderMesh();
    RenderMesh(const RenderMesh&)=delete;
    RenderMesh& operator=(const RenderMesh&)=delete;
    bool update(const SoftBody& body,double alpha);
    void draw(Material material,Color color) const;
    RayCollision pick(Ray ray) const;
    const Surface& surface() const { return surface_; }
    Vec3 center() const { return center_; }
    bool graphicsChanged() const { return graphicsChanged_; }
private:
    Surface surface_;
    Mesh mesh_{};
    Vec3 center_{};
    struct NodePose {Vec3 current,previous;};
    std::vector<NodePose> cachedPose_;
    double cachedAlpha_=-1;
    bool graphicsChanged_=true;
};
class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    void sync(const PhysicsWorld& world,double alpha);
    void drawBackground(const Appearance& appearance) const;
    void draw(const PhysicsWorld& world,const Camera3D& camera,bool debug,const Appearance& appearance);
    RayCollision pick(std::size_t body,Ray ray) const;
    unsigned int targetGeneration() const { return targets_.generation(); }
    Texture2D uiBackdrop() const {return uiBackdrop_;}
    std::uint64_t sceneRevision() const {return sceneRevision_;}
private:
    enum Uniform {BaseColor,Jelly,Ground,LightVP,LightDirection,Eye,ViewMatrix,KeyColor,AmbientTop,AmbientBottom,FogColor,KeyIntensity,RimIntensity,Contrast,ShowGrid,ShowRing,Transparency,RefractionStrength,Gloss,TintStrength,ViewportSize,GroundHalfSize,GroundRoughness,GroundPattern,PatternScale,PatternStrength,GroundRingRadius,GroundOutline,UniformCount};
    std::array<int,UniformCount> uniforms_{};
    int backViewMatrix_=-1;
    bool shadowDirty_=true,shadowValid_=false;
    Vec3 shadowDirection_{};
    float shadowSpan_{};
    bool sceneDirty_=true,sceneValid_=false;
    bool directValid_=false;
    int directWidth_{},directHeight_{};
    Texture2D uiBackdrop_{};
    std::uint64_t sceneRevision_{};
    Camera3D cachedCamera_{};
    Appearance cachedAppearance_;
    unsigned int cachedTargetGeneration_{};
    int cachedLayer_{};
    void drawFloor(const Appearance& appearance);
    void drawDebug(const PhysicsWorld& world,const Camera3D& camera,bool debug);
    Vec3 bodyColor(std::size_t body,const PhysicsWorld& world,const Appearance& appearance) const;
    std::uint64_t structureRevision_=~std::uint64_t{0};
    void drawBodiesDepth(Material material) const;
    void configureMaterial(Vec3 color,float jelly,float ground);
    Shader shader_{},shadowShader_{},backShader_{};
    Material material_{},depthMaterial_{},backMaterial_{};
    RenderTexture2D shadow_{};
    Camera3D light_{};
    Mesh floor_{};
    SceneTargets targets_;
    std::vector<std::size_t> order_;
    std::vector<std::unique_ptr<RenderMesh>> meshes_;
};
}
