#include "jely/scene/Appearance.hpp"
#include <stdexcept>

namespace jely {
Vec3 HsvColor::rgb() const {
    double h=std::fmod(hue,360.0)/60.0,s=saturation/100.0,v=brightness/100.0;
    if(h<0) h+=6;
    double c=v*s,x=c*(1-std::abs(std::fmod(h,2.0)-1)),m=v-c;
    Vec3 rgb;
    if(h<1)rgb={c,x,0};else if(h<2)rgb={x,c,0};else if(h<3)rgb={0,c,x};
    else if(h<4)rgb={0,x,c};else if(h<5)rgb={x,0,c};else rgb={c,0,x};
    return rgb+Vec3{m,m,m};
}
void Appearance::setTheme(int theme) {
    switch(theme) {
    case 0: background={218,65,15};platform={218,28,23};break;
    case 1: background={28,62,22};platform={32,35,38};break;
    case 2: background={225,8,17};platform={225,8,32};break;
    default:throw std::invalid_argument("Unknown appearance theme");
    }
}
void Appearance::validate() const {
    auto colorValid=[](const HsvColor& c){return std::isfinite(c.hue)&&c.hue>=0&&c.hue<=360&&std::isfinite(c.saturation)&&c.saturation>=0&&c.saturation<=100&&std::isfinite(c.brightness)&&c.brightness>=0&&c.brightness<=100;};
    auto unit=[](double value){return std::isfinite(value)&&value>=0&&value<=1;};
    auto range=[](double value,double low,double high){return std::isfinite(value)&&value>=low&&value<=high;};
    if(!range(groundWidth,6,32)||!range(groundDepth,6,32)||!unit(groundRoughness)||!range(groundPatternScale,0.25,4)||!unit(groundPatternStrength)||!range(groundRingRadius,0.5,10)||groundPattern<0||groundPattern>1)throw std::invalid_argument("Invalid ground settings");
    if(!std::isfinite(timeOfDay)||timeOfDay<0||timeOfDay>24||!std::isfinite(contrast)||contrast<0.8||contrast>2||!colorValid(background)||!colorValid(platform)||!colorValid(jellyColor)||!std::isfinite(transparency)||transparency<0||transparency>100||!unit(refraction)||!unit(gloss)||!unit(tintStrength))
        throw std::invalid_argument("Invalid appearance settings");
}
void Appearance::setMaterialPreset(int preset) {
    switch(preset){
    case 0:jellyColor={164,90,80};break;
    case 1:jellyColor={328,64,84};break;
    case 2:jellyColor={35,86,93};break;
    default:throw std::invalid_argument("Unknown jelly material preset");
    }
}
void Appearance::resetMaterial() {
    const Appearance defaults;
    transparency=defaults.transparency;refraction=defaults.refraction;gloss=defaults.gloss;tintStrength=defaults.tintStrength;jellyColor=defaults.jellyColor;
}
void Appearance::advanceTime(double seconds) {
    if(!std::isfinite(seconds)||seconds<0)throw std::invalid_argument("Invalid day cycle duration");
    if(cycleTime)timeOfDay=std::fmod(timeOfDay+seconds*0.2,24.0);
}
void Appearance::resetEnvironment() {
    const Appearance defaults;
    timeOfDay=defaults.timeOfDay;contrast=defaults.contrast;background=defaults.background;platform=defaults.platform;
    grid=defaults.grid;ring=defaults.ring;cycleTime=defaults.cycleTime;
    resetGround();
}
void Appearance::resetGround() {
    const Appearance defaults;platform=defaults.platform;grid=defaults.grid;ring=defaults.ring;
    groundWidth=defaults.groundWidth;groundDepth=defaults.groundDepth;groundRoughness=defaults.groundRoughness;
    groundPatternScale=defaults.groundPatternScale;groundPatternStrength=defaults.groundPatternStrength;groundRingRadius=defaults.groundRingRadius;
    groundPattern=defaults.groundPattern;groundOutline=defaults.groundOutline;
}
LightingState Appearance::lighting() const {
    validate();
    constexpr double pi=3.141592653589793;
    double angle=(timeOfDay-6)*pi/12,sunHeight=std::sin(angle);
    double daylight=std::clamp((sunHeight+0.12)/0.58,0.0,1.0);
    daylight=daylight*daylight*(3-2*daylight);
    double twilight=std::exp(-sunHeight*sunHeight/0.045)*daylight;
    LightingState result;
    // A continuous sun/moon orbit avoids a shadow jump when the automatic clock crosses the horizon.
    result.direction=Vec3{-std::cos(angle),std::max(0.20,std::abs(sunHeight)),0.45*sunHeight}.normalized();
    result.keyColor=lerp(Vec3{0.40,0.59,1.0},lerp(Vec3{1.0,0.94,0.83},Vec3{1.0,0.48,0.19},twilight),daylight);
    result.keyIntensity=0.42+2.8*daylight;
    result.rimIntensity=0.48-0.15*daylight;
    result.ambientTop=Vec3{0.19,0.23,0.31}*(0.35+0.65*daylight);
    result.ambientBottom=Vec3{0.025,0.036,0.055}*(0.6+0.4*daylight);
    result.backgroundTop=background.rgb()*(0.60+0.40*daylight);
    result.backgroundBottom=result.backgroundTop*0.32;
    return result;
}
}
