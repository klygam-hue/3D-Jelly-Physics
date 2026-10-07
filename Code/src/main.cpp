#include "jely/app/Application.hpp"
#include "jely/physics/SoftBody.hpp"
#include "EmbeddedFont.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

int main(int argc,char** argv) {
#ifdef _WIN32
    bool smokeRequest=false;for(int i=1;i<argc;i++)if(std::string_view(argv[i])=="--smoke")smokeRequest=true;
#endif
    try {
        jely::AppOptions options;
#ifdef _WIN32
        BOOL clientAnimation=TRUE;
        if(SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&clientAnimation,0)&&!clientAnimation)options.reducedMotion=true;
#endif
        for(int i=1;i<argc;i++) {
            std::string arg=argv[i];
            auto value=[&](){if(i+1>=argc)throw std::invalid_argument("Missing command line value");return std::string(argv[++i]);};
            if(arg=="--smoke") {options.smokeFrames=std::stoi(value());if(options.smokeFrames<2||options.smokeFrames>10000)throw std::invalid_argument("Smoke frames out of range");}
            else if(arg=="--screenshot") options.screenshot=value();
            else if(arg=="--report") options.report=value();
            else if(arg=="--automate") options.automation=true;
            else if(arg=="--benchmark")options.benchmark=true;
            else if(arg=="--no-sleep")options.noSleep=true;
            else if(arg=="--paused")options.paused=true;
            else if(arg=="--menu-hidden")options.menuHidden=true;
            else if(arg=="--reduced-motion")options.reducedMotion=true;
            else if(arg=="--solid-ui")options.solidUi=true;
            else if(arg=="--ui-automate")options.uiAutomation=true;
            else if(arg=="--ui-tab")options.uiTab=std::stoi(value());
            else if(arg=="--licenses"){std::cout<<jely::embeddedProjectLicense<<jely::embeddedFontLicense<<jely::GraphicsRuntime::licenses();return 0;}
            else if(arg=="--backend"){options.backend=jely::parseApi(value());options.backendExplicit=true;}
            else if(arg=="--strict-backend")options.strictBackend=true;
            else if(arg=="--simulate-vulkan-failure")options.simulateVulkanFailure=true;
            else if(arg=="--probe-vulkan-child")options.probeChild=true;
            else if(arg=="--switch-cycles")options.switchCycles=std::stoi(value());
            else if(arg=="--spawn")options.spawnCount=std::stoi(value());
            else if(arg=="--spawn-seed"){options.spawnSeed=std::stoull(value());options.spawnSeedExplicit=true;}
            else if(arg=="--spawn-automate")options.spawnAutomation=true;
            else if(arg=="--ground-automate")options.groundAutomation=true;
            else if(arg=="--softness-automate")options.softnessAutomation=true;
            else if(arg=="--softness")options.softness=std::stod(value());
            else if(arg=="--ground-width")options.appearance.groundWidth=std::stod(value());
            else if(arg=="--ground-depth")options.appearance.groundDepth=std::stod(value());
            else if(arg=="--ground-style"){auto style=value();if(style=="plain")options.appearance.grid=false;else if(style=="grid"){options.appearance.grid=true;options.appearance.groundPattern=0;}else if(style=="tiles"){options.appearance.grid=true;options.appearance.groundPattern=1;}else throw std::invalid_argument("Ground style must be plain/grid/tiles");}
            else if(arg=="--ground-roughness")options.appearance.groundRoughness=std::stod(value());
            else if(arg=="--ground-scale")options.appearance.groundPatternScale=std::stod(value());
            else if(arg=="--ground-strength")options.appearance.groundPatternStrength=std::stod(value());
            else if(arg=="--ground-ring-radius")options.appearance.groundRingRadius=std::stod(value());
            else if(arg=="--no-outline")options.appearance.groundOutline=false;
            else if(arg=="--time") options.appearance.timeOfDay=std::stod(value());
            else if(arg=="--theme") options.appearance.setTheme(std::stoi(value()));
            else if(arg=="--contrast") options.appearance.contrast=std::stod(value());
            else if(arg=="--transparency")options.appearance.transparency=std::stod(value());
            else if(arg=="--refraction")options.appearance.refraction=std::stod(value());
            else if(arg=="--gloss")options.appearance.gloss=std::stod(value());
            else if(arg=="--tint")options.appearance.tintStrength=std::stod(value());
            else if(arg=="--cycle-day")options.appearance.cycleTime=true;
            else if(arg=="--scene")options.preset=std::stoi(value());
            else if(arg=="--view")options.cameraView=std::stoi(value());
            else if(arg=="--no-grid") options.appearance.grid=false;
            else if(arg=="--no-ring") options.appearance.ring=false;
            else throw std::invalid_argument("Unknown argument: "+arg);
        }
        options.appearance.validate();
        jely::PhysicsSettings softnessCheck;softnessCheck.setSoftness(options.softness);
        if(options.benchmark&&options.smokeFrames<120)throw std::invalid_argument("Benchmark requires --smoke 120 or more");
        if(options.preset<0||options.preset>2)throw std::invalid_argument("Invalid scene preset");
        if(options.cameraView<0||options.cameraView>2)throw std::invalid_argument("Invalid camera view");
        if(options.uiTab<0||options.uiTab>4)throw std::invalid_argument("Invalid UI tab");
        if(options.switchCycles<0||options.switchCycles>20||(options.switchCycles>0&&options.switchCycles*120+20>options.smokeFrames))throw std::invalid_argument("Switch cycles need enough smoke frames (120 per cycle + 20)");
        if(options.switchCycles>0&&options.uiAutomation)throw std::invalid_argument("Run UI and API-switch automation separately");
        if(options.spawnCount<0||options.spawnCount>14)throw std::invalid_argument("Initial random spawn count must be 0..14");
        if(options.spawnAutomation&&(options.smokeFrames<120||options.uiAutomation||options.automation||options.switchCycles>0))throw std::invalid_argument("Spawn automation needs --smoke 120+ and no other input automation");
        if(options.groundAutomation&&(options.smokeFrames<350||options.uiAutomation||options.automation||options.spawnAutomation||options.switchCycles>0))throw std::invalid_argument("Ground automation needs --smoke 350+ and no other input automation");
        if(options.softnessAutomation&&(options.smokeFrames<350||options.uiAutomation||options.automation||options.spawnAutomation||options.groundAutomation||options.switchCycles>0))throw std::invalid_argument("Softness automation needs --smoke 350+ and no other input automation");
        if(options.probeChild&&(options.smokeFrames!=2||options.backend!=jely::GraphicsApi::Vulkan||!options.strictBackend))throw std::invalid_argument("Invalid internal driver probe arguments");
        return jely::Application(std::move(options)).run();
    } catch(const std::exception& e) {
        std::cerr<<"3D Jely Physiks: "<<e.what()<<'\n';
#ifdef _WIN32
        if(!smokeRequest)MessageBoxA(nullptr,e.what(),"3D Jely Physiks - startup error",MB_OK|MB_ICONERROR);
#endif
        return 1;
    }
}
