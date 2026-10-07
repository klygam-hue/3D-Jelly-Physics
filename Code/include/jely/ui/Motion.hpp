#pragma once
#include <algorithm>
#include <cmath>

namespace jely {
// Exact critically damped response, independent of render-frame frequency.
struct AnimatedValue {
    double value=0,velocity=0;
    void reset(double target) {value=target;velocity=0;}
    void advance(double target,double dt,bool reducedMotion=false,double frequency=20) {
        if(reducedMotion){reset(target);return;}
        if(!std::isfinite(dt)||dt<=0)return;
        const double offset=value-target,decay=std::exp(-frequency*dt),term=velocity+frequency*offset;
        value=target+(offset+term*dt)*decay;
        velocity=(velocity-frequency*term*dt)*decay;
        if(std::abs(value-target)<1e-5&&std::abs(velocity)<1e-4)reset(target);
    }
};
struct UiPoint {float x{},y{};};
struct UiRect {
    float x{},y{},width{},height{};
    bool contains(UiPoint p) const {return p.x>=x&&p.x<x+width&&p.y>=y&&p.y<y+height;}
};
inline bool roundedContains(UiRect r,float radius,UiPoint p) {
    if(!r.contains(p))return false;
    radius=std::clamp(radius,0.0f,std::min(r.width,r.height)*0.5f);
    const float cx=std::clamp(p.x,r.x+radius,r.x+r.width-radius);
    const float cy=std::clamp(p.y,r.y+radius,r.y+r.height-radius);
    return (p.x-cx)*(p.x-cx)+(p.y-cy)*(p.y-cy)<=radius*radius;
}
struct MenuLayout {
    float progress=1;
    explicit MenuLayout(double open):progress(float(std::clamp(open,0.0,1.0))) {}
    float offset() const {return -340*(1-progress);}
    float reservedWidth() const {return 350*progress;}
    UiRect header() const {return {20,20,174+156*progress,62};}
    UiRect toggle() const {return {28,30,42,42};}
    UiRect body(float screenHeight) const {return {20+offset(),99,310,screenHeight-181};}
    bool blocks(UiPoint mouse,float screenHeight) const {
        return roundedContains(header(),28,mouse)||(progress>0.01f&&roundedContains(body(screenHeight),28,mouse));
    }
};
}
