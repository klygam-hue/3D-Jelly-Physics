#include "jely/app/Application.hpp"
#include "jely/physics/PhysicsWorld.hpp"
#include "jely/render/Renderer.hpp"
#include "jely/input/CameraController.hpp"
#include "jely/ui/Panel.hpp"
#include "jely/app/FrameProfiler.hpp"
#include "jely/app/PhysicsClock.hpp"
#include "jely/render/GraphicsSession.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace jely {
namespace {
struct DragPlane { Vector3 normal{}; float distance{}; bool active=false; };
}
int Application::run() {
    GraphicsSession graphics(options_.benchmark,options_.simulateVulkanFailure,options_.probeChild);
    FrameProfiler profiler(options_.benchmark?options_.smokeFrames:0);
    auto elapsedMs=[](auto begin){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();};
    CameraController camera; PhysicsWorld world;UiState state;
    state.appearance=options_.appearance;
    state.preset=options_.preset;world.reset(state.preset);
    if(!world.resizeGround(state.appearance.groundWidth,state.appearance.groundDepth))throw std::runtime_error("Initial ground too small for scene");
    if(options_.spawnSeedExplicit)world.seedSpawns(options_.spawnSeed);
    for(int i=0;i<options_.spawnCount;i++)if(!world.spawnRandom(state.resolution))throw std::runtime_error("Initial spawn budget exceeded");
    world.settings.setSoftness(options_.softness);
    world.settings.sleepEnabled=!options_.noSleep;state.paused=options_.paused;
    state.hidden=options_.menuHidden;state.solidUi=options_.solidUi;state.reducedMotion=options_.reducedMotion;
    state.tab=static_cast<PanelTab>(options_.uiTab);
    if(options_.softnessAutomation){state.tab=PanelTab::Simulation;state.reducedMotion=true;world.settings.gravity=0;state.paused=false;}
    if(options_.switchCycles>0)state.tab=PanelTab::Graphics;
    const auto initialApi=options_.backendExplicit?options_.backend:(options_.smokeFrames>0?GraphicsApi::OpenGL:graphics.preference());
    int apiFallbacks=graphics.switchTo(initialApi,world,state,options_.strictBackend)?0:1;
    const double initialCompliance=world.settings.edgeCompliance;
    if(options_.cameraView!=0){
        float yaw=options_.cameraView==1?1.5707963f:-1.5707963f;
        camera.apply({{(0.65f-yaw)/0.006f,0},{},0,{}},0);
    }
    DragPlane plane;PhysicsClock physicsClock;double meanMs=0;int frames=0,pickChecks=0,cameraChecks=0;bool captured=false;
    int uiChecks=0;std::string uiFailure;double nextMonitorCheck=0;
    int spawnChecks=0,spawned=options_.spawnCount;
    int groundChecks=0;
    int softnessChecks=0;double dragPeak=0;Vec3 softnessAnchor{};
    const auto groundInitialSettings=world.settings;
    std::vector<Vec3> groundInitialPose;
    if(options_.groundAutomation){state.paused=true;for(const auto& body:world.bodies())for(const auto& n:body.nodes())groundInitialPose.push_back(n.position);}
    std::size_t spawnBaseline=world.bodies().size();
    std::vector<Vec3> spawnSavedPose;
    if(options_.spawnAutomation){state.paused=true;for(const auto& body:world.bodies())for(const auto& node:body.nodes())spawnSavedPose.push_back(node.position);}
    int apiSwitches=0,apiStateChecks=0;unsigned int apiErrors=0;bool skipDelta=false;
    while(!WindowShouldClose()&&(options_.smokeFrames==0||frames<options_.smokeFrames)) {
        if(IsWindowMinimized()) {PollInputEvents();WaitTime(0.01);skipDelta=true;continue;}
        if(state.graphicsSwitchRequested) {
            apiErrors+=graphics.errors();
            const auto savedSettings=world.settings;const auto savedAppearance=state.appearance;
            const auto savedSteps=world.stepCount();const auto savedCamera=camera.camera();
            std::vector<Vec3> pose;for(const auto& body:world.bodies())for(const auto& node:body.nodes()){pose.push_back(node.position);pose.push_back(node.velocity);}
            if(plane.active){world.releaseGrab();plane.active=false;}
            bool changed=graphics.switchTo(state.requestedApi,world,state,options_.strictBackend);
            if(changed) {
                ++apiSwitches;
                if(options_.smokeFrames==0)try{graphics.savePreference(state.graphicsApi);}catch(const std::exception& e){state.graphicsMessage=std::string("API active, but preference was not saved: ")+e.what();}
            }else ++apiFallbacks;
            std::size_t index=0;bool same=world.settings==savedSettings&&state.appearance==savedAppearance&&world.stepCount()==savedSteps;
            for(const auto& body:world.bodies())for(const auto& node:body.nodes())for(auto value:{node.position,node.velocity}){same=same&&(value-pose[index++]).lengthSquared()==0;}
            auto currentCamera=camera.camera();same=same&&Vector3Distance(currentCamera.position,savedCamera.position)==0&&Vector3Distance(currentCamera.target,savedCamera.target)==0;
            if(!same)uiFailure="Graphics API switch changed simulation/settings/camera";else ++apiStateChecks;
            state.graphicsSwitchRequested=false;skipDelta=true;nextMonitorCheck=0;
        }
        auto& renderer=graphics.renderer();auto& panel=graphics.panel();
        if(GetTime()>=nextMonitorCheck){
            const int monitor=GetCurrentMonitor(),count=GetMonitorCount();
            const int reported=monitor>=0&&monitor<count?GetMonitorRefreshRate(monitor):0;
            const int hz=reported>0&&reported<=1000?reported:0;
            state.monitorIndex=monitor;state.monitorHz=hz;
            // Context recreation resets raylib's limiter even on the same monitor.
            if(!options_.benchmark)SetTargetFPS(hz);
            state.monitorName=monitor>=0&&monitor<count?GetMonitorName(monitor):"Unknown monitor";
            nextMonitorCheck=GetTime()+0.5;
        }
        auto frameStart=options_.benchmark?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
        double frameDt=double(GetFrameTime());
        if(skipDelta){frameDt=0;skipDelta=false;}
        double dt=std::clamp(frameDt,0.0,0.1);
        if(IsKeyPressed(KEY_TAB)) state.hidden=!state.hidden;
        if(IsKeyPressed(KEY_SPACE)) state.paused=!state.paused;
        if(IsKeyPressed(KEY_R)) state.resetRequested=true;
        if(IsKeyPressed(KEY_J)) state.impulseRequested=true;
        if(IsKeyPressed(KEY_B))state.spawnRequested=true;
        if(IsKeyPressed(KEY_F1)) state.debug=!state.debug;
        if(IsKeyPressed(KEY_N)&&state.paused) state.stepRequested=true;
        if(IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if(options_.automation) {
            if(frames==0) {
                Vector3 initial=camera.camera().position;
                auto check=[&](bool condition){if(!condition)throw std::runtime_error("Camera smoke check failed");++cameraChecks;};
                camera.apply({{50,-20},{},0,{}},0.016f);
                check(Vector3Distance(initial,camera.camera().position)>0.1f);
                Vector3 previousTarget=camera.camera().target;
                camera.apply({{},{10,5},0,{}},0.016f);
                check(Vector3Distance(previousTarget,camera.camera().target)>0.01f);
                float previousDistance=Vector3Distance(camera.camera().target,camera.camera().position);
                camera.apply({{},{},2,{}},0.016f);
                check(Vector3Distance(camera.camera().target,camera.camera().position)<previousDistance);
                previousTarget=camera.camera().target;
                camera.apply({{},{},0,{1,1}},0.016f);
                check(Vector3Distance(previousTarget,camera.camera().target)>0.01f&&physics(camera.camera().position).finite());
                camera.reset();
                check(Vector3Distance(initial,camera.camera().position)<0.00001f);
                Vector2 screen=GetWorldToScreen(jely::graphics(world.bodies()[0].center()),camera.camera());
                auto hit=renderer.pick(0,GetScreenToWorldRay(screen,camera.camera()));
                if(!hit.hit) throw std::runtime_error("Surface ray picking smoke check failed");
                ++pickChecks;
            }
            if(frames==35) state.impulseRequested=true;
            if(frames==20)state.appearance.transparency=0;
            if(frames==30)state.appearance.transparency=65;
            if(frames==50) {state.appearance.setMaterialPreset(1);state.appearance.timeOfDay=0;}
            if(frames==60) {state.appearance.setMaterialPreset(2);state.appearance.transparency=100;}
            if(frames==65) {state.appearance.setMaterialPreset(0);state.appearance.timeOfDay=6.4;state.appearance.transparency=65;}
            if(frames==70) {world.setGrab(0,world.bodies()[0].index(4,4,2),{0.65,2.5,0});}
            if(frames==90) world.releaseGrab();
            if(frames==100)state.appearance.cycleTime=true;
            if(frames==110) state.paused=true;
            if(frames==112) state.stepRequested=true;
            if(frames==115) state.paused=false;
            if(frames==125) {state.preset=1;state.resetRequested=true;state.appearance.setTheme(1);state.appearance.timeOfDay=17.6;}
            if(frames==160) {state.preset=2;state.resetRequested=true;state.appearance.setTheme(2);state.appearance.grid=false;state.appearance.ring=false;}
            if(frames==200) state.debug=true;
            if(frames==210) {state.debug=false;state.appearance=Appearance{};}
            if(frames==220)SetWindowSize(1280,960);
            if(frames==230)SetWindowSize(1440,900);
        }
        Panel::Input uiInput{GetMousePosition(),IsMouseButtonPressed(MOUSE_BUTTON_LEFT),IsMouseButtonDown(MOUSE_BUTTON_LEFT)};
        if(options_.softnessAutomation){
            uiInput={{600,100},false,false};
            auto check=[&](bool condition,const char* message){if(condition)++softnessChecks;else if(uiFailure.empty())uiFailure=std::string("Softness UI: ")+message;};
            if(frames==10)uiInput={{38,342},true,true};
            if(frames==11)check(world.settings.rigid(),"zero slider is not stone");
            if(frames==40)uiInput={{282,342},true,true};
            if(frames==41)check(world.settings.edgeCompliance==0.2,"maximum slider not soft");
            if(frames==60)uiInput={{160,342},true,true};
            if(frames==61)check(world.settings.edgeCompliance==0.000025,"middle slider default mismatch");
            if(frames==70)uiInput={{282,342},true,true};
            if(frames==72){auto id=world.bodies()[0].index(4,2,2);softnessAnchor=world.bodies()[0].nodes()[id].position;world.setGrab(0,id,softnessAnchor);check(world.grab()->count>1,"point grab");}
            if(frames>=73&&frames<162){world.moveGrab(softnessAnchor+Vec3{(frames-72)*0.03,0,0});for(const auto& e:world.bodies()[0].edges())dragPeak=std::max(dragPeak,std::abs((world.bodies()[0].nodes()[e.a].position-world.bodies()[0].nodes()[e.b].position).length()/e.rest-1));}
            if(frames==162){check(world.bodies()[0].center().x>2,"whole body did not follow");check(dragPeak>0.15,"no elastic stretch");world.releaseGrab();}
            if(frames==180){state.preset=1;state.resetRequested=true;world.settings.gravity=9.81;}
            if(frames==340){auto stats=world.bodies()[0].stats();check(stats.finite&&stats.minTetRatio>=0.099,"unstable soft drop");check(std::abs(stats.volumeRatio-1)<0.03,"soft drop volume drift");check(state.monitorHz>=0&&state.monitorHz<=1000,"invalid monitor refresh");}
        }
        if(options_.uiAutomation) {
            uiInput={{600,100},false,false};
            auto click=[&](float x,float y){uiInput={{x,y},true,true};};
            auto check=[&](bool passed,const char* description){if(passed)++uiChecks;else if(uiFailure.empty())uiFailure=std::string("UI automation: ")+description;};
            switch(frames) {
            case 10:click(158,128);break;
            case 11:check(state.tab==PanelTab::Appearance,"Appearance tab");break;
            case 35:click(60,280);break;
            case 36:check(state.appearance.timeOfDay==0,"Night preset");break;
            case 55:click(205,128);break;
            case 56:check(state.tab==PanelTab::Jelly,"Jelly tab");break;
            case 75:click(282,284);break;
            case 76:check(state.appearance.transparency==100,"Invisible slider endpoint");break;
            case 85:click(38,284);break;
            case 86:check(state.appearance.transparency==0,"Opaque slider endpoint");break;
            case 95:click(196.6f,284);break;
            case 96:check(std::abs(state.appearance.transparency-65)<0.01,"Transparency slider restore");break;
            case 110:click(49,51);break;
            case 165:check(state.hidden&&panel.openAmount()<0.01,"Animated collapse");break;
            case 170:click(49,51);break;
            case 220:check(!state.hidden&&panel.openAmount()>0.99,"Animated reopen");break;
            case 230:click(49,51);break;
            case 240:click(49,51);break;
            case 265:check(!state.hidden&&panel.openAmount()>0.85,"Mid-animation reversal");click(67+MenuLayout(panel.openAmount()).offset(),128);break;
            case 266:check(state.tab==PanelTab::Simulation,"Physics tab");check(world.settings.edgeCompliance==initialCompliance,"Tab changed physical softness");break;
            case 290:click(220,588);break;
            case 292:check(state.resolution==7&&world.bodies()[0].resolution()==7,"Detailed topology reset");break;
            case 310:click(90,588);break;
            case 312:check(state.resolution==5&&world.bodies()[0].resolution()==5,"Balanced topology reset");break;
            case 330:click(90,float(GetScreenHeight())-43);break;
            case 331:check(state.reducedMotion,"Reduce motion toggle");break;
            case 340:click(49,51);break;
            case 345:click(49,51);break;
            case 346:check(!state.hidden&&panel.openAmount()==1,"Reduced-motion reopen");break;
            case 355:click(240,float(GetScreenHeight())-43);break;
            case 356:check(state.solidUi,"Solid glass toggle");break;
            case 365:click(240,float(GetScreenHeight())-43);break;
            case 366:check(!state.solidUi,"Glass restore");break;
            case 375:click(90,float(GetScreenHeight())-43);break;
            case 376:check(!state.reducedMotion,"Animation restore");break;
            case 385:click(205,128);break;
            case 400:SetWindowSize(1280,960);break;
            case 420:SetWindowSize(1440,900);break;
            default:break;
            }
        }
        if(options_.switchCycles>0) {
            uiInput={{600,100},false,false};
            if(frames<options_.switchCycles*120) {
                if(frames%120==20)uiInput={{220,222},true,true};
                if(frames%120==80)uiInput={{90,222},true,true};
            }
        }
        if(options_.spawnAutomation){
            uiInput={{600,100},false,false};
            if(frames==10||frames==30||frames==70)uiInput={{float(GetScreenWidth())*0.5f,47},true,true};
            if(frames==50)state.hidden=true;
            if(frames==100){
                bool same=world.bodies().size()==spawnBaseline+3;std::size_t i=0;
                for(std::size_t b=0;b<spawnBaseline;b++)for(const auto& n:world.bodies()[b].nodes())same=same&&(n.position-spawnSavedPose[i++]).lengthSquared()==0;
                if(!same)uiFailure="Spawn UI changed existing bodies or failed additive spawning";else ++spawnChecks;
                for(std::size_t b=spawnBaseline;b<world.bodies().size();b++){
                    const auto& body=world.bodies()[b];
                    if(body.customColor()&&body.stats().finite)++spawnChecks;else uiFailure="Spawned shape/color invalid";
                    const auto screen=GetWorldToScreen(jely::graphics(body.center()),camera.camera());
                    if(renderer.pick(b,GetScreenToWorldRay(screen,camera.camera())).hit)++spawnChecks;else uiFailure="Spawned shape cannot be picked";
                }
                state.paused=false;
            }
        }
        if(options_.groundAutomation){
            uiInput={{600,100},false,false};
            auto click=[&](float x,float y){uiInput={{x,y},true,true};};
            auto check=[&](bool ok,const char* reason){if(ok)++groundChecks;else uiFailure=std::string("Ground automation: ")+reason;};
            switch(frames){
            case 0:click(283,47);break;
            case 1:check(state.tab==PanelTab::Ground,"navigation");break;
            case 20:click(282,269);break;
            case 22:check(state.appearance.groundWidth==32&&world.scene().halfExtent==16,"max width / physics");break;
            case 40:click(38,269);break;
            case 42:check(state.appearance.groundWidth==6&&world.scene().halfExtent==3,"min width / physics");break;
            case 60:click(282,319);break;
            case 62:check(state.appearance.groundDepth==32&&world.scene().halfDepth==16,"depth / physics");break;
            case 80:click(244,208);break;
            case 82:check(state.appearance.grid&&state.appearance.groundPattern==1,"tiles");break;
            case 100:click(38,369);break;
            case 102:check(state.appearance.groundRoughness==0,"roughness");break;
            case 120:click(282,419);break;
            case 122:check(state.appearance.groundPatternScale==4,"pattern scale");break;
            case 140:click(38,469);break;
            case 142:check(state.appearance.groundPatternStrength==0,"pattern strength");break;
            case 160:click(160,539);break;
            case 162:check(std::abs(state.appearance.platform.hue-180)<1e-8,"hue");break;
            case 180:click(282,587);break;
            case 182:check(state.appearance.platform.saturation==100,"saturation");break;
            case 200:click(282,635);break;
            case 202:check(state.appearance.platform.brightness==100,"brightness");break;
            case 220:click(90,675);break;
            case 222:check(!state.appearance.ring,"ring toggle");break;
            case 240:click(220,675);break;
            case 242:check(!state.appearance.groundOutline,"outline toggle");break;
            case 260:click(70,208);break;
            case 262:check(!state.appearance.grid,"plain");break;
            case 280:click(150,788);break;
            case 282:check(state.appearance.groundWidth==16&&state.appearance.groundDepth==16&&state.appearance.groundRoughness==0.6&&world.scene().halfDepth==8&&world.scene().halfExtent==8,"ground reset");break;
            case 300:{bool same=world.settings==groundInitialSettings&&world.stepCount()==0;std::size_t i=0;for(const auto& body:world.bodies())for(const auto& n:body.nodes())same=same&&(n.position-groundInitialPose[i++]).lengthSquared()==0;check(same,"reset altered physics/material state");break;}
            case 320:state.hidden=true;break;
            case 335:click(283,47);break;
            case 337:check(!state.hidden&&state.tab==PanelTab::Ground,"closed-menu navigation");break;
            default:break;
            }
        }
        panel.update(state,options_.smokeFrames>0?1.0/120.0:dt,uiInput);
        if(options_.uiAutomation) {
            if(frames==165) {
                panel.update(state,0,{{49,51},false,false});if(panel.containsMouse(state))++uiChecks;else uiFailure="UI automation: launcher input capture";
                panel.update(state,0,{{100,400},false,false});if(!panel.containsMouse(state))++uiChecks;else uiFailure="UI automation: hidden menu input release";
            }
            if(frames==340){if(state.hidden&&panel.openAmount()==0)++uiChecks;else uiFailure="UI automation: reduced-motion collapse";}
        }
        if(state.resetRequested) {
            world.reset(state.preset,state.resolution);world.settings.validate();
            physicsClock.reset();state.resetRequested=false;state.error.clear();plane.active=false;
            renderer.sync(world,1);
        }
        if(state.impulseRequested) {world.impulse({1.0,5.4,-0.4});state.impulseRequested=false;}
        if(state.appearance.groundWidth!=world.scene().halfExtent*2||state.appearance.groundDepth!=world.scene().halfDepth*2){
            if(world.resizeGround(state.appearance.groundWidth,state.appearance.groundDepth)){plane.active=false;state.groundMessage.clear();}
            else {state.appearance.groundWidth=world.scene().halfExtent*2;state.appearance.groundDepth=world.scene().halfDepth*2;state.groundMessage="Ground too small for the current bodies";}
        }
        if(state.spawnRequested){
            state.spawnRequested=false;
            try{if(world.spawnRandom(state.resolution)){++spawned;state.spawnMessage.clear();}else state.spawnMessage="Limit: 16 jellies. Reset to clear.";}
            catch(const std::exception& e){state.spawnMessage=e.what();}
        }
        if(!state.paused)state.appearance.advanceTime(options_.smokeFrames>0?1.0/120.0:dt);
        camera.update(panel.containsMouse(state)||plane.active);
        Ray ray=GetScreenToWorldRay(GetMousePosition(),camera.camera());
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&!panel.containsMouse(state)) {
            float closest=std::numeric_limits<float>::max();std::size_t selected=0;RayCollision hit{};
            for(std::size_t i=0;i<world.bodies().size();i++) {auto collision=renderer.pick(i,ray);if(collision.hit&&collision.distance<closest){closest=collision.distance;selected=i;hit=collision;}}
            if(hit.hit) {
                double nearest=1e10;std::size_t node=0;
                const auto& nodes=world.bodies()[selected].nodes();
                for(std::size_t i=0;i<nodes.size();i++) if(nodes[i].surface) {double d=(nodes[i].position-physics(hit.point)).lengthSquared();if(d<nearest){nearest=d;node=i;}}
                Vector3 anchor=jely::graphics(nodes[node].position);
                world.setGrab(selected,node,physics(anchor));
                plane.normal=Vector3Normalize(Vector3Subtract(camera.camera().target,camera.camera().position));
                plane.distance=Vector3DotProduct(anchor,plane.normal);plane.active=true;
            }
        }
        if(plane.active&&IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float denominator=Vector3DotProduct(ray.direction,plane.normal);
            if(std::abs(denominator)>0.001f) {
                float t=(plane.distance-Vector3DotProduct(ray.position,plane.normal))/denominator;
                if(t>0) world.moveGrab(physics(Vector3Add(ray.position,Vector3Scale(ray.direction,t))));
            }
        }
        if(plane.active&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {world.releaseGrab();plane.active=false;}
        auto before=std::chrono::steady_clock::now();int steps=0;
        try {
            if(!state.paused) {
                steps=physicsClock.advance(world,options_.smokeFrames>0?PhysicsWorld::fixedStep*2:frameDt);
                state.lostTime=physicsClock.lostTime();
            } else physicsClock.reset();
            if(state.stepRequested&&state.paused) {world.step();++steps;state.stepRequested=false;}
        } catch(const std::exception& e) {state.paused=true;state.error=e.what();world.releaseGrab();plane.active=false;}
        double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-before).count();
        if(steps>0) meanMs=meanMs*0.92+(ms/steps)*0.08;
        state.physicsMs=meanMs;
        double alpha=state.paused?1.0:physicsClock.alpha();
        auto meshStart=options_.benchmark?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
        renderer.sync(world,alpha);
        double meshMs=options_.benchmark?elapsedMs(meshStart):0;
        auto drawStart=options_.benchmark?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
        BeginDrawing();ClearBackground({12,19,29,255});
        renderer.draw(world,camera.camera(),state.debug,state.appearance);
        panel.prepareBackdrop(renderer.uiBackdrop(),renderer.sceneRevision(),state);panel.draw(world,state);
        double drawMs=options_.benchmark?elapsedMs(drawStart):0;
        // Capture the drawn frame before presentation; EGL/Vulkan may discard the back buffer.
        if(!options_.screenshot.empty()&&options_.smokeFrames>0&&frames==options_.smokeFrames-2) {
            rlDrawRenderBatchActive();Image frame=LoadImageFromScreen();
            captured=frame.data!=nullptr&&ExportImage(frame,options_.screenshot.c_str());UnloadImage(frame);
        }
        EndDrawing();++frames;
        if(options_.smokeFrames>0&&!options_.benchmark)rlCheckErrors();
        if(options_.benchmark)profiler.record(frames,ms,meshMs,drawMs,elapsedMs(frameStart));
    }
    if(!uiFailure.empty())state.error=uiFailure;
    apiErrors+=graphics.errors();
    if(!options_.report.empty()) {
        std::ofstream report(options_.report); if(!report) throw std::runtime_error("Cannot write smoke report");
        report<<"frames="<<frames<<"\nphysics_steps="<<world.stepCount()<<"\nsolver_ms="<<meanMs<<"\nbodies="<<world.bodies().size()<<"\nscreenshot="<<captured<<"\ncamera_checks="<<cameraChecks<<"\npicking_checks="<<pickChecks<<"\nerror="<<state.error<<'\n';
        report<<"time_of_day="<<state.appearance.timeOfDay<<"\ncontrast="<<state.appearance.contrast<<"\nobstacles="<<world.scene().spheres.size()+world.scene().boxes.size()<<'\n';
        report<<"transparency="<<state.appearance.transparency<<"\nrefraction="<<state.appearance.refraction<<"\ngloss="<<state.appearance.gloss<<"\nrender_target_generations="<<graphics.renderer().targetGeneration()<<'\n';
        report<<"sleeping="<<world.sleeping()<<"\n";
        report<<"ui_checks="<<uiChecks<<"\nmenu_hidden="<<state.hidden<<"\nmenu_open="<<graphics.panel().openAmount()<<"\nui_tab="<<static_cast<int>(state.tab)<<"\nreduced_motion="<<state.reducedMotion<<"\nsolid_ui="<<state.solidUi<<"\nglass_generations="<<graphics.panel().glassGeneration()<<"\nblur_updates="<<graphics.panel().blurUpdates()<<'\n';
        report<<"graphics_api="<<apiName(state.graphicsApi)<<"\ngraphics_driver="<<state.graphicsDriver<<"\nvulkan_device_confirmed="<<graphics.vulkanDeviceConfirmed()<<"\napi_switches="<<apiSwitches<<"\napi_state_checks="<<apiStateChecks<<"\napi_fallbacks="<<apiFallbacks<<"\ngraphics_message="<<state.graphicsMessage<<'\n';
        report<<"graphics_errors="<<apiErrors<<'\n';
        report<<"monitor_hz="<<state.monitorHz<<"\nmonitor_index="<<state.monitorIndex<<"\nmonitor_name="<<state.monitorName<<"\nrender_fps_limit="<<(options_.benchmark?0:state.monitorHz)<<"\nphysics_hz="<<1.0/PhysicsWorld::fixedStep<<"\nsoftness="<<world.settings.softness()<<"\nedge_compliance="<<world.settings.edgeCompliance<<'\n';
        report<<"spawned="<<spawned<<"\nspawn_checks="<<spawnChecks<<"\nspawn_message="<<state.spawnMessage<<'\n';
        report<<"softness_checks="<<softnessChecks<<"\nsoftness_drag_peak="<<dragPeak<<'\n';
        report<<"ground_checks="<<groundChecks<<"\nground_width="<<state.appearance.groundWidth<<"\nground_depth="<<state.appearance.groundDepth<<"\nphysics_half_width="<<world.scene().halfExtent<<"\nphysics_half_depth="<<world.scene().halfDepth<<"\nground_pattern="<<state.appearance.groundPattern<<"\nground_roughness="<<state.appearance.groundRoughness<<"\nground_message="<<state.groundMessage<<'\n';
        if(options_.benchmark)profiler.report(report);
        for(const auto& body:world.bodies()) {auto s=body.stats();report<<"finite="<<s.finite<<" volume="<<s.volumeRatio<<" min_tet="<<s.minTetRatio<<" max_speed="<<s.maxSpeed<<" shape="<<int(body.shape().kind);if(body.customColor()){auto c=*body.customColor();report<<" color="<<c.x<<','<<c.y<<','<<c.z;}report<<'\n';}
    }
    if(options_.smokeFrames>0&&(frames!=options_.smokeFrames||(!options_.screenshot.empty()&&!captured))) return 3;
    return !state.error.empty()?2:apiErrors?4:0;
}
}
