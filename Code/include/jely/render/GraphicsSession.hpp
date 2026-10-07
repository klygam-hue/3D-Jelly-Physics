#pragma once
#include "jely/render/Backend.hpp"
#include "jely/render/Renderer.hpp"
#include "jely/ui/Panel.hpp"
#include <memory>

namespace jely {
// GPU/context lifetime is separate from simulation, camera and user settings.
class GraphicsSession {
public:
    explicit GraphicsSession(bool benchmark,bool simulateVulkanFailure=false,bool probeChild=false);
    ~GraphicsSession();
    bool switchTo(GraphicsApi api,const PhysicsWorld& world,UiState& state,bool strict=false);
    GraphicsApi preference() const {return runtime_.loadPreference();}
    void savePreference(GraphicsApi api) const {runtime_.savePreference(api);}
    Renderer& renderer() {return *renderer_;}
    Panel& panel() {return *panel_;}
    bool vulkanDeviceConfirmed() const {return runtime_.vulkanDeviceConfirmed();}
    unsigned int errors() const {return runtime_.errors();}
private:
    struct Window;
    void destroy();
    void create(GraphicsApi api,const PhysicsWorld& world,UiState& state,int width,int height,Vector2 position);
    GraphicsRuntime runtime_;
    bool benchmark_=false,simulateVulkanFailure_=false;
    bool probeChild_=false;
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<Panel> panel_;
};
}
