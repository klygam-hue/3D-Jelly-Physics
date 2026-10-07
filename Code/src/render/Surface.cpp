#include "jely/render/Surface.hpp"
#include <map>
#include <stdexcept>

namespace jely {
Surface::Surface(int subdivisions) {
    if(subdivisions<4||subdivisions>64) throw std::invalid_argument("Surface subdivision out of range");
    std::map<std::array<int,3>,unsigned short> vertexMap;
    auto vertex=[&](std::array<int,3> p) {
        auto it=vertexMap.find(p); if(it!=vertexMap.end()) return it->second;
        auto id=static_cast<unsigned short>(coordinates_.size()); vertexMap.emplace(p,id);
        coordinates_.push_back({double(p[0])/subdivisions,double(p[1])/subdivisions,double(p[2])/subdivisions}); return id;
    };
    for(int axis=0;axis<3;axis++) for(int side=0;side<2;side++) {
        int u=(axis+1)%3,v=(axis+2)%3;
        for(int j=0;j<subdivisions;j++) for(int i=0;i<subdivisions;i++) {
            std::array<unsigned short,4> quad{};
            constexpr int du[]{0,1,1,0},dv[]{0,0,1,1};
            for(int k=0;k<4;k++) { std::array<int,3> p{}; p[axis]=side*subdivisions; p[u]=i+du[k]; p[v]=j+dv[k]; quad[k]=vertex(p); }
            if(side) indices_.insert(indices_.end(),{quad[0],quad[1],quad[2],quad[0],quad[2],quad[3]});
            else indices_.insert(indices_.end(),{quad[0],quad[2],quad[1],quad[0],quad[3],quad[2]});
        }
    }
    positions_.resize(coordinates_.size()); normals_.resize(coordinates_.size()); scratch_.resize(coordinates_.size());
    std::vector<std::vector<unsigned short>> adjacency(coordinates_.size());
    for(std::size_t i=0;i<indices_.size();i+=3) for(int j=0;j<3;j++) {
        auto a=indices_[i+j],b=indices_[i+(j+1)%3];
        adjacency[a].push_back(b); adjacency[b].push_back(a);
    }
    neighbourOffsets_.push_back(0);
    for(auto& n:adjacency) {
        std::sort(n.begin(),n.end());n.erase(std::unique(n.begin(),n.end()),n.end());
        neighbours_.insert(neighbours_.end(),n.begin(),n.end());neighbourOffsets_.push_back(neighbours_.size());
        inverseDegree_.push_back(1.0/static_cast<double>(n.size()));
    }
}
void Surface::bind(int resolution) {
    bindings_.resize(coordinates_.size());interpolatedNodes_.resize(static_cast<std::size_t>(resolution*resolution*resolution));
    for(std::size_t i=0;i<coordinates_.size();i++) {
        auto& binding=bindings_[i];binding.count=0;
        Vec3 p=coordinates_[i]*double(resolution-1);
        int x=std::min(static_cast<int>(p.x),resolution-2),y=std::min(static_cast<int>(p.y),resolution-2),z=std::min(static_cast<int>(p.z),resolution-2);
        p=p-Vec3{double(x),double(y),double(z)};
        for(int dz=0;dz<2;dz++)for(int dy=0;dy<2;dy++)for(int dx=0;dx<2;dx++) {
            double weight=(dx?p.x:1-p.x)*(dy?p.y:1-p.y)*(dz?p.z:1-p.z);
            if(weight==0)continue;
            if(binding.count>=4)throw std::logic_error("Surface binding must lie on a lattice boundary");
            binding.nodes[binding.count]=static_cast<std::size_t>(((z+dz)*resolution+y+dy)*resolution+x+dx);
            binding.weights[binding.count]=weight;++binding.count;
        }
    }
    boundResolution_=resolution;
}
void Surface::update(const SoftBody& body,double alpha) {
    if(boundResolution_!=body.resolution())bind(body.resolution());
    for(std::size_t i=0;i<body.nodes().size();i++)interpolatedNodes_[i]=lerp(body.nodes()[i].framePrevious,body.nodes()[i].position,alpha);
    for(std::size_t i=0;i<positions_.size();i++) {
        Vec3 position{};const auto& binding=bindings_[i];
        for(unsigned char k=0;k<binding.count;k++)position+=interpolatedNodes_[binding.nodes[k]]*binding.weights[k];
        positions_[i]=position;
    }
    // Taubin smoothing rounds the coarse interpolated surface without cumulative shrinkage.
    // Each frame starts from the current simulation: smoothing never feeds back into physics.
    for(int pass=0;pass<4;pass++) {
        double factor=pass%2==0?0.5:-0.53;
        for(std::size_t i=0;i<positions_.size();i++) {
            Vec3 avg{};for(std::size_t n=neighbourOffsets_[i];n<neighbourOffsets_[i+1];n++)avg+=positions_[neighbours_[n]];
            scratch_[i]=positions_[i]+(avg*inverseDegree_[i]-positions_[i])*factor;
        }
        positions_.swap(scratch_);
    }
    std::fill(normals_.begin(),normals_.end(),Vec3{});
    for(std::size_t i=0;i<indices_.size();i+=3) {
        auto a=indices_[i],b=indices_[i+1],c=indices_[i+2];
        Vec3 normal=(positions_[b]-positions_[a]).cross(positions_[c]-positions_[a]);
        normals_[a]+=normal; normals_[b]+=normal; normals_[c]+=normal;
    }
    for(auto& normal:normals_) normal=normal.normalized();
}
}
