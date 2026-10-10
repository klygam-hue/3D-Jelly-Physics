#pragma once
#include "jely/math/Vec3.hpp"
#include <array>
#include <cstddef>
#include <vector>
#include <optional>

namespace jely {
struct PhysicsSettings {
    double gravity=9.81;
    double edgeCompliance=0.000025;
    double volumeCompliance=0.00000008;
    double damping=0.45;
    double friction=0.65;
    double restitution=0.12;
    int substeps=3;
    int iterations=8;
    bool sleepEnabled=true;
    bool operator==(const PhysicsSettings&) const=default;
    void validate() const;
    void setSoftness(double percent);
    double softness() const;
    bool rigid() const {return edgeCompliance==0;}
};
struct Node {
    Vec3 position, previous, framePrevious, velocity, rest;
    Vec3 contactNormal{};
    double inverseMass{};
    bool surface{};
};
struct DistanceConstraint { std::size_t a{},b{}; double rest{},lambda{}; };
struct VolumeConstraint { std::array<std::size_t,4> nodes{}; double rest{},lambda{},barrierLambda{}; };
struct BodyStats { double volumeRatio{}, maxSpeed{}, minTetRatio{}; bool finite=true; };
enum class ShapeKind {RoundedBox,Ellipsoid,Capsule,Pillow};
struct BodyShape {
    ShapeKind kind=ShapeKind::RoundedBox;
    Vec3 scale{1,1,1};
    double rounding=0.64,yaw=0;
};

class SoftBody {
public:
    SoftBody(Vec3 center,double size=1.8,int resolution=5,double mass=1.6,BodyShape shape={},std::optional<Vec3> color={});
    const BodyShape& shape() const {return shape_;}
    const std::optional<Vec3>& customColor() const {return color_;}
    void beginFrame();
    void integrate(double h,const PhysicsSettings& settings);
    void solve(double h,const PhysicsSettings& settings);
    void finish(double h,const PhysicsSettings& settings);
    void projectRigid();
    void impulse(Vec3 velocityChange);
    Vec3 center() const;
    BodyStats stats() const;
    std::size_t index(int x,int y,int z) const;
    Vec3 sample(Vec3 coordinates,double alpha=1.0) const;
    int resolution() const { return resolution_; }
    double size() const { return size_; }
    double particleRadius() const { return size_/(resolution_-1)*0.43; }
    double restVolume() const { return restVolume_; }
    std::vector<Node>& nodes() { return nodes_; }
    const std::vector<Node>& nodes() const { return nodes_; }
    const std::vector<DistanceConstraint>& edges() const { return edges_; }
    const std::vector<VolumeConstraint>& tetrahedra() const { return volumes_; }
    const std::vector<std::size_t>& surfaceNodes() const { return surfaceNodes_; }
private:
    void solveVolume(VolumeConstraint& tet,double alpha,bool barrier);
    void rigidVelocity();
    void guardInversion();
    int resolution_{};
    double size_{},restVolume_{};
    BodyShape shape_;
    std::optional<Vec3> color_;
    std::vector<Node> nodes_;
    std::vector<DistanceConstraint> edges_;
    std::vector<VolumeConstraint> volumes_;
    std::vector<std::vector<std::size_t>> nodeTets_;
    std::vector<Vec3> guardedPositions_;
    std::vector<double> guardedMinimum_;
    std::vector<std::size_t> surfaceNodes_;
    std::vector<double> masses_;
    Vec3 restMassCenter_;
    double mass_{};
};
}
