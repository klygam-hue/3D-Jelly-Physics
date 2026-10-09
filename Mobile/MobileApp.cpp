#include "TouchInput.hpp"
#include "MobilePlatform.hpp"
#include "jely/app/PhysicsClock.hpp"
#include "jely/input/CameraController.hpp"
#include "jely/render/Renderer.hpp"
#include "jely/ui/Glass.hpp"
#include "EmbeddedFont.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <memory>
#include <limits>
#include <stdexcept>
#include <vector>
#ifdef JELY_IPADOS
#include <SDL.h>
#endif

namespace jely::mobile {
namespace {
constexpr Color ink{235,247,255,255},muted{163,190,209,255},accent{88,232,204,255};
struct Parameter {const char* name;double low,high;std::function<double()> get;std::function<void(double)> set;};
class App {
public:
    explicit App(int smoke):smoke_(smoke) {
        SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_VSYNC_HINT);
        InitWindow(1280,800,"3D Jelly Physics 1.9.2");
        if(!IsWindowReady())throw std::runtime_error("Mobile graphics initialization failed");
        captureFramebuffer();driver_=verifyContext();
#ifdef JELY_IPADOS
        resolution_=7;
#endif
        world_.reset(0,resolution_);world_.settings.setSoftness(50);world_.seedSpawns(431);
        if(!smoke_)loadSettings();
        scale_=std::min({uiScale(),GetScreenWidth()/460.f,GetScreenHeight()/320.f});font_=LoadFontFromMemory(".ttf",embeddedFont,sizeof(embeddedFont),int(26*scale_),nullptr,0);
        if(!font_.texture.id)throw std::runtime_error("Embedded mobile font could not be loaded");
        SetTextureFilter(font_.texture,TEXTURE_FILTER_BILINEAR);
        renderer_=std::make_unique<Renderer>();glass_=std::make_unique<Glass>();
        renderer_->sync(world_,1);SetTargetFPS(fps_);previousTime_=GetTime();
    }
    ~App(){glass_.reset();renderer_.reset();UnloadFont(font_);CloseWindow();}
    bool frame() {
        const double now=GetTime();double dt=now-previousTime_;previousTime_=now;
        if(!active()) {cancelDrag();touch_.cancel();clock_.reset();if(dirty_)saveSettings();PollInputEvents();WaitTime(0.02);return true;}
        if(dt>0.5){dt=0;cancelDrag();touch_.cancel();clock_.reset();}
        std::array<Finger,8> fingers{};int count=std::clamp(GetTouchPointCount(),0,8);
        for(int i=0;i<count;i++){auto p=GetTouchPosition(i);fingers[i]={GetTouchPointId(i),{p.x,p.y}};}
        if(count==0&&IsMouseButtonDown(MOUSE_BUTTON_LEFT)){auto p=GetMousePosition();fingers[0]={-1,{p.x,p.y}};count=1;}
        auto input=touch_.advance(fingers.data(),count,count>0&&overUi({fingers[0].position.x,fingers[0].position.y}));
        if(smoke_)automate();
        if(input.released){cancelDrag();if(dirty_)saveSettings();}
        if(input.pressed&&!input.ui)startDrag({input.pointer.x,input.pointer.y});
        if(dragging_&&input.down&&!input.ui)moveDrag({input.pointer.x,input.pointer.y});
        CameraInput cameraInput;
        cameraInput.orbit={input.orbit.x,input.orbit.y};cameraInput.pan={input.pan.x,input.pan.y};cameraInput.wheel=input.wheel;
        if(input.down&&!input.ui&&!dragging_)cameraInput.orbit={input.delta.x,input.delta.y};
        if(count==0&&!overUi(GetMousePosition())){if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT))cameraInput.orbit=GetMouseDelta();cameraInput.wheel+=GetMouseWheelMove();}
        camera_.apply(cameraInput,float(dt));
        if(!paused_) {clock_.advance(world_,smoke_?PhysicsWorld::fixedStep*2:dt);appearance_.advanceTime(std::min(dt,0.1));}
        else clock_.reset();
        renderer_->sync(world_,paused_?1:clock_.alpha());
        BeginDrawing();ClearBackground({12,19,29,255});
        renderer_->draw(world_,camera_.camera(),debug_,appearance_);
        glass_->prepare(renderer_->uiBackdrop(),renderer_->sceneRevision(),!solid_);
        drawUi(input);errors_+=graphicsErrors();
        if(smoke_&&frames_==smoke_-1) {
            rlDrawRenderBatchActive();Image image=LoadImageFromScreen();
            captured_=image.data&&ExportImage(image,(storagePath()+"mobile-view.png").c_str());UnloadImage(image);
        }
        EndDrawing();++frames_;
        if(smoke_&&frames_>=smoke_){report();return false;}
        return !WindowShouldClose();
    }
    int result() const{return smoke_&&(!captured_||errors_||!error_.empty())?1:0;}
