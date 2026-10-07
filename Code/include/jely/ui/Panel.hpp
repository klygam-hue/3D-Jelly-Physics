#pragma once
#include "jely/physics/PhysicsWorld.hpp"
#include "jely/scene/Appearance.hpp"
#include "jely/ui/Motion.hpp"
#include "jely/ui/Glass.hpp"
#include "jely/render/Backend.hpp"
#include "raylib.h"
#include <string>
#include <array>

namespace jely {
enum class PanelTab { Simulation,Appearance,Jelly,Graphics,Ground };
struct UiState {
    bool paused=false,debug=false,hidden=false,resetRequested=false,impulseRequested=false,stepRequested=false;
    bool reducedMotion=false,solidUi=false;
    int preset=0,resolution=5;
    double physicsMs=0;
    double lostTime=0;
    int monitorHz=0,monitorIndex=-1;
    std::string monitorName;
    PanelTab tab=PanelTab::Jelly;
    Appearance appearance;
    std::string error;
    bool spawnRequested=false;
    std::string spawnMessage;
    std::string groundMessage;
    GraphicsApi graphicsApi=GraphicsApi::OpenGL,requestedApi=GraphicsApi::OpenGL;
    bool graphicsSwitchRequested=false;
    std::string graphicsDriver,graphicsMessage;
};
class Panel {
public:
    Panel();
    ~Panel();
    Panel(const Panel&)=delete;
    Panel& operator=(const Panel&)=delete;
    bool containsMouse(const UiState& state) const;
    float width(const UiState& state) const;
    struct Input {Vector2 mouse{};bool pressed=false,down=false;};
    void update(UiState& state,double dt,Input input);
    void restoreLayout(const UiState& state);
    void prepareBackdrop(Texture2D scene,std::uint64_t revision,const UiState& state);
    void draw(PhysicsWorld& world,UiState& state);
    double openAmount() const {return menu_.value;}
    unsigned int glassGeneration() const {return glass_.generation();}
    std::uint64_t blurUpdates() const {return glass_.blurUpdates();}
    Rectangle toggleBounds() const;
    Rectangle spawnBounds() const {return {float(GetScreenWidth())*0.5f-110,20,220,54};}
private:
    Color color(Color value) const;
    Rectangle translated(Rectangle rectangle) const;
    Vector2 pointer() const {return {input_.mouse.x-offsetX_,input_.mouse.y};}
    void rounded(Rectangle rectangle,float radius,Color value) const;
    void selectTab(UiState& state,PanelTab tab);
    void text(const char* value,float x,float y,float size,Color color) const;
    Vector2 measure(const char* value,float size) const;
    std::array<int,128> glyphIndex_{};
    bool button(Rectangle rectangle,const char* label,bool selected=false,float fontSize=15,int animationId=0);
    enum ControlId {MotionControl=-2,GlassControl=-3,SpawnControl=-4,GroundControl=-5};
    bool slider(const char* name,double& value,double minimum,double maximum,float y,const char* format);
    void drawAppearance(Appearance& appearance);
    void drawJelly(Appearance& appearance);
    void drawGraphics(UiState& state);
    void drawGround(UiState& state);
    Font font_{};
    Glass glass_;
    Input input_;
    AnimatedValue menu_{1,0},tabIndicator_{2,0},contentFade_{1,0};
    struct Response {int key=-1;AnimatedValue hover,press,selection;};
    std::array<Response,96> responses_{};
    PanelTab lastTab_=PanelTab::Jelly;
    double dt_=1.0/120.0;
    float offsetX_=0,opacity_=1;
    bool interactive_=true,reducedMotion_=false;
    bool ownsFont_=false;
    Texture2D previousShapesTexture_{};
    Rectangle previousShapesRectangle_{};
    int activeSlider_=-1;
    double nextTelemetryUpdate_=-1,cachedVolume_=1;
    std::size_t cachedNodes_{},telemetryStep_{};
};
}
