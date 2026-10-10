#include "jely/physics/PhysicsWorld.hpp"
#include <algorithm>
#include <stdexcept>
#include <bit>
#include "jely/scene/Appearance.hpp"

namespace jely {
PhysicsWorld::PhysicsWorld() { reset(); }
void PhysicsWorld::reset(int preset,int resolution) {
    wake();
    if(preset<0||preset>2) throw std::invalid_argument("Unknown scene preset");
    scene_.configure(preset); bodies_.clear(); grab_.reset(); stepCount_=0;
    if(preset==2) {
        bodies_.emplace_back(Vec3{-1.1,3.5,0},1.8,resolution);
        bodies_.emplace_back(Vec3{1.1,4.0,0.2},1.8,resolution);
        bodies_[0].impulse({1.2,0,0}); bodies_[1].impulse({-1.2,0,0});
    } else bodies_.emplace_back(Vec3{0,preset==1?6.2:3.6,0},1.8,resolution);
    broadphase_.clear();
    broadphase_.reserve(bodies_.size()*static_cast<std::size_t>(resolution*resolution*resolution));
    sleepNodes_.clear();sleepNodes_.reserve(broadphase_.capacity());
    ++structureRevision_;
}
bool PhysicsWorld::spawnRandom(int resolution) {
    if(resolution<3||resolution>10)throw std::invalid_argument("Invalid spawn resolution");
    if(bodies_.size()>=maxBodies)return false;
    auto random=[&](double low,double high){return low+(high-low)*double(spawnRandom_()>>11)*(1.0/9007199254740992.0);};
    BodyShape shape;shape.kind=static_cast<ShapeKind>(spawnRandom_()%4);
    shape.scale={random(0.75,1.15),random(0.75,1.15),random(0.75,1.15)};
    shape.rounding=random(0.2,0.78);shape.yaw=random(0,6.283185307179586);
    if(shape.kind==ShapeKind::Capsule)shape.scale={random(0.65,0.85),random(1.15,1.4),random(0.65,0.85)};
    if(shape.kind==ShapeKind::Pillow)shape.scale={random(1.0,1.3),random(0.5,0.65),random(1.0,1.3)};
    const double size=random(1.2,1.8);
    const auto color=HsvColor{random(0,360),random(60,95),random(75,98)}.rgb();
    SoftBody candidate({},size,resolution,1.6*std::pow(size/1.8,3)*shape.scale.x*shape.scale.y*shape.scale.z,shape,color);
    Vec3 low{1e9,1e9,1e9},high{-1e9,-1e9,-1e9};
    for(const auto& node:candidate.nodes()){low={std::min(low.x,node.rest.x),std::min(low.y,node.rest.y),std::min(low.z,node.rest.z)};high={std::max(high.x,node.rest.x),std::max(high.y,node.rest.y),std::max(high.z,node.rest.z)};}
    // Spawn in a clear horizontal column above existing physical bounds. No old node is moved.
    const double xMin=-scene_.halfExtent-low.x+0.25,xMax=scene_.halfExtent-high.x-0.25;
    const double zMin=-scene_.halfDepth-low.z+0.25,zMax=scene_.halfDepth-high.z-0.25;
    if(xMin>xMax||zMin>zMax)return false;
    struct Bounds {Vec3 low,high;double radius;};
    std::array<Bounds,maxBodies> bounds{};
    for(std::size_t i=0;i<bodies_.size();i++){
        const auto& body=bodies_[i];auto& box=bounds[i];box.low={1e9,1e9,1e9};box.high={-1e9,-1e9,-1e9};box.radius=body.particleRadius();
        for(const auto& n:body.nodes()){box.low={std::min(box.low.x,n.position.x),std::min(box.low.y,n.position.y),std::min(box.low.z,n.position.z)};box.high={std::max(box.high.x,n.position.x),std::max(box.high.y,n.position.y),std::max(box.high.z,n.position.z)};}
    }
    Vec3 center{};double bestHeight=1e9;
    for(int trial=0;trial<32;trial++) {
        Vec3 p{random(std::max(xMin,-5.0),std::min(xMax,5.0)),4.5,random(std::max(zMin,-5.0),std::min(zMax,5.0))};
        for(std::size_t i=0;i<bodies_.size();i++) {
            const auto& box=bounds[i];const auto a=box.low,b=box.high;
            const double gap=box.radius+candidate.particleRadius()+0.1;
            if(p.x+high.x+gap>=a.x&&p.x+low.x-gap<=b.x&&p.z+high.z+gap>=a.z&&p.z+low.z-gap<=b.z)p.y=std::max(p.y,b.y-low.y+gap);
        }
        if(p.y<bestHeight){center=p;bestHeight=p.y;}
        if(bestHeight==4.5)break;
    }
    for(auto& node:candidate.nodes())node.position=node.previous=node.framePrevious=center+node.rest;
    std::size_t total=candidate.nodes().size();for(const auto& body:bodies_)total+=body.nodes().size();
    broadphase_.reserve(total);sleepNodes_.reserve(total);
    bodies_.push_back(std::move(candidate));wake();++structureRevision_;
    return true;
}
void PhysicsWorld::wake() {sleeping_=false;quietSteps_=0;}
bool PhysicsWorld::resizeGround(double width,double depth) {
    if(!std::isfinite(width)||!std::isfinite(depth)||width<6||width>32||depth<6||depth>32)throw std::invalid_argument("Ground dimensions must be 6..32");
    const double halfX=width*0.5,halfZ=depth*0.5,skin=0.035;
    if(halfX==scene_.halfExtent&&halfZ==scene_.halfDepth)return true;
    std::vector<Vec3> shifts;shifts.reserve(bodies_.size());
    std::vector<std::pair<Vec3,Vec3>> bounds;bounds.reserve(bodies_.size());
    for(const auto& body:bodies_) {
        Vec3 low{1e9,1e9,1e9},high{-1e9,-1e9,-1e9};
        for(const auto& n:body.nodes()){low={std::min(low.x,n.position.x),std::min(low.y,n.position.y),std::min(low.z,n.position.z)};high={std::max(high.x,n.position.x),std::max(high.y,n.position.y),std::max(high.z,n.position.z)};}
        if(high.x-low.x>width-2*skin||high.z-low.z>depth-2*skin)return false;
        bounds.push_back({low,high});
        const double dx=low.x<-halfX+skin?-halfX+skin-low.x:(high.x>halfX-skin?halfX-skin-high.x:0);
        const double dz=low.z<-halfZ+skin?-halfZ+skin-low.z:(high.z>halfZ-skin?halfZ-skin-high.z:0);
        shifts.push_back({dx,0,dz});
    }
    auto overlap=[](Vec3 a,Vec3 b,Vec3 c,Vec3 d,double pad){return a.x<=d.x+pad&&c.x<=b.x+pad&&a.y<=d.y+pad&&c.y<=b.y+pad&&a.z<=d.z+pad&&c.z<=b.z+pad;};
    for(std::size_t a=0;a<bodies_.size();a++)for(std::size_t b=a+1;b<bodies_.size();b++){
        const double pad=bodies_[a].particleRadius()+bodies_[b].particleRadius()+0.02;
        const auto [loA,hiA]=bounds[a];const auto [loB,hiB]=bounds[b];
        // Conservatively reject relative movement within contact bounds, including existing contacts.
        // Equal whole-body translations preserve the pair's relative geometry.
        if((shifts[a]-shifts[b]).lengthSquared()>0&&overlap(loA+shifts[a],hiA+shifts[a],loB+shifts[b],hiB+shifts[b],pad))return false;
    }
    for(std::size_t b=0;b<bodies_.size();b++)for(auto& n:bodies_[b].nodes()){n.position+=shifts[b];n.previous+=shifts[b];n.framePrevious+=shifts[b];}
    scene_.halfExtent=halfX;scene_.halfDepth=halfZ;releaseGrab();wake();
    return true;
}
std::uint64_t PhysicsWorld::environmentSignature() const {
    std::uint64_t hash=1469598103934665603ULL;
    auto add=[&](double value){hash=(hash^std::bit_cast<std::uint64_t>(value))*1099511628211ULL;};
    add(scene_.halfExtent);add(scene_.halfDepth);add(double(scene_.spheres.size()));add(double(scene_.boxes.size()));
    add(double(bodies_.size()));for(const auto& body:bodies_){add(body.size());add(double(body.resolution()));add(body.restVolume());}
    for(const auto& sphere:scene_.spheres){add(sphere.center.x);add(sphere.center.y);add(sphere.center.z);add(sphere.radius);}
    for(const auto& box:scene_.boxes){add(box.center.x);add(box.center.y);add(box.center.z);add(box.halfSize.x);add(box.halfSize.y);add(box.halfSize.z);}
    return hash;
}
bool PhysicsWorld::sleepingStateUnchanged() const {
    if(!settings.sleepEnabled||grab_||!(settings==sleepSettings_)||environmentSignature()!=sleepEnvironment_)return false;
    std::size_t index=0;
    for(const auto& body:bodies_)for(const auto& node:body.nodes()) {
        if(index>=sleepNodes_.size())return false;
        const auto& cached=sleepNodes_[index++];
        if(node.position.x!=cached.position.x||node.position.y!=cached.position.y||node.position.z!=cached.position.z||node.inverseMass!=cached.inverseMass||node.velocity.lengthSquared()!=0)return false;
    }
    return index==sleepNodes_.size();
}
void PhysicsWorld::considerSleep() {
    if(!settings.sleepEnabled||grab_){quietSteps_=0;return;}
    for(const auto& body:bodies_) {
        bool supported=settings.gravity==0;
        for(const auto& node:body.nodes()) {
            if(node.velocity.lengthSquared()>1e-6){quietSteps_=0;return;}
            const auto normal=node.contactNormal;
            supported=supported||(normal.y>0&&normal.y*normal.y>0.0625*normal.lengthSquared());
        }
        if(!supported){quietSteps_=0;return;}
    }
    if(++quietSteps_<180)return;
    for(const auto& body:bodies_) {
        auto stats=body.stats();
        if(!stats.finite||std::abs(stats.volumeRatio-1)>0.025||stats.minTetRatio<0.85){quietSteps_=0;return;}
    }
    sleepNodes_.clear();
    for(auto& body:bodies_)for(auto& node:body.nodes()) {
        node.velocity={};node.previous=node.framePrevious=node.position;
        sleepNodes_.push_back({node.position,node.inverseMass});
    }
    sleepSettings_=settings;sleepEnvironment_=environmentSignature();sleeping_=true;
}
void PhysicsWorld::collideEnvironment(SoftBody& body) {
    constexpr double skin=0.035;
    for(auto& node:body.nodes()) {
        auto project=[&](Vec3 normal,double penetration) {
            if(penetration>0) { node.position+=normal*penetration; node.contactNormal+=normal; }
        };
        project({0,1,0},skin-node.position.y);
        project({1,0,0},-scene_.halfExtent+skin-node.position.x);
        project({-1,0,0},node.position.x-scene_.halfExtent+skin);
        project({0,0,1},-scene_.halfDepth+skin-node.position.z);
        project({0,0,-1},node.position.z-scene_.halfDepth+skin);
        for(const auto& sphere:scene_.spheres) {
            Vec3 d=node.position-sphere.center; double l=d.length();
            project(d.normalized(),sphere.radius+skin-l);
        }
        for(const auto& box:scene_.boxes) {
            Vec3 local=node.position-box.center,ext=box.halfSize+Vec3{skin,skin,skin};
            Vec3 distance{ext.x-std::abs(local.x),ext.y-std::abs(local.y),ext.z-std::abs(local.z)};
            if(distance.x>0&&distance.y>0&&distance.z>0) {
                if(distance.x<distance.y&&distance.x<distance.z) project({local.x>=0?1.0:-1.0,0,0},distance.x);
                else if(distance.z<distance.y) project({0,0,local.z>=0?1.0:-1.0},distance.z);
                else project({0,local.y>=0?1.0:-1.0,0},distance.y);
            }
        }
    }
}
void PhysicsWorld::collideBodies() {
    if(bodies_.size()<2) return;
    if(bodies_.size()==2) {
        std::array<Vec3,2> lower{},upper{};
        for(std::size_t i=0;i<2;i++) {
            lower[i]=upper[i]=bodies_[i].nodes()[bodies_[i].surfaceNodes().front()].position;
            for(auto index:bodies_[i].surfaceNodes()) {
                const Vec3 p=bodies_[i].nodes()[index].position;
                lower[i]={std::min(lower[i].x,p.x),std::min(lower[i].y,p.y),std::min(lower[i].z,p.z)};
                upper[i]={std::max(upper[i].x,p.x),std::max(upper[i].y,p.y),std::max(upper[i].z,p.z)};
            }
        }
        const double padding=bodies_[0].particleRadius()+bodies_[1].particleRadius();
        if(lower[0].x>upper[1].x+padding||lower[1].x>upper[0].x+padding||
           lower[0].y>upper[1].y+padding||lower[1].y>upper[0].y+padding||
           lower[0].z>upper[1].z+padding||lower[1].z>upper[0].z+padding)return;
    }
    broadphase_.clear();
    for(std::size_t b=0;b<bodies_.size();b++)for(auto n:bodies_[b].surfaceNodes())
        broadphase_.push_back({b,n,bodies_[b].nodes()[n].position.x,bodies_[b].particleRadius()});
    std::sort(broadphase_.begin(),broadphase_.end(),[](const auto& a,const auto& b){return a.x<b.x;});
    double maxRadius=0; for(const auto& p:broadphase_) maxRadius=std::max(maxRadius,p.radius);
    // Sweep-and-prune culls separated node pairs without allocating per solver iteration.
    for(std::size_t i=0;i<broadphase_.size();i++) {
        auto a=broadphase_[i];
        for(std::size_t j=i+1;j<broadphase_.size()&&broadphase_[j].x-a.x<a.radius+maxRadius;j++) {
            auto b=broadphase_[j]; if(a.body==b.body) continue;
            Node& na=bodies_[a.body].nodes()[a.node]; Node& nb=bodies_[b.body].nodes()[b.node];
            Vec3 d=na.position-nb.position;double squared=d.lengthSquared(),r=a.radius+b.radius;
            if(squared>=r*r) continue;
            double l=std::sqrt(squared);
            Vec3 normal=l>1e-12?d/l:Vec3{a.body<b.body?-1.0:1.0,0,0};
            double correction=(r-l)/(na.inverseMass+nb.inverseMass);
            na.position+=normal*(correction*na.inverseMass); nb.position-=normal*(correction*nb.inverseMass);
            na.contactNormal+=normal; nb.contactNormal-=normal;
        }
    }
}
void PhysicsWorld::solveGrab(double h) {
    if(!grab_) return;
    auto& grab=*grab_;auto& nodes=bodies_[grab.body].nodes();Vec3 point=grab.offset,previousPoint=grab.offset;double inverse=0;
    for(std::size_t i=0;i<grab.count;i++){const auto [id,w]=grab.patch[i];point+=nodes[id].position*w;previousPoint+=nodes[id].previous*w;inverse+=nodes[id].inverseMass*w*w;}
    if(inverse<1e-15)return;
    constexpr double compliance=0.000003,dampingRatio=0.7;
    const double alpha=compliance/(h*h);
    // XPBD viscous damping (Macklin et al., 2016, equation 26), measured
    // relative to the moving hand. Damping absolute speed would kill throws.
    const double gamma=2*dampingRatio*std::sqrt(compliance/inverse)/h;
    const Vec3 relativeMotion=point-previousPoint-grab.velocity*h;
    Vec3 error=point-grab.target;
    auto solve=[&](double c,double motion,double& lambda){double dl=(-c-alpha*lambda-gamma*motion)/((1+gamma)*inverse+alpha);lambda+=dl;return dl;};
    Vec3 correction{solve(error.x,relativeMotion.x,grab.lambdaX),solve(error.y,relativeMotion.y,grab.lambdaY),solve(error.z,relativeMotion.z,grab.lambdaZ)};
    for(std::size_t i=0;i<grab.count;i++){const auto [id,w]=grab.patch[i];nodes[id].position+=correction*(nodes[id].inverseMass*w);}
}
void PhysicsWorld::finishRigidContacts(SoftBody& body) {
    body.projectRigid();
    Vec3 low{1e9,1e9,1e9},high{-1e9,-1e9,-1e9};
    for(const auto& n:body.nodes()){low={std::min(low.x,n.position.x),std::min(low.y,n.position.y),std::min(low.z,n.position.z)};high={std::max(high.x,n.position.x),std::max(high.y,n.position.y),std::max(high.z,n.position.z)};}
    constexpr double skin=0.035;
    auto bound=[](double low,double high,double half){return low<-half+0.035?-half+0.035-low:(high>half-0.035?half-0.035-high:0);};
    Vec3 shift{bound(low.x,high.x,scene_.halfExtent),std::max(0.0,skin-low.y),bound(low.z,high.z,scene_.halfDepth)};
    for(auto& n:body.nodes())n.position+=shift;
}
void PhysicsWorld::step() {
    settings.validate(); double h=fixedStep/settings.substeps;
    if(sleeping_){if(sleepingStateUnchanged()){++stepCount_;return;}wake();}
    for(auto& body:bodies_) body.beginFrame();
    for(int substep=0;substep<settings.substeps;substep++) {
        // A finite acceleration prevents a cursor teleport or reversal from
        // injecting an instantaneous velocity jump into a soft surface patch.
        // Braking speed shrinks near the goal; all updates use physics time.
        if(grab_){
            constexpr double acceleration=80.0,maxSpeed=12.0;
            const Vec3 delta=grab_->goal-grab_->target;
            const double speed=std::min(maxSpeed,std::sqrt(2*acceleration*delta.length()));
            const Vec3 desired=limited(delta/h,speed);
            grab_->velocity+=limited(desired-grab_->velocity,acceleration*h);
            grab_->target+=grab_->velocity*h;
        }
        for(auto& body:bodies_) body.integrate(h,settings);
        if(grab_) grab_->lambdaX=grab_->lambdaY=grab_->lambdaZ=0;
        for(int i=0;i<settings.iterations;i++) {
            for(auto& body:bodies_) body.solve(h,settings);
            solveGrab(h); collideBodies();
            for(auto& body:bodies_) collideEnvironment(body);
            if(settings.rigid())for(auto& body:bodies_)finishRigidContacts(body);
        }
        for(auto& body:bodies_) body.finish(h,settings);
    }
    ++stepCount_;
    considerSleep();
}
void PhysicsWorld::impulse(Vec3 delta) {wake();for(auto& body:bodies_) body.impulse(delta);}
void PhysicsWorld::setGrab(std::size_t body,std::size_t node,Vec3 target) {
    if(body>=bodies_.size()||node>=bodies_[body].nodes().size()||!target.finite()) throw std::invalid_argument("Invalid grab target");
    wake();Grab grab;grab.body=body;grab.node=node;grab.target=grab.goal=target;
    const auto& nodes=bodies_[body].nodes();const auto anchor=nodes[node].rest;
    const double radius=bodies_[body].size()*0.42;
    std::vector<std::pair<double,std::size_t>> nearby;nearby.reserve(bodies_[body].surfaceNodes().size());
    for(auto id:bodies_[body].surfaceNodes()){double d=(nodes[id].rest-anchor).lengthSquared();if(d<=radius*radius||id==node)nearby.push_back({d,id});}
    if(nearby.empty())nearby.push_back({0,node});
    std::sort(nearby.begin(),nearby.end());grab.count=std::min(nearby.size(),grab.patch.size());
    double sum=0;for(std::size_t i=0;i<grab.count;i++){double w=std::exp(-nearby[i].first/(radius*radius*0.5));grab.patch[i]={nearby[i].second,w};sum+=w;}
    Vec3 center{};for(std::size_t i=0;i<grab.count;i++){grab.patch[i].weight/=sum;center+=nodes[grab.patch[i].node].position*grab.patch[i].weight;}
    grab.offset=nodes[node].position-center;grab_=grab;
}
void PhysicsWorld::moveGrab(Vec3 target) {
    if(grab_&&target.finite()) {
        wake();
        grab_->goal={std::clamp(target.x,-scene_.halfExtent+0.1,scene_.halfExtent-0.1),std::clamp(target.y,0.15,9.0),std::clamp(target.z,-scene_.halfDepth+0.1,scene_.halfDepth-0.1)};
    }
}
}