private:
    Rectangle rect(float x,float y,float w,float h) const{return {x*scale_,y*scale_,w*scale_,h*scale_};}
    float width()const{return GetScreenWidth()/scale_;}float height()const{return GetScreenHeight()/scale_;}
    Rectangle panel()const{return rect(12,72,std::min(540.f,width()-24),std::max(140.f,std::min(570.f,height()-114)));}
    bool overUi(Vector2 p) const{return CheckCollisionPointRec(p,rect(8,8,std::min(480.f,width()-16),56))||(menu_&&CheckCollisionPointRec(p,panel()));}
    void text(const char* value,float x,float y,float size=16,Color color=ink){DrawTextEx(font_,value,{x*scale_,y*scale_},size*scale_,0,color);}
    bool button(const char* label,Rectangle r,const TouchFrame& input,bool selected=false) {
        const bool hover=CheckCollisionPointRec({input.pointer.x,input.pointer.y},r);
        DrawRectangleRounded(r,0.45f,12,selected?Color{45,125,114,245}:hover&&input.down?Color{54,77,95,245}:Color{29,46,63,235});
        const float size=15*scale_;auto bounds=MeasureTextEx(font_,label,size,0);
        DrawTextEx(font_,label,{r.x+(r.width-bounds.x)*0.5f,r.y+(r.height-bounds.y)*0.5f},size,0,selected?accent:ink);
        return input.ui&&input.pressed&&hover;
    }
    void drawUi(const TouchFrame& input) {
        if(button(menu_?"Close":"Settings",rect(12,12,86,48),input,menu_))menu_=!menu_;
        if(button("Spawn",rect(104,12,78,48),input)){if(!world_.spawnRandom(resolution_))message_="16 bodies maximum";}
        if(button(paused_?"Resume":"Pause",rect(188,12,82,48),input,paused_)){paused_=!paused_;clock_.reset();cancelDrag();}
        if(button("Reset",rect(276,12,72,48),input)){world_.reset(preset_,resolution_);clock_.reset();cancelDrag();}
        if(button("Launch",rect(354,12,82,48),input))world_.impulse({0,5,0});
        if(!menu_)text("Drag jelly / orbit empty space   |   2 fingers: zoom + orbit   |   3: pan",16,height()-52,12,muted);
        if(menu_) {
            auto pane=panel();glass_->draw(pane,24*scale_,1,{input.pointer.x,input.pointer.y},solid_);
            const float paneWidth=pane.width/scale_,paneHeight=pane.height/scale_;
            constexpr const char* tabs[]{"Physics","Jelly","Scene","Ground","Tools"};
            for(int i=0;i<5;i++)if(button(tabs[i],rect(20+i*(paneWidth-16)/5,80,(paneWidth-24)/5,46),input,tab_==i)){tab_=i;page_=0;}
            auto parameters=settings();const int rows=std::max(1,int((paneHeight-116)/60));
            const int pages=std::max(1,int((parameters.size()+rows-1)/rows));page_=std::clamp(page_,0,pages-1);
            for(int row=0;row<rows;row++) {
                int index=page_*rows+row;if(index>=int(parameters.size()))break;
                auto& parameter=parameters[index];const float y=138+row*60;double value=parameter.get();
                const Rectangle hit=rect(24,y-4,paneWidth-24,54);
                if(input.ui&&input.down&&CheckCollisionPointRec({input.pointer.x,input.pointer.y},hit)) {
                    const double t=std::clamp((input.pointer.x/scale_-36)/(paneWidth-64),0.f,1.f);
                    parameter.set(parameter.low+t*(parameter.high-parameter.low));value=parameter.get();dirty_=true;
                }
                text(parameter.name,28,y,15);char number[40];std::snprintf(number,sizeof(number),"%.2f",value);text(number,paneWidth-55,y,14,accent);
                const float trackY=y+31,trackWidth=paneWidth-64;DrawRectangleRounded(rect(36,trackY,trackWidth,5),1,8,{62,85,102,255});
                const float t=float(std::clamp((value-parameter.low)/(parameter.high-parameter.low),0.0,1.0));
                DrawRectangleRounded(rect(36,trackY,trackWidth*t,5),1,8,accent);DrawCircleV({(36+trackWidth*t)*scale_,(trackY+2.5f)*scale_},10*scale_,accent);
            }
            const float navY=72+paneHeight-52;
            if(button("Back",rect(24,navY,82,44),input))page_=std::max(0,page_-1);
            char pager[32];std::snprintf(pager,sizeof(pager),"%d / %d",page_+1,pages);text(pager,125,navY+13,15,muted);
            if(button("Next",rect(paneWidth-78,navY,82,44),input))page_=std::min(pages-1,page_+1);
        }
        char status[160];std::snprintf(status,sizeof(status),"1.9.2  |  %d FPS  |  %zu bodies  |  %s",GetFPS(),world_.bodies().size(),resolution_==7?"Detailed":"Balanced");
        text(status,std::max(16.f,width()-380),height()-30,14,muted);
        if(!message_.empty())text(message_.c_str(),width()-300,74,15,accent);
        if(!error_.empty())text(error_.c_str(),16,height()-60,16,{255,153,138,255});
        if(dirty_&&!input.down)saveSettings();
    }
    std::vector<Parameter> settings() {
        std::vector<Parameter> result;
        auto field=[&](const char* name,double low,double high,double& value){result.push_back({name,low,high,[&value]{return value;},[&value](double next){value=next;}});};
        auto toggle=[&](const char* name,bool& value){result.push_back({name,0,1,[&value]{return value?1.:0.;},[&value](double next){value=next>=0.5;}});};
        if(tab_==0) {
            result.push_back({"Softness (%)",0,100,[this]{return world_.settings.softness();},[this](double v){world_.settings.setSoftness(v);}});
            field("Gravity",0,20,world_.settings.gravity);field("Friction",0,1,world_.settings.friction);field("Damping",0,2,world_.settings.damping);
            result.push_back({"Detailed mesh (0 / 1)",0,1,[this]{return resolution_==7?1.:0.;},[this](double v){int resolution=v>=0.5?7:5;if(resolution!=resolution_){resolution_=resolution;cancelDrag();world_.reset(preset_,resolution_);clock_.reset();}}});
            result.push_back({"Drop / High drop / Duet",0,2,[this]{return double(preset_);},[this](double v){int p=int(std::round(v));if(p!=preset_){preset_=p;cancelDrag();world_.reset(preset_,resolution_);clock_.reset();}}});
        } else if(tab_==1) {
            field("Transparency (%)",0,100,appearance_.transparency);field("Refraction",0,1,appearance_.refraction);field("Gloss",0,1,appearance_.gloss);field("Tint strength",0,1,appearance_.tintStrength);
            field("Hue",0,360,appearance_.jellyColor.hue);field("Saturation (%)",0,100,appearance_.jellyColor.saturation);field("Brightness (%)",0,100,appearance_.jellyColor.brightness);
        } else if(tab_==2) {
            field("Time of day",0,24,appearance_.timeOfDay);toggle("Auto day",appearance_.cycleTime);field("Contrast",0.8,2,appearance_.contrast);
            field("Background hue",0,360,appearance_.background.hue);field("Background saturation",0,100,appearance_.background.saturation);field("Background brightness",0,100,appearance_.background.brightness);
        } else if(tab_==3) {
            result.push_back({"Ground width",6,32,[this]{return appearance_.groundWidth;},[this](double v){if(world_.resizeGround(v,appearance_.groundDepth))appearance_.groundWidth=v;else message_="Move bodies before shrinking ground";}});
            result.push_back({"Ground depth",6,32,[this]{return appearance_.groundDepth;},[this](double v){if(world_.resizeGround(appearance_.groundWidth,v))appearance_.groundDepth=v;else message_="Move bodies before shrinking ground";}});
            field("Ground hue",0,360,appearance_.platform.hue);field("Ground saturation",0,100,appearance_.platform.saturation);field("Ground brightness",0,100,appearance_.platform.brightness);
            field("Visual roughness",0,1,appearance_.groundRoughness);toggle("Grid",appearance_.grid);toggle("Ring",appearance_.ring);toggle("Outline",appearance_.groundOutline);
            result.push_back({"Grid / checker tiles",0,1,[this]{return double(appearance_.groundPattern);},[this](double v){appearance_.groundPattern=v>=0.5?1:0;}});
            field("Pattern size",0.25,4,appearance_.groundPatternScale);field("Pattern strength",0,1,appearance_.groundPatternStrength);field("Ring radius",0.5,10,appearance_.groundRingRadius);
        } else {
            toggle("Opaque controls",solid_);toggle("Physics debug",debug_);
            result.push_back({"Render target 60 / 120 FPS",60,120,[this]{return double(fps_);},[this](double v){fps_=v>=90?120:60;SetTargetFPS(fps_);}});
            result.push_back({"Reset camera",0,1,[]{return 0.;},[this](double v){if(v>=0.5)camera_.reset();}});
        }
        return result;
    }
    void cancelDrag(){world_.releaseGrab();dragging_=false;}
    void startDrag(Vector2 pointer) {
        auto ray=GetScreenToWorldRay(pointer,camera_.camera());float closest=std::numeric_limits<float>::max();RayCollision best{};std::size_t body=0;
        for(std::size_t i=0;i<world_.bodies().size();i++){auto hit=renderer_->pick(i,ray);if(hit.hit&&hit.distance<closest){best=hit;closest=hit.distance;body=i;}}
        if(!best.hit)return;
        std::size_t node=0;double distance=std::numeric_limits<double>::max();const auto& nodes=world_.bodies()[body].nodes();
        for(std::size_t i=0;i<nodes.size();i++)if(nodes[i].surface){double d=(nodes[i].position-physics(best.point)).lengthSquared();if(d<distance){distance=d;node=i;}}
        const auto anchor=graphics(nodes[node].position);world_.setGrab(body,node,physics(anchor));
        planeNormal_=Vector3Normalize(Vector3Subtract(camera_.camera().target,camera_.camera().position));planeDistance_=Vector3DotProduct(anchor,planeNormal_);dragging_=true;
    }
    void moveDrag(Vector2 pointer) {
        auto ray=GetScreenToWorldRay(pointer,camera_.camera());const float denominator=Vector3DotProduct(ray.direction,planeNormal_);
        if(std::abs(denominator)>0.001f){float t=(planeDistance_-Vector3DotProduct(ray.position,planeNormal_))/denominator;if(t>0)world_.moveGrab(physics(Vector3Add(ray.position,Vector3Scale(ray.direction,t))));}
    }
    void automate() {
        if(frames_==5)world_.spawnRandom(resolution_);
        if(frames_==15){appearance_.transparency=0;world_.impulse({0,4,0});}
        if(frames_==25)appearance_.transparency=100;
        if(frames_==35)appearance_.transparency=65;
        if(frames_==45){world_.settings.setSoftness(100);menu_=true;}
        if(frames_==55){tab_=1;appearance_.jellyColor.hue=315;}
        if(frames_==65){world_.resizeGround(24,10);appearance_.groundWidth=24;appearance_.groundDepth=10;}
        if(frames_==75)world_.spawnRandom(resolution_);
        if(frames_==85){auto id=world_.bodies()[0].surfaceNodes().back();world_.setGrab(0,id,world_.bodies()[0].nodes()[id].position);}
        if(frames_>=86&&frames_<105)world_.moveGrab({1,3,0});
        if(frames_==105)world_.releaseGrab();
        for(const auto& body:world_.bodies())if(!body.stats().finite||body.stats().minTetRatio<=0)error_="Mobile regression: invalid body";
    }
    void report() {
        std::ofstream out(storagePath()+"mobile-report.txt");
        if(!out)throw std::runtime_error("Cannot save mobile validation report");
        out<<"version=1.9.2\nframes="<<frames_<<"\nphysics_steps="<<world_.stepCount()<<"\nshader_api=OpenGL ES 3\ndriver="<<driver_<<"\ngraphics_errors="<<errors_<<"\ncaptured="<<captured_<<"\nerror="<<error_<<'\n';
        for(const auto& body:world_.bodies()){auto s=body.stats();out<<"finite="<<s.finite<<" min_tet="<<s.minTetRatio<<" volume="<<s.volumeRatio<<'\n';}
        std::printf("JELY_MOBILE_RESULT %s graphics_errors=%u frames=%d\n",result()?"FAIL":"PASS",errors_,frames_);
    }
    void saveSettings() {
        if(smoke_){dirty_=false;return;}
        std::ofstream out(storagePath()+"settings.txt");
        out<<world_.settings.softness()<<' '<<world_.settings.gravity<<' '<<world_.settings.friction<<' '<<world_.settings.damping<<' '<<resolution_<<' '<<fps_<<' '<<solid_<<'\n';
        out<<appearance_.transparency<<' '<<appearance_.refraction<<' '<<appearance_.gloss<<' '<<appearance_.tintStrength<<' '<<appearance_.jellyColor.hue<<' '<<appearance_.jellyColor.saturation<<' '<<appearance_.jellyColor.brightness<<' '<<appearance_.timeOfDay<<' '<<appearance_.contrast<<'\n';
        if(!out)message_="Settings could not be saved";
        dirty_=false;
    }
    void loadSettings() {
        std::ifstream in(storagePath()+"settings.txt");
        if(!in){in.clear();in.open(storagePath()+"settings-191.txt");}
        double softness=50;PhysicsSettings settings=world_.settings;Appearance appearance=appearance_;int resolution=resolution_,fps=60;bool solid=false;
        if(!(in>>softness>>settings.gravity>>settings.friction>>settings.damping>>resolution>>fps>>solid))return;
        if(!(in>>appearance.transparency>>appearance.refraction>>appearance.gloss>>appearance.tintStrength>>appearance.jellyColor.hue>>appearance.jellyColor.saturation>>appearance.jellyColor.brightness>>appearance.timeOfDay>>appearance.contrast))return;
        try {settings.setSoftness(softness);settings.validate();appearance.validate();if((resolution!=5&&resolution!=7)||(fps!=60&&fps!=120))return;
            world_.settings=settings;appearance_=appearance;resolution_=resolution;fps_=fps;solid_=solid;world_.reset(0,resolution_);
        }catch(const std::exception&){/* Corrupt preferences keep safe defaults. */}
    }
    PhysicsWorld world_;PhysicsClock clock_;CameraController camera_;Appearance appearance_;TouchInput touch_;
    std::unique_ptr<Renderer> renderer_;std::unique_ptr<Glass> glass_;Font font_{};
    bool menu_=false,paused_=false,dragging_=false,solid_=false,debug_=false,dirty_=false,captured_=false;
    int resolution_=5,preset_=0,fps_=60,tab_=0,page_=0,frames_=0,smoke_=0;
    float scale_=1,planeDistance_=0;Vector3 planeNormal_{};double previousTime_=0;unsigned int errors_=0;
    std::string driver_,message_,error_;
};
}
}

int main(int argc,char** argv) {
    try {
        static std::unique_ptr<jely::mobile::App> app=std::make_unique<jely::mobile::App>(jely::mobile::smokeFrames(argc,argv));
#ifdef JELY_IPADOS
        SDL_iPhoneSetAnimationCallback(static_cast<SDL_Window*>(GetWindowHandle()),1,[](void*){
            try {if(!app->frame())std::exit(app->result());}catch(const std::exception& e){std::fprintf(stderr,"Mobile fatal: %s\n",e.what());std::exit(1);}
        },nullptr);
        return 0;
#else
        while(app->frame()){}
        const int result=app->result();app.reset();return result;
#endif
    }catch(const std::exception& e){std::fprintf(stderr,"Mobile fatal: %s\n",e.what());return 1;}
}
