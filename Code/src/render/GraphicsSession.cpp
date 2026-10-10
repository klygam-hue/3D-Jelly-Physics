#include "jely/render/GraphicsSession.hpp"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <stdexcept>

namespace jely {
struct GraphicsSession::Window {
    Window(GraphicsRuntime& runtime,GraphicsApi api,bool benchmark,int width,int height,Vector2 position,bool probeChild) {
        runtime.select(api,probeChild);
        SetTraceLogLevel(benchmark?LOG_WARNING:LOG_INFO);
        SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE|(benchmark?0:FLAG_VSYNC_HINT)|(probeChild?FLAG_WINDOW_HIDDEN:0));
        InitWindow(width,height,"3D Jelly Physics 1.9.3 | Soft Body Laboratory");
        if(!IsWindowReady()){glfwTerminate();throw std::runtime_error("Window/graphics initialization failed");}
        try{runtime.verifyContext(api);}catch(...){CloseWindow();throw;}
        SetWindowMinSize(1160,900);SetTargetFPS(0); // Application follows the active monitor; benchmarks remain uncapped.
        if(position.x>=-30000&&position.y>=-30000)SetWindowPosition(int(position.x),int(position.y));
    }
    ~Window(){if(IsWindowReady())CloseWindow();}
};
GraphicsSession::GraphicsSession(bool benchmark,bool failure,bool probe):benchmark_(benchmark),simulateVulkanFailure_(failure),probeChild_(probe){}
GraphicsSession::~GraphicsSession(){destroy();}
void GraphicsSession::destroy(){panel_.reset();renderer_.reset();window_.reset();}
void GraphicsSession::create(GraphicsApi api,const PhysicsWorld& world,UiState& state,int width,int height,Vector2 position) {
    if(api==GraphicsApi::Vulkan&&simulateVulkanFailure_)throw std::runtime_error("Simulated Vulkan-unavailable test");
    window_=std::make_unique<Window>(runtime_,api,benchmark_,width,height,position,probeChild_);
    renderer_=std::make_unique<Renderer>();panel_=std::make_unique<Panel>();
    renderer_->sync(world,1);panel_->restoreLayout(state);
    state.graphicsApi=api;state.graphicsDriver=runtime_.driver();
}
bool GraphicsSession::switchTo(GraphicsApi api,const PhysicsWorld& world,UiState& state,bool strict) {
    if(window_&&state.graphicsApi==api)return true;
    if(window_&&(IsWindowFullscreen()||IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE))) {
        state.graphicsMessage="Press F11 to leave fullscreen before switching API.";return false;
    }
    const GraphicsApi previous=window_?state.graphicsApi:GraphicsApi::OpenGL;
    const int width=window_?GetScreenWidth():1440,height=window_?GetScreenHeight():900;
    const Vector2 position=window_?GetWindowPosition():Vector2{-40000,-40000};
    destroy();
    try{create(api,world,state,width,height,position);state.graphicsMessage.clear();return true;}
    catch(const std::exception& e) {
        const std::string reason=e.what();destroy();
        if(strict)throw;
        create(previous,world,state,width,height,position);
        state.graphicsMessage=std::string("Could not activate ")+apiName(api)+". Restored "+apiName(previous)+". "+reason;
        return false;
    }
}
}
