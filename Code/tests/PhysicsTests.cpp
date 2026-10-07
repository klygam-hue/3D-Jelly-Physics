#include "jely/physics/PhysicsWorld.hpp"
#include "jely/render/Surface.hpp"
#include "jely/scene/Appearance.hpp"
#include "jely/ui/Motion.hpp"
#include "jely/render/Backend.hpp"
#include "jely/math/RigidFit.hpp"
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using namespace jely;
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
void simulate(PhysicsWorld& world,int steps) { for(int i=0;i<steps;i++)world.step(); }
double maximumStrain(const SoftBody& body) {
    double result=0;for(const auto& e:body.edges())result=std::max(result,std::abs((body.nodes()[e.a].position-body.nodes()[e.b].position).length()/e.rest-1));return result;
}
double height(const SoftBody& body) {
    double low=1e9,high=-1e9;for(const auto& n:body.nodes()){low=std::min(low,n.position.y);high=std::max(high,n.position.y);}return high-low;
}
void healthy(const PhysicsWorld& world,double volumeTolerance=0.18) {
    for(const auto& body:world.bodies()) {
        auto s=body.stats();
        require(s.finite,"non-finite state");
        require(std::abs(s.volumeRatio-1)<volumeTolerance,"volume drift");
        require(s.minTetRatio>0.05,"tetrahedron inversion/collapse");
        require(s.maxSpeed<=35.0001,"velocity guard failed");
        for(const auto& n:body.nodes()) {
            require(n.position.y>=0.034999,"floor penetration");
            require(std::abs(n.position.x)<=7.96501&&std::abs(n.position.z)<=7.96501,"wall penetration");
        }
    }
}
}
int main(int argc,char** argv) {
    using namespace jely;
    int failed=0;
    int selected=0;
    const std::string filter=argc>1?argv[1]:"";
    auto test=[&](const char* name,const std::function<void()>& run) {
        if(!filter.empty()&&std::string(name).find(filter)==std::string::npos)return;
        ++selected;
        auto start=std::chrono::steady_clock::now();
        try {run();std::cout<<"PASS ";} catch(const std::exception& e) {++failed;std::cout<<"FAIL "<<e.what()<<" ";}
        std::cout<<name<<" ("<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<" ms)\n";
    };
    test("topology, mass and positive rest volume",[]{
        for(int r:{3,5,7,10}) {
            SoftBody body({0,4,0},1.8,r,1.6);
            require(body.nodes().size()==static_cast<std::size_t>(r*r*r),"node count");
            require(body.tetrahedra().size()==static_cast<std::size_t>(6*(r-1)*(r-1)*(r-1)),"tet count");
            double mass=0;for(const auto& n:body.nodes())mass+=1/n.inverseMass;
            require(std::abs(mass-1.6)<1e-10,"mass lumping");
            require(std::abs(body.stats().volumeRatio-1)<1e-10,"rest volume");
            require(body.stats().minTetRatio>0.99999,"rest orientation");
        }
    });
    test("translation invariance and zero-gravity rest",[]{
        SoftBody body({0,4,0});PhysicsSettings settings;settings.gravity=0;
        Vec3 displacement{1.2,-0.6,2.1};
        for(auto& n:body.nodes()) {n.position+=displacement;n.previous+=displacement;n.framePrevious+=displacement;}
        Vec3 before=body.center();
        for(int frame=0;frame<60;frame++){body.integrate(1.0/360,settings);for(int i=0;i<8;i++)body.solve(1.0/360,settings);body.finish(1.0/360,settings);}
        require((body.center()-before).length()<1e-9,"rest body moves");
        require(body.stats().maxSpeed<1e-8,"rest velocity");
    });
    test("free fall follows gravity",[]{
        PhysicsWorld world;Vec3 before=world.bodies()[0].center();simulate(world,12);
        double fall=before.y-world.bodies()[0].center().y;
        require(fall>0.045&&fall<0.053,"free fall displacement");
        require(world.bodies()[0].stats().volumeRatio>0.9999,"free fall deformation");
    });
    test("20 second drop and stable resting contact",[]{
        PhysicsWorld world;world.settings.sleepEnabled=false;simulate(world,2400);healthy(world,0.08);
        auto s=world.bodies()[0].stats();std::cout<<"[volume="<<s.volumeRatio<<" speed="<<s.maxSpeed<<"] ";
        require(s.maxSpeed<0.45,"resting jitter");
        require(world.bodies()[0].center().y<1.2,"body did not settle");
    });
    test("impulse, elastic recovery and walls",[]{
        PhysicsWorld world;simulate(world,500);world.impulse({25,7,12});simulate(world,1800);healthy(world,0.12);
        require(world.bodies()[0].stats().maxSpeed<0.6,"impulse failed to settle");
    });
    test("sphere and box obstacles",[]{
        PhysicsWorld world;world.reset(1);world.scene().spheres.push_back({{0,0.8,0},1.25});simulate(world,1200);healthy(world);
        const auto& sphere=world.scene().spheres.front();
        for(const auto& n:world.bodies()[0].nodes()) require((n.position-sphere.center).length()>=sphere.radius+0.03499,"sphere penetration");
        world.reset();
        world.scene().boxes.push_back({{-3.2,0.55,-0.5},{0.85,0.55,1.15}});
        for(auto& n:world.bodies()[0].nodes()){n.position.x-=3.2;n.previous=n.framePrevious=n.position;}
        simulate(world,900);healthy(world);
        const auto& box=world.scene().boxes.front();
        for(const auto& n:world.bodies()[0].nodes()) {
            Vec3 p=n.position-box.center,e=box.halfSize;
            require(std::abs(p.x)>=e.x+0.03499||std::abs(p.y)>=e.y+0.03499||std::abs(p.z)>=e.z+0.03499,"box penetration");
        }
    });
    test("drag stretch, release and recover",[]{
        PhysicsWorld world;simulate(world,480);auto id=world.bodies()[0].index(4,4,2);
        world.setGrab(0,id,world.bodies()[0].nodes()[id].position);
        for(int i=0;i<360;i++) {world.moveGrab({1.0,2.6,0});world.step();}
        healthy(world,0.25);require(world.bodies()[0].nodes()[id].position.y>2.0,"grab does not lift");
        world.releaseGrab();simulate(world,1600);healthy(world,0.12);
        require(world.bodies()[0].stats().maxSpeed<0.6,"release failed to settle");
    });
    test("two-body contact and volume",[]{
        PhysicsWorld world;world.reset(2);simulate(world,1800);healthy(world,0.15);
        require((world.bodies()[0].center()-world.bodies()[1].center()).length()>1.0,"bodies interpenetrated");
    });
    test("soft and detailed quality configurations",[]{
        for(int resolution:{3,7}) {
            PhysicsWorld world;world.reset(0,resolution);world.settings.edgeCompliance=0.00025;world.settings.iterations=10;
            simulate(world,800);healthy(world,0.25);
        }
    });
    test("deterministic replay",[]{
        PhysicsWorld a,b;a.impulse({1,0,-1});b.impulse({1,0,-1});simulate(a,360);simulate(b,360);
        for(std::size_t i=0;i<a.bodies()[0].nodes().size();i++) require((a.bodies()[0].nodes()[i].position-b.bodies()[0].nodes()[i].position).lengthSquared()==0,"nondeterministic state");
    });
    test("welded manifold surface, binding and smooth normals",[]{
        SoftBody body({0,4,0});Surface surface;surface.update(body,1);
        require(surface.positions().size()==3458,"surface vertex count");
        require(surface.indices().size()/3==6912,"surface triangle count");
        std::map<std::pair<unsigned short,unsigned short>,int> edges;
        const auto& indices=surface.indices();
        for(std::size_t i=0;i<indices.size();i+=3)for(int k=0;k<3;k++){auto a=indices[i+k],b=indices[i+(k+1)%3];++edges[{std::min(a,b),std::max(a,b)}];}
        for(auto [edge,count]:edges){(void)edge;require(count==2,"nonmanifold mesh");}
        for(std::size_t i=0;i<surface.positions().size();i++) {
            require(surface.positions()[i].finite()&&surface.normals()[i].finite(),"surface NaN");
            require(std::abs(surface.normals()[i].length()-1)<1e-8,"normal not normalized");
            require(surface.normals()[i].dot(surface.positions()[i]-body.center())>0,"inward normal");
        }
        Vec3 before=surface.positions()[100];for(auto& n:body.nodes()){n.framePrevious=n.position;n.position+=Vec3{2,0,0};}
        surface.update(body,0.5);require((surface.positions()[100]-before-Vec3{1,0,0}).length()<1e-9,"render interpolation");
    });
    test("empty demo presets and high drop",[]{
        PhysicsWorld world;
        for(int i=0;i<3;i++){world.reset(i);require(world.scene().spheres.empty()&&world.scene().boxes.empty(),"demo has obstacles");}
        world.reset(1);require(world.bodies()[0].center().y>6,"high drop height");simulate(world,900);healthy(world);
    });
    test("appearance day/night, HSV and independent colors",[]{
        Appearance a;auto day=a.lighting();a.timeOfDay=0;auto night=a.lighting();
        require(day.keyIntensity>night.keyIntensity*3,"day/night has no lighting difference");
        require((day.direction-night.direction).length()>0.3,"light direction unchanged");
        for(double hour:{0.0,6.0,6.4,12.0,17.6,18.0,24.0}){a.timeOfDay=hour;auto l=a.lighting();require(l.direction.finite()&&l.direction.y>0.1&&std::abs(l.direction.length()-1)<1e-10,"invalid day/night direction");}
        for(double horizon:{6.0,18.0}){a.timeOfDay=horizon-0.001;auto before=a.lighting();a.timeOfDay=horizon+0.001;require((before.direction-a.lighting().direction).length()<0.01,"shadow direction jumps at horizon");}
        require((HsvColor{0,100,100}.rgb()-Vec3{1,0,0}).length()<1e-10,"HSV red");
        require((HsvColor{120,100,100}.rgb()-Vec3{0,1,0}).length()<1e-10,"HSV green");
        require((HsvColor{210,100,100}.rgb()-Vec3{0,0.5,1}).length()<1e-10,"HSV blue transition");
        require((HsvColor{360,100,100}.rgb()-HsvColor{0,100,100}.rgb()).length()<1e-10,"hue wrapping");
        Vec3 platform=a.platform.rgb();a.background={45,90,50};require((platform-a.platform.rgb()).lengthSquared()==0,"background changes platform");
        for(int theme=0;theme<3;theme++){a.setTheme(theme);a.validate();}
        bool caught=false;a.timeOfDay=-1;try{a.validate();}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid time accepted");
    });
    test("material limits, presets, reset and automatic day cycle",[]{
        Appearance a;
        for(double value:{0.0,65.0,100.0}){a.transparency=value;a.validate();}
        for(int preset=0;preset<3;preset++){a.setMaterialPreset(preset);a.validate();}
        a.background={45,90,50};a.platform={240,80,40};a.timeOfDay=17.6;
        a.transparency=0;a.gloss=0;a.refraction=1;a.tintStrength=0;
        a.resetMaterial();
        require(a.transparency==65&&a.gloss==0.8&&a.refraction==0.65&&a.tintStrength==0.6,"material reset");
        require(a.background.hue==45&&a.platform.hue==240&&a.timeOfDay==17.6,"material reset modifies scene");
        a.transparency=42;a.jellyColor.hue=77;a.resetEnvironment();
        require(a.transparency==42&&a.jellyColor.hue==77&&a.timeOfDay==14&&a.background.hue==218,"scene reset modifies material");
        a.cycleTime=true;a.timeOfDay=23.9;a.advanceTime(1.0);require(std::abs(a.timeOfDay-0.1)<1e-9,"day cycle wrap");
        a.cycleTime=false;a.advanceTime(5.0);require(std::abs(a.timeOfDay-0.1)<1e-9,"disabled day cycle changes clock");
        bool caught=false;a.transparency=101;try{a.validate();}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid transparency accepted");
        a.transparency=65;a.gloss=-0.1;caught=false;try{a.validate();}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid gloss accepted");
    });
    test("separated broadphase bodies still collide when approaching",[]{
        PhysicsWorld world;world.reset(2);world.settings.gravity=0;
        for(std::size_t i=0;i<2;i++) {
            auto& body=world.bodies()[i];Vec3 delta=Vec3{i==0?-4.0:4.0,3,0}-body.center();
            for(auto& node:body.nodes()){node.position+=delta;node.previous=node.framePrevious=node.position;node.velocity={i==0?4.0:-4.0,0,0};}
        }
        simulate(world,360);healthy(world,0.15);
        require((world.bodies()[0].center()-world.bodies()[1].center()).length()>1.0,"broadphase skipped approaching contact");
    });
    test("cached surface rebinding and affine deformation",[]{
        Surface reused;
        for(int resolution:{3,7,5,3}) {
            SoftBody body({0,4,0},1.8,resolution);reused.update(body,1);
            Surface fresh;fresh.update(body,1);
            for(std::size_t i=0;i<fresh.positions().size();i++)require((reused.positions()[i]-fresh.positions()[i]).length()<1e-10,"stale lattice binding");
            auto old=reused.positions();
            for(auto& node:body.nodes())node.position={node.position.x*1.7+2,node.position.y*0.6+1,node.position.z*0.8-3};
            reused.update(body,1);
            for(std::size_t i=0;i<old.size();i++)require((reused.positions()[i]-Vec3{old[i].x*1.7+2,old[i].y*0.6+1,old[i].z*0.8-3}).length()<1e-9,"affine deformation binding");
        }
    });
    test("sleep wakes on force, grab, settings and direct scene/body edits",[]{
        PhysicsWorld world;simulate(world,1800);require(world.sleeping(),"resting scene did not sleep");
        Vec3 center=world.bodies()[0].center();simulate(world,120);
        require((center-world.bodies()[0].center()).lengthSquared()==0,"sleep position drift");
        world.impulse({0,3,0});require(!world.sleeping(),"impulse did not wake");simulate(world,30);require(world.bodies()[0].center().y>center.y+0.2,"sleep ignored impulse");
        simulate(world,1800);require(world.sleeping(),"scene did not return to sleep");
        world.setGrab(0,world.bodies()[0].index(4,4,2),{1,2.6,0});require(!world.sleeping(),"grab did not wake");
        simulate(world,60);world.releaseGrab();simulate(world,1800);require(world.sleeping(),"released scene did not sleep");
        world.settings.gravity=12;world.step();require(!world.sleeping(),"settings did not wake");simulate(world,1800);require(world.sleeping(),"changed gravity scene did not sleep");
        world.bodies()[0].nodes()[0].position.x+=0.01;world.step();require(!world.sleeping(),"direct node edit did not wake");
        simulate(world,1800);require(world.sleeping(),"edited body did not sleep");
        world.scene().spheres.push_back({{4,1,0},0.5});world.step();require(!world.sleeping(),"scene edit did not wake");
    });
    test("unsupported low-gravity bodies never freeze",[]{
        PhysicsWorld world;world.settings.gravity=0.0001;simulate(world,240);
        require(!world.sleeping(),"unsupported body incorrectly slept");
        require(world.bodies()[0].nodes()[0].velocity.y<0,"low-gravity fall stopped");
    });
    test("UI animation frame independence, reversal and reduced motion",[]{
        AnimatedValue one{1,0},many{1,0};one.advance(0,0.5,false,18);
        for(int i=0;i<60;i++)many.advance(0,1.0/120.0,false,18);
        require(std::abs(one.value-many.value)<1e-12&&std::abs(one.velocity-many.velocity)<1e-11,"frame-dependent animation");
        require(many.value<0.01&&many.value>=0,"collapse settling");
        many.advance(1,0.1);many.advance(0,0.05);many.advance(1,0.8);
        require(std::abs(many.value-1)<0.001,"animation reversal failed");
        many.advance(0,0,true);require(many.value==0&&many.velocity==0,"reduced motion should snap");
        many.advance(1,0);require(many.value==0,"zero dt changed animation");
        many.advance(1,0.016,true);require(many.value==1,"reduced motion reopen");
    });
    test("UI menu hit geometry survives collapse and window resize",[]{
        for(float height:{900.0f,960.0f,1080.0f}) {
            MenuLayout open(1),closed(0),moving(0.6);
            require(open.blocks({100,400},height),"open panel doesn't capture input");
            require(!closed.blocks({100,400},height),"hidden panel captures scene input");
            require(closed.blocks({49,51},height),"closed launcher unavailable");
            require(moving.blocks({40,400},height),"moving panel input leaked");
            require(!open.blocks({600,400},height),"scene area blocked");
            require(open.body(height).height==height-181,"resized menu geometry");
            require(closed.reservedWidth()==0&&open.reservedWidth()==350,"animated viewport spacing");
        }
        require(!roundedContains({20,20,174,62},28,{20,20}),"rounded corner hit outside shape");
    });
    test("graphics API parsing and lossless GLES shader adaptation",[]{
        require(parseApi("opengl")==GraphicsApi::OpenGL&&parseApi("vulkan")==GraphicsApi::Vulkan,"API parsing");
        bool caught=false;try{parseApi("fake-vulkan");}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid API accepted");
        const std::string source="#version 330\nin vec3 point;\nvoid main(){gl_Position=vec4(point,1);}";
        require(GraphicsRuntime::shaderSource(source,false)==source,"native shader changed");
        auto translated=GraphicsRuntime::shaderSource(source,true);
        require(translated.starts_with("#version 300 es\nprecision highp float;\nprecision highp int;"),"ESSL version/precision");
        require(translated.ends_with(source.substr(source.find('\n'))),"shader body altered");
        caught=false;try{GraphicsRuntime::shaderSource("#version 120\nvoid main(){}",true);}catch(const std::invalid_argument&){caught=true;}require(caught,"unsupported source silently accepted");
    });
    test("cross-platform graphics policy and vendor-neutral capabilities",[]{
        require(supportsAngleVulkan(DesktopPlatform::Windows)&&supportsAngleVulkan(DesktopPlatform::Linux),"desktop Vulkan platform policy");
        require(!supportsAngleVulkan(DesktopPlatform::MacOS)&&!supportsAngleVulkan(DesktopPlatform::Other),"unsupported platform disguised as Vulkan");
        require(adequateContext(3,3,false)&&adequateContext(4,1,false)&&adequateContext(3,0,true),"valid context rejected");
        require(!adequateContext(3,2,false)&&!adequateContext(2,1,true),"old context accepted");
        for(auto renderer:{"amd radeon","nvidia geforce","intel iris","apple m2"})require(!softwareRenderer(renderer),"hardware vendor blacklisted");
        for(auto renderer:{"angle swiftshader","mesa llvmpipe","lavapipe","softpipe","software rasterizer"})require(softwareRenderer(renderer),"software falsely reported as hardware Vulkan");
    });
    test("random spawn shapes, colors, budget and preserved world state",[]{
        PhysicsWorld world;world.seedSpawns(431);world.step();
        const auto settings=world.settings;const auto steps=world.stepCount();
        std::vector<Vec3> positions,velocities;for(const auto& n:world.bodies()[0].nodes()){positions.push_back(n.position);velocities.push_back(n.velocity);}
        world.setGrab(0,0,positions[0]);std::array<bool,4> kinds{};
        while(world.bodies().size()<PhysicsWorld::maxBodies){require(world.spawnRandom(),"spawn unexpectedly failed");const auto& body=world.bodies().back();kinds[int(body.shape().kind)]=true;require(body.customColor().has_value(),"missing independent color");require(body.stats().finite&&std::abs(body.stats().volumeRatio-1)<1e-12&&body.stats().minTetRatio>0.999999,"invalid rest shape");for(const auto& tet:body.tetrahedra())require(tet.rest>1e-10,"nonpositive random tetrahedron");for(const auto& n:body.nodes())require(n.inverseMass>0&&n.velocity.lengthSquared()==0,"invalid spawn mass/velocity");}
        require(!world.spawnRandom()&&world.bodies().size()==16,"budget exceeded");
        require(world.stepCount()==steps&&world.settings==settings&&world.grab().has_value(),"spawn reset simulation/settings/grab");
        for(std::size_t i=0;i<positions.size();i++)require((positions[i]-world.bodies()[0].nodes()[i].position).lengthSquared()==0&&(velocities[i]-world.bodies()[0].nodes()[i].velocity).lengthSquared()==0,"existing body changed");
        for(bool kind:kinds)require(kind,"shape family not exercised");
        for(std::size_t a=1;a<world.bodies().size();a++)for(std::size_t b=a+1;b<world.bodies().size();b++)require((world.bodies()[a].customColor().value()-world.bodies()[b].customColor().value()).lengthSquared()>1e-10,"spawn color variation missing");
        world.reset();require(world.bodies().size()==1&&!world.bodies()[0].customColor(),"reset didn't clear spawned bodies");
    });
    test("random spawn deterministic seed, varied geometry and stable drop",[]{
        for(std::uint64_t seed=0;seed<24;seed++) {
            PhysicsWorld a,b;a.seedSpawns(seed);b.seedSpawns(seed);
            require(a.spawnRandom(seed%2?7:5)&&b.spawnRandom(seed%2?7:5),"seeded spawn failed");
            const auto& x=a.bodies().back();const auto& y=b.bodies().back();
            require((x.customColor().value()-y.customColor().value()).lengthSquared()==0,"seed color differs");
            for(std::size_t i=0;i<x.nodes().size();i++)require((x.nodes()[i].position-y.nodes()[i].position).lengthSquared()==0,"seed geometry differs");
            Surface surface(8);surface.update(x,1);for(const auto& normal:surface.normals())require(normal.finite()&&std::abs(normal.length()-1)<1e-8,"random shape normal invalid");
            if(seed<4){a.settings.sleepEnabled=false;simulate(a,480);healthy(a,0.25);}
        }
        bool caught=false;try{BodyShape bad;bad.scale.y=0;SoftBody body({},1.8,5,1.6,bad);}catch(const std::invalid_argument&){caught=true;}require(caught,"degenerate shape accepted");
    });
    test("spawn wakes supported sleeping world",[]{
        PhysicsWorld world;simulate(world,2400);require(world.sleeping(),"fixture didn't sleep");world.seedSpawns(5);require(world.spawnRandom(),"sleeping spawn failed");require(!world.sleeping(),"spawn did not wake world");auto y=world.bodies().back().center().y;world.step();require(world.bodies().back().center().y<y,"new shape didn't fall");
    });
    test("ground appearance ranges and independent reset",[]{
        Appearance a;a.groundWidth=6;a.groundDepth=32;a.groundPattern=1;a.groundPatternScale=4;a.groundPatternStrength=0.9;a.groundRoughness=0;a.groundOutline=false;a.groundRingRadius=10;a.platform={44,89,70};a.background={300,50,20};a.transparency=22;a.timeOfDay=3;
        a.validate();a.resetGround();require(a.groundWidth==16&&a.groundDepth==16&&a.groundPattern==0&&a.groundOutline&&a.platform.hue==218,"ground reset incomplete");require(a.background.hue==300&&a.timeOfDay==3&&a.transparency==22,"ground reset changed other settings");
        for(int field=0;field<6;field++){Appearance bad;if(field==0)bad.groundWidth=0;if(field==1)bad.groundDepth=33;if(field==2)bad.groundPattern=2;if(field==3)bad.groundPatternScale=0;if(field==4)bad.groundRoughness=1.1;if(field==5)bad.groundPatternStrength=std::numeric_limits<double>::quiet_NaN();bool caught=false;try{bad.validate();}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid ground setting accepted");}
    });
    test("rectangular ground resize preserves shape and matches contacts/spawn",[]{
        PhysicsWorld world;for(auto& n:world.bodies()[0].nodes()){n.position.x+=6;n.previous.x+=6;n.framePrevious.x+=6;n.velocity={1,2,3};}
        const auto rest=world.bodies()[0].nodes()[0].rest;const auto relative=world.bodies()[0].nodes().back().position-world.bodies()[0].nodes()[0].position;const auto settings=world.settings;
        world.setGrab(0,0,world.bodies()[0].nodes()[0].position);
        require(world.resizeGround(6,32),"valid resize rejected");require(world.scene().halfExtent==3&&world.scene().halfDepth==16&&!world.grab(),"rectangular bounds/grab invalid");
        for(const auto& n:world.bodies()[0].nodes()){require(n.position.x<=2.965+1e-10&&n.position.x>=-2.965-1e-10,"body not moved inside");require((n.velocity-Vec3{1,2,3}).lengthSquared()==0&&(n.position-n.previous).lengthSquared()==0,"resize altered velocity/interpolation");}
        require((world.bodies()[0].nodes()[0].rest-rest).lengthSquared()==0&&(world.bodies()[0].nodes().back().position-world.bodies()[0].nodes()[0].position-relative).lengthSquared()<1e-24,"resize deformed body");require(world.settings==settings&&world.stepCount()==0,"resize reset physics");
        world.seedSpawns(8);for(int i=0;i<4;i++){require(world.spawnRandom(),"rectangular spawn failed");for(const auto& n:world.bodies().back().nodes())require(std::abs(n.position.x)<3&&std::abs(n.position.z)<16,"spawn outside new ground");}
        world.step();for(const auto& b:world.bodies())for(const auto& n:b.nodes())require(std::abs(n.position.x)<=2.965+1e-9&&std::abs(n.position.z)<=15.965+1e-9,"contacts outside new bounds");
    });
    test("ground shrink rejects stretched bodies transactionally",[]{
        PhysicsWorld world;for(auto& n:world.bodies()[0].nodes())n.position.x*=5;const auto original=world.bodies()[0].nodes()[0].position;world.setGrab(0,0,original);
        require(!world.resizeGround(6,16),"stretched body squeezed into ground");require(world.scene().halfExtent==8&&world.grab().has_value()&&(world.bodies()[0].nodes()[0].position-original).lengthSquared()==0,"rejected resize changed world");
        PhysicsWorld crowded;crowded.bodies().emplace_back(Vec3{6,3.6,0});const auto before=crowded.bodies()[1].nodes()[0].position;
        require(!crowded.resizeGround(6,16),"resize introduced new body overlap");require(crowded.scene().halfExtent==8&&(crowded.bodies()[1].nodes()[0].position-before).lengthSquared()==0,"overlap rejection partially changed world");
        PhysicsWorld touching;touching.bodies().clear();touching.bodies().emplace_back(Vec3{2,3.6,0});touching.bodies().emplace_back(Vec3{3.7,3.6,0});require(!touching.resizeGround(6,16),"resize worsened existing contact by moving bodies differently");
        bool caught=false;try{world.resizeGround(5,16);}catch(const std::invalid_argument&){caught=true;}require(caught,"invalid bounds accepted");
    });
    test("softness continuous mapping, exact endpoints and input validation",[]{
        PhysicsSettings settings;require(settings.softness()==50,"default softness changed");double previous=-1;
        for(int i=0;i<=1000;i++){double value=i*0.1;settings.setSoftness(value);settings.validate();require(settings.edgeCompliance>previous,"nonmonotonic softness");require(std::abs(settings.softness()-value)<1e-10,"softness round trip");previous=settings.edgeCompliance;}
        settings.setSoftness(0);require(settings.rigid()&&settings.edgeCompliance==0,"zero is not rigid");settings.setSoftness(100);require(!settings.rigid()&&settings.edgeCompliance==0.2,"soft endpoint");
        for(double invalid:{-1.,101.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){bool caught=false;try{settings.setSoftness(invalid);}catch(const std::invalid_argument&){caught=true;}require(caught,"bad softness accepted");require(settings.edgeCompliance==0.2,"invalid setter mutated settings");}
    });
    test("softness rigid quaternion fit, arbitrary turns and reflection rejection",[]{
        for(int r:{3,5,7})for(int kind=0;kind<4;kind++)for(double angle:{0.,0.7,3.141592653589793}) {
            BodyShape shape;shape.kind=static_cast<ShapeKind>(kind);shape.scale={0.8,1.2,1.1};SoftBody body({1,3,2},1.8,r,1.6,shape);
            const auto axis=Vec3{1,2,-3}.normalized();Rotation rotation{std::cos(angle/2),axis.x*std::sin(angle/2),axis.y*std::sin(angle/2),axis.z*std::sin(angle/2)};
            for(auto& n:body.nodes())n.position=rotation.apply(n.rest)+Vec3{1,3,2};auto before=body.nodes();body.projectRigid();
            for(std::size_t i=0;i<before.size();i++)require((body.nodes()[i].position-before[i].position).length()<1e-10,"fit changed rigid pose/rotation");
            for(auto& n:body.nodes())n.position.x=2-n.position.x;body.projectRigid();require(maximumStrain(body)<1e-10&&body.stats().minTetRatio>0.999999,"fit allowed reflection/deformation");
        }
    });
    test("softness weighted grab stretches whole jelly and recovers",[]{
        double softPeak=0;
        for(double percent:{0.,50.,100.}) {
            PhysicsWorld world;world.settings.setSoftness(percent);world.settings.gravity=0;world.settings.sleepEnabled=false;
            auto id=world.bodies()[0].index(4,2,2);const auto anchor=world.bodies()[0].nodes()[id].position;world.setGrab(0,id,anchor);
            require(world.grab()->count>1&&world.grab()->count<=32,"point-only grab");double sum=0;for(std::size_t i=0;i<world.grab()->count;i++)sum+=world.grab()->patch[i].weight;require(std::abs(sum-1)<1e-12,"invalid grab weights");
            double peak=0;for(int i=0;i<180;i++){world.moveGrab(anchor+Vec3{i*0.015,0,0});world.step();peak=std::max(peak,maximumStrain(world.bodies()[0]));healthy(world);}
            require(world.bodies()[0].center().x>2,"whole body failed to follow cursor");
            if(percent==0)require(peak<1e-10,"stone deformed while dragged");if(percent==100){softPeak=peak;require(peak>0.15,"jelly did not stretch");}
            world.releaseGrab();simulate(world,960);healthy(world);require(maximumStrain(world.bodies()[0])<(percent==100?0.02:1e-6),"permanent drag deformation");
        }
        require(softPeak<1,"unbounded drag stretch");
    });
    test("softness spring drop, stone contacts and live material change",[]{
        for(int resolution:{5,7})for(double percent:{0.,75.,100.}) {
            PhysicsWorld world;world.reset(1,resolution);world.settings.setSoftness(percent);world.settings.sleepEnabled=false;
            const double rest=height(world.bodies()[0]);double minimum=rest,recovered=0;
            for(int i=0;i<960;i++){world.step();healthy(world,0.025);double h=height(world.bodies()[0]);if(h<minimum){minimum=h;recovered=0;}else recovered=std::max(recovered,h-minimum);if(percent==0)require(maximumStrain(world.bodies()[0])<1e-10,"stone deformed on floor");}
            if(percent==100){require(minimum/rest<0.8,"no elastic impact compression");require(recovered/rest>0.15,"no spring recovery");require(height(world.bodies()[0])/rest>0.8,"body did not regain height");}
            world.settings.setSoftness(0);simulate(world,120);healthy(world);require(maximumStrain(world.bodies()[0])<1e-10,"live switch to stone failed");
            world.settings.setSoftness(100);world.impulse({1,5,0});simulate(world,480);healthy(world,0.025);
        }
    });
    test("softness input batching cannot change physical handle speed",[]{
        PhysicsWorld a,b;for(auto* world:{&a,&b}){world->settings.setSoftness(100);world->settings.gravity=0;auto id=world->bodies()[0].index(4,2,2);world->setGrab(0,id,world->bodies()[0].nodes()[id].position);}
        for(int i=0;i<180;i++){a.moveGrab({6,4,0});for(int repeat=0;repeat<10;repeat++)b.moveGrab({6,4,0});a.step();b.step();}
        for(std::size_t i=0;i<a.bodies()[0].nodes().size();i++)require((a.bodies()[0].nodes()[i].position-b.bodies()[0].nodes()[i].position).lengthSquared()==0,"input-rate dependent grab");
    });
    test("softness extreme random shapes and rigid wall drag remain stable",[]{
        for(int kind=0;kind<4;kind++)for(double percent:{0.,100.}) {
            PhysicsWorld world;world.bodies().clear();BodyShape shape;shape.kind=static_cast<ShapeKind>(kind);shape.scale={0.75,1.25,0.85};shape.yaw=0.7;
            world.bodies().emplace_back(Vec3{0,5,0},1.8,5,1.6,shape);world.settings.setSoftness(percent);simulate(world,600);healthy(world,0.03);
            auto id=world.bodies()[0].surfaceNodes().back();world.setGrab(0,id,world.bodies()[0].nodes()[id].position);
            for(int i=0;i<240;i++){world.moveGrab({9,1,-9});world.step();try{healthy(world,0.08);}catch(...){std::cerr<<"shape="<<kind<<" softness="<<percent<<" step="<<i<<" min_tet="<<world.bodies()[0].stats().minTetRatio<<"\n";throw;}if(percent==0)require(maximumStrain(world.bodies()[0])<1e-10,"wall deformed stone");}
            world.releaseGrab();simulate(world,600);healthy(world,0.05);
        }
    });
    test("invalid configuration and indices rejected",[]{
        bool caught=false;try{SoftBody b({0,0,0},1.8,1);}catch(const std::invalid_argument&){caught=true;}require(caught,"bad topology accepted");
        PhysicsWorld world;world.settings.substeps=0;caught=false;try{world.step();}catch(const std::invalid_argument&){caught=true;}require(caught,"bad settings accepted");
        caught=false;try{world.setGrab(9,0,{});}catch(const std::invalid_argument&){caught=true;}require(caught,"bad grab accepted");
    });
    if(selected==0){std::cerr<<"No matching tests\n";return 2;}
    std::cout<<"RESULT "<<(failed==0?"PASS":"FAIL")<<" failures="<<failed<<" groups="<<selected<<'\n';return failed?1:0;
}
