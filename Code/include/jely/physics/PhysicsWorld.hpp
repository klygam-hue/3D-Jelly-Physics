#pragma once
#include "jely/physics/SoftBody.hpp"
#include "jely/scene/Scene.hpp"
#include <optional>
#include <cstdint>
#include <random>

namespace jely {
struct Grab {
    std::size_t body{},node{};Vec3 target,goal,offset;
    double lambdaX{},lambdaY{},lambdaZ{};
    struct Weight {std::size_t node;double weight;};
    std::array<Weight,32> patch{};
    std::size_t count{};
};
class PhysicsWorld {
public:
    static constexpr double fixedStep=1.0/120.0;
    static constexpr std::size_t maxBodies=16;
    PhysicsWorld();
    void reset(int preset=0,int resolution=5);
    bool spawnRandom(int resolution=5);
    bool resizeGround(double width,double depth);
    void seedSpawns(std::uint64_t seed){spawnRandom_=std::mt19937_64(seed);}
    std::uint64_t structureRevision() const {return structureRevision_;}
    void step();
    void impulse(Vec3 delta);
    void setGrab(std::size_t body,std::size_t node,Vec3 target);
    void moveGrab(Vec3 target);
    void releaseGrab() { grab_.reset(); }
    const std::optional<Grab>& grab() const { return grab_; }
    std::vector<SoftBody>& bodies() { return bodies_; }
    const std::vector<SoftBody>& bodies() const { return bodies_; }
    const Scene& scene() const { return scene_; }
    Scene& scene() { return scene_; }
    PhysicsSettings settings;
    std::size_t stepCount() const { return stepCount_; }
    bool sleeping() const { return sleeping_; }
private:
    void collideEnvironment(SoftBody& body);
    void collideBodies();
    void solveGrab(double h);
    void finishRigidContacts(SoftBody& body);
    void wake();
    void considerSleep();
    bool sleepingStateUnchanged() const;
    std::uint64_t environmentSignature() const;
    struct BroadphaseNode { std::size_t body,node; double x,radius; };
    Scene scene_;
    std::vector<SoftBody> bodies_;
    std::mt19937_64 spawnRandom_{std::random_device{}()};
    std::uint64_t structureRevision_{};
    std::vector<BroadphaseNode> broadphase_;
    std::optional<Grab> grab_;
    std::size_t stepCount_{};
    bool sleeping_=false;
    std::size_t quietSteps_{};
    PhysicsSettings sleepSettings_;
    std::uint64_t sleepEnvironment_{};
    struct SleepNode {Vec3 position;double inverseMass;};
    std::vector<SleepNode> sleepNodes_;
};
}
