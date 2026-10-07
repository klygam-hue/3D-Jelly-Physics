#pragma once
#include "jely/math/Vec3.hpp"

namespace jely {
struct HsvColor {
    double hue{},saturation{},brightness{};
    Vec3 rgb() const;
    bool operator==(const HsvColor&) const=default;
};
struct LightingState {
    Vec3 direction,keyColor,ambientTop,ambientBottom,backgroundTop,backgroundBottom;
    double keyIntensity{},rimIntensity{};
};
struct Appearance {
    bool operator==(const Appearance&) const=default;
    double timeOfDay=14.0,contrast=1.25;
    HsvColor background{218,65,15},platform{218,28,23};
    bool grid=true,ring=true;
    double groundWidth=16,groundDepth=16,groundRoughness=0.6;
    double groundPatternScale=1,groundPatternStrength=0.55,groundRingRadius=2.4;
    int groundPattern=0; // 0 grid, 1 checker tiles; grid=false selects a plain surface.
    bool groundOutline=true;
    double transparency=65,refraction=0.65,gloss=0.8,tintStrength=0.6;
    HsvColor jellyColor{164,90,80};
    bool cycleTime=false;
    void setMaterialPreset(int preset);
    void resetMaterial();
    void resetEnvironment();
    void resetGround();
    void advanceTime(double seconds);
    void setTheme(int theme);
    void validate() const;
    LightingState lighting() const;
};
}
