#include "jely/physics/SoftBody.hpp"
#include <set>
#include <stdexcept>
#include "jely/math/RigidFit.hpp"

namespace jely {
void PhysicsSettings::setSoftness(double percent) {
    if(!std::isfinite(percent)||percent<0||percent>100)throw std::invalid_argument("Softness must be 0..100");
    // Continuous zero endpoint; 50% retains the previous default compliance exactly.
    edgeCompliance=percent==0?0:(percent==100?0.2:(percent==50?0.000025:0.2*std::expm1(percent*std::log(7999.0)/50.0)/(7999.0*7999.0-1)));
}
double PhysicsSettings::softness() const {return std::clamp(50.0*std::log1p(edgeCompliance*(7999.0*7999.0-1)/0.2)/std::log(7999.0),0.0,100.0);}
void PhysicsSettings::validate() const {
    if(!std::isfinite(gravity)||gravity<0||gravity>30||!std::isfinite(edgeCompliance)||edgeCompliance<0||edgeCompliance>0.2||
       !std::isfinite(volumeCompliance)||volumeCompliance<0||volumeCompliance>0.001||!std::isfinite(damping)||damping<0||damping>10||
       !std::isfinite(friction)||friction<0||friction>1||!std::isfinite(restitution)||restitution<0||restitution>1||
       substeps<1||substeps>12||iterations<1||iterations>40) throw std::invalid_argument("Invalid physics configuration");
}
std::size_t SoftBody::index(int x,int y,int z) const { return static_cast<std::size_t>((z*resolution_+y)*resolution_+x); }
SoftBody::SoftBody(Vec3 center,double size,int resolution,double mass,BodyShape shape,std::optional<Vec3> color):resolution_(resolution),size_(size),shape_(shape),color_(color) {
    if(!center.finite()||!std::isfinite(size)||size<0.25||size>5||resolution<3||resolution>10||!std::isfinite(mass)||mass<=0)
        throw std::invalid_argument("Invalid soft body topology");
    if(int(shape.kind)<0||int(shape.kind)>3||!shape.scale.finite()||shape.scale.x<0.5||shape.scale.x>1.5||shape.scale.y<0.5||shape.scale.y>1.5||shape.scale.z<0.5||shape.scale.z>1.5||!std::isfinite(shape.rounding)||shape.rounding<0||shape.rounding>0.85||!std::isfinite(shape.yaw))throw std::invalid_argument("Invalid rest shape");
    if(color&&(!color->finite()||color->x<0||color->x>1||color->y<0||color->y>1||color->z<0||color->z>1))throw std::invalid_argument("Invalid body color");
    const double cosine=std::cos(shape.yaw),sine=std::sin(shape.yaw);
    nodes_.resize(static_cast<std::size_t>(resolution*resolution*resolution));
    for(int z=0;z<resolution;z++) for(int y=0;y<resolution;y++) for(int x=0;x<resolution;x++) {
        Vec3 p{2.0*x/(resolution-1)-1,2.0*y/(resolution-1)-1,2.0*z/(resolution-1)-1};
        // Round corners in the rest configuration; the deformation remains entirely simulated.
        Vec3 cut{shape.rounding,shape.rounding,shape.rounding};
        if(shape.kind==ShapeKind::Ellipsoid)cut={0,0,0};
        if(shape.kind==ShapeKind::Capsule)cut={0,0.55,0};
        Vec3 q{std::clamp(p.x,-cut.x,cut.x),std::clamp(p.y,-cut.y,cut.y),std::clamp(p.z,-cut.z,cut.z)};
        Vec3 d=p-q;
        double radius=std::max({std::abs(d.x),std::abs(d.y),std::abs(d.z)});
        p=(q+d.normalized({0,0,0})*radius)*(size*0.5);
        p={p.x*shape.scale.x,p.y*shape.scale.y,p.z*shape.scale.z};
        p={cosine*p.x+sine*p.z,p.y,-sine*p.x+cosine*p.z};
        Node& node=nodes_[index(x,y,z)];
        node.rest=p; node.position=node.previous=node.framePrevious=center+p;
        node.surface=x==0||y==0||z==0||x==resolution-1||y==resolution-1||z==resolution-1;
        if(node.surface)surfaceNodes_.push_back(index(x,y,z));
    }
    // Freudenthal triangulation gives neighbouring cells identical shared-face diagonals.
    constexpr std::array<std::array<int,3>,6> permutations{{{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}}};
    std::set<std::pair<std::size_t,std::size_t>> edgeSet;
    std::vector<double> nodeVolume(nodes_.size(),0.0);
    for(int z=0;z<resolution-1;z++) for(int y=0;y<resolution-1;y++) for(int x=0;x<resolution-1;x++) {
        for(auto permutation:permutations) {
            std::array<int,3> c{x,y,z};
            VolumeConstraint tet;
            tet.nodes[0]=index(c[0],c[1],c[2]);
            for(int k=0;k<3;k++) { ++c[permutation[k]]; tet.nodes[k+1]=index(c[0],c[1],c[2]); }
            auto volume=[&]{return signedVolume(nodes_[tet.nodes[0]].rest,nodes_[tet.nodes[1]].rest,nodes_[tet.nodes[2]].rest,nodes_[tet.nodes[3]].rest);};
            if(volume()<0) std::swap(tet.nodes[1],tet.nodes[2]);
            tet.rest=volume(); restVolume_+=tet.rest;
            if(tet.rest<1e-10) throw std::runtime_error("Degenerate tetrahedron");
            for(auto id:tet.nodes) nodeVolume[id]+=tet.rest/4.0;
            for(int a=0;a<4;a++) for(int b=a+1;b<4;b++) edgeSet.emplace(std::min(tet.nodes[a],tet.nodes[b]),std::max(tet.nodes[a],tet.nodes[b]));
            volumes_.push_back(tet);
        }
    }
    for(auto [a,b]:edgeSet) edges_.push_back({a,b,(nodes_[a].rest-nodes_[b].rest).length(),0});
    nodeTets_.resize(nodes_.size());guardedPositions_.resize(nodes_.size());guardedMinimum_.resize(volumes_.size());
    for(std::size_t i=0;i<volumes_.size();i++)for(auto node:volumes_[i].nodes)nodeTets_[node].push_back(i);
    masses_.resize(nodes_.size());mass_=mass;
    for(std::size_t i=0;i<nodes_.size();i++){masses_[i]=mass*nodeVolume[i]/restVolume_;nodes_[i].inverseMass=1/masses_[i];restMassCenter_+=nodes_[i].rest*masses_[i];}
    restMassCenter_*=1/mass_;
}
void SoftBody::projectRigid() {
    Vec3 center{};for(std::size_t i=0;i<nodes_.size();i++)center+=nodes_[i].position*masses_[i];center*=1/mass_;
    std::array<Vec3,3> covariance{};
    for(std::size_t i=0;i<nodes_.size();i++){const auto p=(nodes_[i].position-center)*masses_[i],r=nodes_[i].rest-restMassCenter_;covariance[0]+=p*r.x;covariance[1]+=p*r.y;covariance[2]+=p*r.z;}
    const auto rotation=rigidRotation(covariance);
    for(auto& n:nodes_)n.position=center+rotation.apply(n.rest-restMassCenter_);
}
void SoftBody::rigidVelocity() {
    Vec3 center{},linear{};for(std::size_t i=0;i<nodes_.size();i++){center+=nodes_[i].position*masses_[i];linear+=nodes_[i].velocity*masses_[i];}center*=1/mass_;linear*=1/mass_;
    std::array<Vec3,3> inertia{};Vec3 momentum{};
    for(std::size_t i=0;i<nodes_.size();i++){auto r=nodes_[i].position-center;double m=masses_[i],rr=r.lengthSquared();inertia[0]+=Vec3{rr-r.x*r.x,-r.y*r.x,-r.z*r.x}*m;inertia[1]+=Vec3{-r.x*r.y,rr-r.y*r.y,-r.z*r.y}*m;inertia[2]+=Vec3{-r.x*r.z,-r.y*r.z,rr-r.z*r.z}*m;momentum+=r.cross(nodes_[i].velocity-linear)*m;}
    const double determinant=inertia[0].dot(inertia[1].cross(inertia[2]));Vec3 angular{};
    if(determinant>1e-20)angular={momentum.dot(inertia[1].cross(inertia[2]))/determinant,momentum.dot(inertia[2].cross(inertia[0]))/determinant,momentum.dot(inertia[0].cross(inertia[1]))/determinant};
    double speed=0;for(const auto& n:nodes_)speed=std::max(speed,(linear+angular.cross(n.position-center)).length());const double factor=speed>35?35/speed:1;
    for(auto& n:nodes_)n.velocity=(linear+angular.cross(n.position-center))*factor;
}
void SoftBody::beginFrame() { for(auto& n:nodes_) n.framePrevious=n.position; }
void SoftBody::integrate(double h,const PhysicsSettings& settings) {
    const double dampingFactor=std::exp(-settings.damping*h);
    for(auto& node:nodes_) {
        node.previous=node.position; node.contactNormal={};
        node.velocity.y-=settings.gravity*h;
        node.velocity=limited(node.velocity*dampingFactor,35.0);
        node.position+=node.velocity*h;
    }
    for(auto& edge:edges_) edge.lambda=0;
    for(auto& tet:volumes_) tet.lambda=tet.barrierLambda=0;
}
void SoftBody::solveVolume(VolumeConstraint& tet,double alpha,bool barrier) {
    auto& a=nodes_[tet.nodes[0]]; auto& b=nodes_[tet.nodes[1]];
    auto& c=nodes_[tet.nodes[2]]; auto& d=nodes_[tet.nodes[3]];
    Vec3 ab=b.position-a.position,ac=c.position-a.position,ad=d.position-a.position;
    double volume=ab.dot(ac.cross(ad))/6;
    double C=volume-tet.rest*(barrier?0.15:1.0);
    double& lambda=barrier?tet.barrierLambda:tet.lambda;
    // Most tetrahedra are far from inversion: do not build gradients for an inactive barrier.
    if(barrier&&C>=0&&lambda==0) return;
    std::array<Vec3,4> g{};
    g[1]=ac.cross(ad)/6; g[2]=ad.cross(ab)/6; g[3]=ab.cross(ac)/6; g[0]=-(g[1]+g[2]+g[3]);
    if(!barrier) {
        // A soft stretch material still resists large local volume loss under
        // a floor/wall grab. Keep the small-strain bulk compliance unchanged.
        const double strain=C/tet.rest,squaredStrain=strain*strain;
        C*=1+12*squaredStrain;
        const double gradient=1+36*squaredStrain;
        for(auto& direction:g)direction*=gradient;
    }
    double denominator=alpha;
    for(int k=0;k<4;k++) denominator+=nodes_[tet.nodes[k]].inverseMass*g[k].lengthSquared();
    if(denominator<1e-15) return;
    double delta=(-C-alpha*lambda)/denominator;
    if(barrier) { double next=std::max(0.0,lambda+delta); delta=next-lambda; }
    lambda+=delta;
    for(int k=0;k<4;k++) nodes_[tet.nodes[k]].position+=g[k]*(nodes_[tet.nodes[k]].inverseMass*delta);
}
void SoftBody::solve(double h,const PhysicsSettings& settings) {
    if(settings.rigid()){projectRigid();return;}
    double edgeAlpha=settings.edgeCompliance/(h*h),volumeAlpha=settings.volumeCompliance/(h*h);
    for(auto& edge:edges_) {
        auto& a=nodes_[edge.a]; auto& b=nodes_[edge.b]; Vec3 delta=a.position-b.position; double length=delta.length();
        if(length<1e-12) continue;
        // Finite-strain elasticity: soft jelly stretches easily near its rest shape,
        // then progressively stiffens rather than becoming a nearly free lattice.
        // Use the derivative in both the XPBD denominator and position correction
        // so the nonlinear constraint and its accumulated multiplier stay consistent.
        constexpr double hardening=12.0;
        const double strain=(length-edge.rest)/edge.rest;
        const double squaredStrain=strain*strain;
        const double constraint=(length-edge.rest)*(1+hardening*squaredStrain);
        const double gradient=1+3*hardening*squaredStrain;
        const double denominator=(a.inverseMass+b.inverseMass)*gradient*gradient+edgeAlpha;
        const double dl=(-constraint-edgeAlpha*edge.lambda)/denominator;
        edge.lambda+=dl; Vec3 correction=delta*(gradient*dl/length);
        a.position+=correction*a.inverseMass; b.position-=correction*b.inverseMass;
    }
    for(auto& tet:volumes_) { solveVolume(tet,volumeAlpha,false); solveVolume(tet,0,true); }
}
void SoftBody::guardInversion() {
    auto valid=[&](double fraction){
        for(const auto& tet:volumes_) {
            std::array<Vec3,4> p;for(int k=0;k<4;k++){const auto& n=nodes_[tet.nodes[k]];p[k]=lerp(n.previous,n.position,fraction);}
            if(signedVolume(p[0],p[1],p[2],p[3])+tet.rest*1e-10<tet.rest*0.1)return false;
        }
        return true;
    };
    if(valid(1))return;
    // Backtrack only nodes touching a threatened tetrahedron. A single
    // compressed contact must not cancel movement and recovery of the body.
    for(std::size_t i=0;i<nodes_.size();i++){guardedPositions_[i]=nodes_[i].position;nodes_[i].position=nodes_[i].previous;}
    for(std::size_t i=0;i<volumes_.size();i++) {
        const auto& tet=volumes_[i];std::array<Vec3,4> p;
        for(int k=0;k<4;k++)p[k]=nodes_[tet.nodes[k]].previous;
        guardedMinimum_[i]=std::min(tet.rest*0.1,signedVolume(p[0],p[1],p[2],p[3]));
    }
    for(int pass=0;pass<3;pass++)for(std::size_t i=0;i<nodes_.size();i++) {
        const Vec3 start=nodes_[i].position,goal=guardedPositions_[i];
        if((goal-start).lengthSquared()<1e-24)continue;
        auto admissible=[&](double fraction){
            for(auto id:nodeTets_[i]) {
                const auto& tet=volumes_[id];std::array<Vec3,4> p;
                for(int k=0;k<4;k++){auto j=tet.nodes[k];p[k]=j==i?lerp(start,goal,fraction):nodes_[j].position;}
                if(signedVolume(p[0],p[1],p[2],p[3])+tet.rest*1e-10<guardedMinimum_[id])return false;
            }
            return true;
        };
        double fraction=1;
        for(int k=0;k<16&&!admissible(fraction);k++)fraction*=0.5;
        if(admissible(fraction))nodes_[i].position=lerp(start,goal,fraction);
    }
    // Independent contact corrections must also retain the bulk volume. Keep
    // admissible translation while damping only excessive volume-changing strain.
    double previousVolume=0,currentVolume=0;
    for(const auto& tet:volumes_) {
        std::array<Vec3,4> p,q;for(int k=0;k<4;k++){p[k]=nodes_[tet.nodes[k]].previous;q[k]=nodes_[tet.nodes[k]].position;}
        previousVolume+=signedVolume(p[0],p[1],p[2],p[3]);currentVolume+=signedVolume(q[0],q[1],q[2],q[3]);
    }
    const double allowed=std::max(restVolume_*0.025,std::abs(previousVolume-restVolume_));
    if(std::abs(currentVolume-restVolume_)<=allowed)return;
    Vec3 translation{},lower{1e9,1e9,1e9},upper{-1e9,-1e9,-1e9};
    for(std::size_t i=0;i<nodes_.size();i++) {
        const auto& n=nodes_[i];guardedPositions_[i]=n.position;translation+=(n.position-n.previous)*(masses_[i]/mass_);
        for(Vec3 p:{n.previous,n.position}){lower={std::min(lower.x,p.x),std::min(lower.y,p.y),std::min(lower.z,p.z)};upper={std::max(upper.x,p.x),std::max(upper.y,p.y),std::max(upper.z,p.z)};}
    }
    auto pose=[&](double fraction){
        Vec3 lo{1e9,1e9,1e9},hi{-1e9,-1e9,-1e9};
        for(std::size_t i=0;i<nodes_.size();i++) {
            const auto p=lerp(nodes_[i].previous,guardedPositions_[i],fraction);
            lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};
        }
        Vec3 shift=translation*(1-fraction);
        shift={std::clamp(shift.x,lower.x-lo.x,upper.x-hi.x),std::clamp(shift.y,lower.y-lo.y,upper.y-hi.y),std::clamp(shift.z,lower.z-lo.z,upper.z-hi.z)};
        for(std::size_t i=0;i<nodes_.size();i++)nodes_[i].position=lerp(nodes_[i].previous,guardedPositions_[i],fraction)+shift;
        double volume=0;
        for(std::size_t i=0;i<volumes_.size();i++) {
            const auto& tet=volumes_[i];std::array<Vec3,4> p;for(int k=0;k<4;k++)p[k]=nodes_[tet.nodes[k]].position;
            double v=signedVolume(p[0],p[1],p[2],p[3]);if(v+tet.rest*1e-10<guardedMinimum_[i])return false;volume+=v;
        }
        return std::abs(volume-restVolume_)<=allowed+restVolume_*1e-10;
    };
    double fraction=0.5;for(int k=0;k<16&&!pose(fraction);k++)fraction*=0.5;
    if(!pose(fraction))pose(0);
}
void SoftBody::finish(double h,const PhysicsSettings& settings) {
    if(!settings.rigid())guardInversion();
    for(auto& node:nodes_) {
        Vec3 incoming=node.velocity;
        node.velocity=(node.position-node.previous)/h;
        if(node.contactNormal.lengthSquared()>0.01) {
            Vec3 normal=node.contactNormal.normalized();
            double vn=node.velocity.dot(normal),impact=incoming.dot(normal);
            Vec3 tangent=node.velocity-normal*vn;
            double normalChange=std::max(0.0,-impact)+std::max(0.0,vn-impact);
            double speed=tangent.length();
            if(speed>1e-12) tangent*=std::max(0.0,1-settings.friction*normalChange/speed);
            double bounce=impact<-0.75?-impact*settings.restitution:0;
            node.velocity=tangent+normal*std::max({0.0,vn,bounce});
        }
        node.velocity=limited(node.velocity,35);
        if(!node.position.finite()||!node.velocity.finite()||node.position.lengthSquared()>1e6)
            throw std::runtime_error("Numerical instability detected; reset the scene");
    }
    if(settings.rigid())rigidVelocity();
}
void SoftBody::impulse(Vec3 delta) { if(!delta.finite()) throw std::invalid_argument("Non-finite impulse"); for(auto& n:nodes_) n.velocity=limited(n.velocity+delta,35); }
Vec3 SoftBody::center() const { Vec3 c{}; for(const auto& n:nodes_) c+=n.position; return c/static_cast<double>(nodes_.size()); }
BodyStats SoftBody::stats() const {
    BodyStats s; s.minTetRatio=1e10; double volume=0;
    for(const auto& t:volumes_) { double v=signedVolume(nodes_[t.nodes[0]].position,nodes_[t.nodes[1]].position,nodes_[t.nodes[2]].position,nodes_[t.nodes[3]].position); volume+=v; s.minTetRatio=std::min(s.minTetRatio,v/t.rest); }
    s.volumeRatio=volume/restVolume_;
    for(const auto& n:nodes_) { s.finite=s.finite&&n.position.finite()&&n.velocity.finite(); s.maxSpeed=std::max(s.maxSpeed,n.velocity.length()); }
    return s;
}
Vec3 SoftBody::sample(Vec3 coordinates,double alpha) const {
    Vec3 p{std::clamp(coordinates.x,0.0,1.0)*(resolution_-1),std::clamp(coordinates.y,0.0,1.0)*(resolution_-1),std::clamp(coordinates.z,0.0,1.0)*(resolution_-1)};
    int x=std::min(static_cast<int>(p.x),resolution_-2),y=std::min(static_cast<int>(p.y),resolution_-2),z=std::min(static_cast<int>(p.z),resolution_-2);
    p=p-Vec3{double(x),double(y),double(z)}; Vec3 result{};
    for(int dz=0;dz<2;dz++) for(int dy=0;dy<2;dy++) for(int dx=0;dx<2;dx++) {
        const auto& n=nodes_[index(x+dx,y+dy,z+dz)];
        result+=lerp(n.framePrevious,n.position,alpha)*((dx?p.x:1-p.x)*(dy?p.y:1-p.y)*(dz?p.z:1-p.z));
    }
    return result;
}
}
