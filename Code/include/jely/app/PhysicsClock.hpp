#pragma once
#include "jely/physics/PhysicsWorld.hpp"
#include <algorithm>
#include <cmath>

namespace jely {
// Fixed 120 Hz physics, independent of rendering. Bound catch-up work after stalls.
class PhysicsClock {
public:
    int advance(PhysicsWorld& world,double seconds) {
        if(!std::isfinite(seconds)||seconds<=0)return 0;
        constexpr double maxFrameTime=0.1;
        constexpr int maxSteps=12;
        constexpr double tickEpsilon=1e-12;
        lostTime_+=std::max(0.0,seconds-maxFrameTime);
        remainder_+=std::min(seconds,maxFrameTime);
        int steps=0;
        // Twelve ticks cover a 100 ms frame (10 FPS); eight only cover 15 FPS.
        while(remainder_+tickEpsilon>=PhysicsWorld::fixedStep&&steps<maxSteps) {
            world.step();remainder_=std::max(0.0,remainder_-PhysicsWorld::fixedStep);++steps;
        }
        if(remainder_+tickEpsilon>=PhysicsWorld::fixedStep) {
            const double discarded=std::floor((remainder_+tickEpsilon)/PhysicsWorld::fixedStep)*PhysicsWorld::fixedStep;
            lostTime_+=discarded;remainder_=std::max(0.0,remainder_-discarded);
        }
        return steps;
    }
    void reset(){remainder_=0;}
    double alpha() const{return std::clamp(remainder_/PhysicsWorld::fixedStep,0.0,1.0);}
    double lostTime() const{return lostTime_;}
private:
    double remainder_=0,lostTime_=0;
};
}
