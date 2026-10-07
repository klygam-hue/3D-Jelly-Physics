#pragma once
#include <string>
#include "jely/scene/Appearance.hpp"
#include "jely/render/Backend.hpp"

namespace jely {
struct AppOptions {
    int smokeFrames=0;
    bool automation=false;
    bool benchmark=false;
    bool noSleep=false,paused=false;
    bool menuHidden=false,reducedMotion=false,solidUi=false,uiAutomation=false;
    int uiTab=2;
    GraphicsApi backend=GraphicsApi::OpenGL;
    bool backendExplicit=false,strictBackend=false,simulateVulkanFailure=false;
    bool probeChild=false;
    int switchCycles=0;
    int spawnCount=0;
    bool spawnAutomation=false;
    bool groundAutomation=false;
    bool softnessAutomation=false;
    double softness=50;
    std::uint64_t spawnSeed=0;
    bool spawnSeedExplicit=false;
    std::string screenshot;
    std::string report;
    Appearance appearance;
    int preset=0;
    int cameraView=0;
};
class Application {
public:
    explicit Application(AppOptions options):options_(std::move(options)) {}
    int run();
private:
    AppOptions options_;
};
}
