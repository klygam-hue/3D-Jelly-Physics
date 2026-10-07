#include "jely/render/Renderer.hpp"
#include "jely/render/Shaders.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cstring>
#include <stdexcept>
#include <numeric>

namespace jely {
RenderMesh::RenderMesh(const SoftBody& body) {
    mesh_.vertexCount=static_cast<int>(surface_.positions().size());
    mesh_.triangleCount=static_cast<int>(surface_.indices().size()/3);
    mesh_.vertices=static_cast<float*>(MemAlloc(static_cast<unsigned int>(mesh_.vertexCount*3*sizeof(float))));
    mesh_.normals=static_cast<float*>(MemAlloc(static_cast<unsigned int>(mesh_.vertexCount*3*sizeof(float))));
    mesh_.texcoords=static_cast<float*>(MemAlloc(static_cast<unsigned int>(mesh_.vertexCount*2*sizeof(float))));
    mesh_.indices=static_cast<unsigned short*>(MemAlloc(static_cast<unsigned int>(surface_.indices().size()*sizeof(unsigned short))));
    if(!mesh_.vertices||!mesh_.normals||!mesh_.texcoords||!mesh_.indices) { UnloadMesh(mesh_); throw std::runtime_error("Render mesh allocation failed"); }
    std::memset(mesh_.vertices,0,static_cast<std::size_t>(mesh_.vertexCount)*3*sizeof(float));
    std::memset(mesh_.normals,0,static_cast<std::size_t>(mesh_.vertexCount)*3*sizeof(float));
    std::memset(mesh_.texcoords,0,static_cast<std::size_t>(mesh_.vertexCount)*2*sizeof(float));
    std::memcpy(mesh_.indices,surface_.indices().data(),surface_.indices().size()*sizeof(unsigned short));
    UploadMesh(&mesh_,true);
    if(mesh_.vaoId==0) { UnloadMesh(mesh_); throw std::runtime_error("GPU mesh creation failed"); }
    update(body,1);
}
RenderMesh::~RenderMesh() { UnloadMesh(mesh_); }
bool RenderMesh::update(const SoftBody& body,double alpha) {
    graphicsChanged_=false;
    auto equal=[](Vec3 a,Vec3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;};
    bool unchanged=cachedAlpha_==alpha&&cachedPose_.size()==body.nodes().size();
    if(unchanged)for(std::size_t i=0;i<cachedPose_.size();i++)if(!equal(cachedPose_[i].current,body.nodes()[i].position)||!equal(cachedPose_[i].previous,body.nodes()[i].framePrevious)){unchanged=false;break;}
    if(unchanged)return false;
    cachedPose_.resize(body.nodes().size());cachedAlpha_=alpha;
    for(std::size_t i=0;i<cachedPose_.size();i++)cachedPose_[i]={body.nodes()[i].position,body.nodes()[i].framePrevious};
    center_=body.center();
    surface_.update(body,alpha);
    bool verticesChanged=false,normalsChanged=false;
    for(std::size_t i=0;i<surface_.positions().size();i++) {
        Vec3 p=surface_.positions()[i],n=surface_.normals()[i];
        const std::array<float,3> position{float(p.x),float(p.y),float(p.z)},normal{float(n.x),float(n.y),float(n.z)};
        for(std::size_t k=0;k<3;k++) {
            verticesChanged=verticesChanged||mesh_.vertices[i*3+k]!=position[k];
            normalsChanged=normalsChanged||mesh_.normals[i*3+k]!=normal[k];
            mesh_.vertices[i*3+k]=position[k];mesh_.normals[i*3+k]=normal[k];
        }
    }
    if(verticesChanged)UpdateMeshBuffer(mesh_,0,mesh_.vertices,mesh_.vertexCount*3*static_cast<int>(sizeof(float)),0);
    if(normalsChanged)UpdateMeshBuffer(mesh_,2,mesh_.normals,mesh_.vertexCount*3*static_cast<int>(sizeof(float)),0);
    graphicsChanged_=verticesChanged||normalsChanged;
    return verticesChanged;
}
void RenderMesh::draw(Material material,Color color) const { material.maps[MATERIAL_MAP_DIFFUSE].color=color;DrawMesh(mesh_,material,MatrixIdentity()); }
RayCollision RenderMesh::pick(Ray ray) const { return GetRayCollisionMesh(ray,mesh_,MatrixIdentity()); }
Renderer::Renderer() {
    shader_=LoadShaderFromMemory(shaders::vertex,shaders::fragment);
    shadowShader_=LoadShaderFromMemory(shaders::shadowVertex,shaders::shadowFragment);
    backShader_=LoadShaderFromMemory(shaders::backVertex,shaders::backFragment);
    if(shader_.id==rlGetShaderIdDefault()||shadowShader_.id==rlGetShaderIdDefault()||backShader_.id==rlGetShaderIdDefault()||shader_.id==0||shadowShader_.id==0||backShader_.id==0) {
        for(auto shader:{shader_,shadowShader_,backShader_})if(shader.id&&shader.id!=rlGetShaderIdDefault())UnloadShader(shader);
        throw std::runtime_error("OpenGL 3.3 shaders could not be compiled");
    }
    material_=LoadMaterialDefault();depthMaterial_=LoadMaterialDefault();backMaterial_=LoadMaterialDefault();
    material_.shader=shader_;depthMaterial_.shader=shadowShader_;backMaterial_.shader=backShader_;
    shadow_=LoadRenderTexture(2048,2048);
    if(shadow_.id==0||!rlFramebufferComplete(shadow_.id)) {
        // Ownership stays explicit: materials borrow the shader and shadow texture.
        material_.shader={}; depthMaterial_.shader={};
        MemFree(material_.maps);MemFree(depthMaterial_.maps);MemFree(backMaterial_.maps);UnloadShader(shader_);UnloadShader(shadowShader_);UnloadShader(backShader_);
        if(shadow_.id)UnloadRenderTexture(shadow_);
        throw std::runtime_error("Shadow framebuffer creation failed");
    }
    SetTextureFilter(shadow_.texture,TEXTURE_FILTER_POINT);
    SetTextureWrap(shadow_.texture,TEXTURE_WRAP_CLAMP);
    shader_.locs[SHADER_LOC_MAP_METALNESS]=GetShaderLocation(shader_,"shadowMap");
    shader_.locs[SHADER_LOC_MAP_ROUGHNESS]=GetShaderLocation(shader_,"sceneColor");
    shader_.locs[SHADER_LOC_MAP_EMISSION]=GetShaderLocation(shader_,"backDepth");
    constexpr const char* names[]{"baseColor","jelly","ground","lightVP","lightDirection","eye","viewMatrix","keyColor","ambientTop","ambientBottom","fogColor","keyIntensity","rimIntensity","contrast","showGrid","showRing","transparency","refractionStrength","gloss","tintStrength","viewportSize","groundHalfSize","groundRoughness","groundPattern","patternScale","patternStrength","groundRingRadius","groundOutline"};
    for(int i=0;i<UniformCount;i++)uniforms_[i]=GetShaderLocation(shader_,names[i]);
    backViewMatrix_=GetShaderLocation(backShader_,"viewMatrix");
    material_.maps[MATERIAL_MAP_METALNESS].texture=shadow_.texture;
    floor_=GenMeshPlane(1,1,1,1);
    light_.position={-5,10,6}; light_.target={0,0,0};light_.up={0,1,0};light_.fovy=19;light_.projection=CAMERA_ORTHOGRAPHIC;
}
Renderer::~Renderer() {
    meshes_.clear();UnloadMesh(floor_);
    // UnloadMaterial would also free the borrowed shadow map and shared shader.
    MemFree(material_.maps);MemFree(depthMaterial_.maps);MemFree(backMaterial_.maps);
    UnloadRenderTexture(shadow_);UnloadShader(shader_);UnloadShader(shadowShader_);UnloadShader(backShader_);
}
void Renderer::sync(const PhysicsWorld& world,double alpha) {
    if(structureRevision_!=world.structureRevision()){shadowDirty_=true;sceneDirty_=true;structureRevision_=world.structureRevision();}
    if(meshes_.size()>world.bodies().size()){shadowDirty_=true;sceneDirty_=true;meshes_.resize(world.bodies().size());}
    while(meshes_.size()<world.bodies().size()){shadowDirty_=true;sceneDirty_=true;meshes_.push_back(std::make_unique<RenderMesh>(world.bodies()[meshes_.size()]));}
    for(std::size_t i=0;i<meshes_.size();i++){
        shadowDirty_=meshes_[i]->update(world.bodies()[i],alpha)||shadowDirty_;
        sceneDirty_=meshes_[i]->graphicsChanged()||sceneDirty_;
    }
}
void Renderer::configureMaterial(Vec3 color,float jelly,float ground) {
    float rgba[]{float(color.x),float(color.y),float(color.z),1};
    SetShaderValue(shader_,uniforms_[BaseColor],rgba,SHADER_UNIFORM_VEC4);
    SetShaderValue(shader_,uniforms_[Jelly],&jelly,SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader_,uniforms_[Ground],&ground,SHADER_UNIFORM_FLOAT);
}
Vec3 Renderer::bodyColor(std::size_t body,const PhysicsWorld& world,const Appearance& appearance) const {
    if(const auto& color=world.bodies()[body].customColor();color)return *color;
    auto hsv=appearance.jellyColor;
    hsv.hue=std::fmod(hsv.hue+150.0*body,360.0);
    return hsv.rgb();
}
void Renderer::drawBodiesDepth(Material material) const {
    for(const auto& mesh:meshes_)mesh->draw(material,WHITE);
}
void Renderer::drawBackground(const Appearance& appearance) const {
    const auto lighting=appearance.lighting();
    auto color=[](Vec3 c){return Color{static_cast<unsigned char>(std::clamp(c.x,0.0,1.0)*255),static_cast<unsigned char>(std::clamp(c.y,0.0,1.0)*255),static_cast<unsigned char>(std::clamp(c.z,0.0,1.0)*255),255};};
    DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),color(lighting.backgroundTop),color(lighting.backgroundBottom));
}
void Renderer::draw(const PhysicsWorld& world,const Camera3D& camera,bool debug,const Appearance& appearance) {
    uiBackdrop_={};
    const bool transparent=appearance.transparency>0&&appearance.transparency<100;
    if(transparent)targets_.resize(GetScreenWidth(),GetScreenHeight());
    const auto lighting=appearance.lighting();
    auto same=[](Vector3 a,Vector3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;};
    const bool sameCamera=same(camera.position,cachedCamera_.position)&&same(camera.target,cachedCamera_.target)&&same(camera.up,cachedCamera_.up)&&camera.fovy==cachedCamera_.fovy&&camera.projection==cachedCamera_.projection;
    auto ordered=[&](auto a,auto b){double da=(meshes_[a]->center()-physics(camera.position)).lengthSquared(),db=(meshes_[b]->center()-physics(camera.position)).lengthSquared();return da==db?a<b:da>db;};
    if(transparent&&sceneValid_&&!sceneDirty_&&sameCamera&&appearance==cachedAppearance_&&targets_.generation()==cachedTargetGeneration_&&std::is_sorted(order_.begin(),order_.end(),ordered)){
        uiBackdrop_=targets_.layer(cachedLayer_).texture;
        DrawTextureRec(targets_.layer(cachedLayer_).texture,{0,0,float(targets_.width()),-float(targets_.height())},{0,0},WHITE);
        drawDebug(world,camera,debug);return;
    }
    const float span=float(std::max(19.0,std::max(appearance.groundWidth,appearance.groundDepth)*1.5));
    if(span!=shadowSpan_){shadowDirty_=true;shadowSpan_=span;}
    light_.fovy=span;light_.position=graphics(lighting.direction*std::max(13.0,double(span)));
    if(appearance.transparency<100&&(shadowDirty_||!shadowValid_||shadowDirection_.x!=lighting.direction.x||shadowDirection_.y!=lighting.direction.y||shadowDirection_.z!=lighting.direction.z)) {
    BeginTextureMode(shadow_);ClearBackground(WHITE);BeginMode3D(light_);
    // Alpha is a depth digit in the packed map, so blending would corrupt it.
    rlDisableColorBlend();
    if(appearance.transparency<100)drawBodiesDepth(depthMaterial_);
    rlEnableColorBlend();
    EndMode3D();EndTextureMode();
    shadowDirection_=lighting.direction;shadowValid_=true;shadowDirty_=false;
    }
    Matrix lightVP=MatrixMultiply(GetCameraMatrix(light_),MatrixOrtho(-span*0.5,span*0.5,-span*0.5,span*0.5,0.01,1000));
    SetShaderValueMatrix(shader_,uniforms_[LightVP],lightVP);
    Vector3 direction=Vector3Normalize(Vector3Subtract(light_.position,light_.target));
    SetShaderValue(shader_,uniforms_[LightDirection],&direction,SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_,uniforms_[Eye],&camera.position,SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(shader_,uniforms_[ViewMatrix],GetCameraMatrix(camera));
    if(transparent)SetShaderValueMatrix(backShader_,backViewMatrix_,GetCameraMatrix(camera));
    auto vectorUniform=[&](Uniform location,Vec3 value){auto v=graphics(value);SetShaderValue(shader_,uniforms_[location],&v,SHADER_UNIFORM_VEC3);};
    auto scalarUniform=[&](Uniform location,double value){float f=float(value);SetShaderValue(shader_,uniforms_[location],&f,SHADER_UNIFORM_FLOAT);};
    vectorUniform(KeyColor,lighting.keyColor);vectorUniform(AmbientTop,lighting.ambientTop);vectorUniform(AmbientBottom,lighting.ambientBottom);
    vectorUniform(FogColor,lighting.backgroundTop);
    scalarUniform(KeyIntensity,lighting.keyIntensity);scalarUniform(RimIntensity,lighting.rimIntensity);scalarUniform(Contrast,appearance.contrast);
    scalarUniform(ShowGrid,appearance.grid?1:0);scalarUniform(ShowRing,appearance.ring?1:0);
    scalarUniform(Transparency,appearance.transparency/100.0);scalarUniform(RefractionStrength,appearance.refraction);
    scalarUniform(Gloss,appearance.gloss);scalarUniform(TintStrength,appearance.tintStrength);
    Vector2 groundHalf{float(appearance.groundWidth*0.5),float(appearance.groundDepth*0.5)};
    SetShaderValue(shader_,uniforms_[GroundHalfSize],&groundHalf,SHADER_UNIFORM_VEC2);
    scalarUniform(GroundRoughness,appearance.groundRoughness);scalarUniform(GroundPattern,appearance.groundPattern);
    scalarUniform(PatternScale,appearance.groundPatternScale);scalarUniform(PatternStrength,appearance.groundPatternStrength);
    scalarUniform(GroundRingRadius,appearance.groundRingRadius);scalarUniform(GroundOutline,appearance.groundOutline?1:0);
    Vector2 viewport{float(GetScreenWidth()),float(GetScreenHeight())};
    SetShaderValue(shader_,uniforms_[ViewportSize],&viewport,SHADER_UNIFORM_VEC2);
    if(!transparent) {
        if(!directValid_||sceneDirty_||!sameCamera||appearance!=cachedAppearance_||directWidth_!=GetScreenWidth()||directHeight_!=GetScreenHeight())++sceneRevision_;
        drawBackground(appearance);BeginMode3D(camera);drawFloor(appearance);
        if(appearance.transparency==0)for(std::size_t i=0;i<meshes_.size();i++){configureMaterial(bodyColor(i,world,appearance),1,0);meshes_[i]->draw(material_,WHITE);}
        EndMode3D();drawDebug(world,camera,debug);
        cachedCamera_=camera;cachedAppearance_=appearance;directWidth_=GetScreenWidth();directHeight_=GetScreenHeight();
        directValid_=true;sceneValid_=false;sceneDirty_=false;return;
    }
    BeginTextureMode(targets_.layer(0));ClearBackground(BLACK);drawBackground(appearance);
    // Borrow a harmless texture for the opaque pass; never bind its own color attachment.
    BeginMode3D(camera);drawFloor(appearance);
    EndMode3D();EndTextureMode();
    order_.resize(meshes_.size());std::iota(order_.begin(),order_.end(),std::size_t{0});
    const Vec3 eye=physics(camera.position);
    std::sort(order_.begin(),order_.end(),[&](auto a,auto b){
        double da=(meshes_[a]->center()-eye).lengthSquared(),db=(meshes_[b]->center()-eye).lengthSquared();
        return da==db?a<b:da>db;
    });
    int current=0;
    if(appearance.transparency<100)for(auto index:order_) {
        BeginTextureMode(targets_.backDepth());ClearBackground(WHITE);BeginMode3D(camera);
        rlDisableColorBlend();rlSetCullFace(RL_CULL_FACE_FRONT);
        meshes_[index]->draw(backMaterial_,WHITE);
        rlSetCullFace(RL_CULL_FACE_BACK);rlEnableColorBlend();EndMode3D();EndTextureMode();
        int next=1-current;targets_.copyLayer(current,next);
        material_.maps[MATERIAL_MAP_ROUGHNESS].texture=targets_.layer(current).texture;
        material_.maps[MATERIAL_MAP_EMISSION].texture=targets_.backDepth().texture;
        BeginTextureMode(targets_.layer(next));BeginMode3D(camera);
        configureMaterial(bodyColor(index,world,appearance),1,0);meshes_[index]->draw(material_,WHITE);
        EndMode3D();EndTextureMode();current=next;
    }
    // Match raylib's bottom-up framebuffer orientation when presenting the finished scene.
    DrawTextureRec(targets_.layer(current).texture,{0,0,float(targets_.width()),-float(targets_.height())},{0,0},WHITE);
    cachedCamera_=camera;cachedAppearance_=appearance;cachedTargetGeneration_=targets_.generation();cachedLayer_=current;
    sceneValid_=true;sceneDirty_=false;
    directValid_=false;uiBackdrop_=targets_.layer(current).texture;++sceneRevision_;
    drawDebug(world,camera,debug);
}
void Renderer::drawFloor(const Appearance& appearance) {
    material_.maps[MATERIAL_MAP_ROUGHNESS].texture=material_.maps[MATERIAL_MAP_DIFFUSE].texture;
    material_.maps[MATERIAL_MAP_EMISSION].texture=material_.maps[MATERIAL_MAP_DIFFUSE].texture;
    configureMaterial(appearance.platform.rgb(),0,1);DrawMesh(floor_,material_,MatrixScale(float(appearance.groundWidth),1,float(appearance.groundDepth)));
}
void Renderer::drawDebug(const PhysicsWorld& world,const Camera3D& camera,bool debug) {
    if(!debug&&!world.grab())return;
    BeginMode3D(camera);
    if(debug) {
        for(const auto& body:world.bodies()) {
            for(const auto& edge:body.edges()) DrawLine3D(graphics(body.nodes()[edge.a].position),graphics(body.nodes()[edge.b].position),{86,244,218,90});
            for(const auto& n:body.nodes()) if(n.surface) DrawSphere(graphics(n.position),0.028f,{244,250,255,220});
        }
        DrawCubeWires({0,2,0},float(world.scene().halfExtent*2),4,float(world.scene().halfDepth*2),{78,117,150,150});
    }
    if(world.grab()) {
        const auto& g=*world.grab();Vector3 target=graphics(g.target),node=graphics(world.bodies()[g.body].nodes()[g.node].position);
        DrawLine3D(node,target,{255,224,151,255});DrawSphereWires(target,0.10f,8,12,{255,224,151,255});
    }
    EndMode3D();
}
RayCollision Renderer::pick(std::size_t body,Ray ray) const { return body<meshes_.size()?meshes_[body]->pick(ray):RayCollision{}; }
}
