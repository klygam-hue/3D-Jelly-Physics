#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace jely::mobile {
struct Point {float x{},y{};};
struct Finger {int id{};Point position{};};
struct TouchFrame {
    Point pointer{},delta{},orbit{},pan{};
    float wheel{};
    bool pressed=false,down=false,released=false,ui=false;
};
// Ownership lasts for the entire contact. Adding a second finger cancels a grab;
// lifting it cannot start a new grab or operate a control underneath it.
class TouchInput {
public:
    TouchFrame advance(const Finger* fingers,int count,bool overUi) {
        count=std::clamp(count,0,8);TouchFrame out;
        if(count==0) {out.released=owner_!=Owner::Idle;out.pointer=previous_;owner_=Owner::Idle;return out;}
        Point center{};for(int i=0;i<count;i++){center.x+=fingers[i].position.x/count;center.y+=fingers[i].position.y/count;}
        const float separation=count>=2?std::hypot(fingers[0].position.x-fingers[1].position.x,fingers[0].position.y-fingers[1].position.y):0;
        if(count>=2) {
            out.released=owner_==Owner::Ui||owner_==Owner::Scene;
            if(owner_==Owner::Camera&&count==previousCount_&&sameIds(fingers,count)) {
                if(count>=3)out.pan={center.x-previous_.x,center.y-previous_.y};
                else {out.orbit={center.x-previous_.x,center.y-previous_.y};if(separation>1&&separation_>1)out.wheel=10*std::log(separation/separation_);}
            }
            owner_=Owner::Camera;out.pointer=center;
        } else if(owner_==Owner::Idle) {
            owner_=overUi?Owner::Ui:Owner::Scene;primary_=fingers[0].id;
            out.pressed=true;out.down=true;out.ui=overUi;out.pointer=center;
        } else if(owner_!=Owner::Camera&&fingers[0].id==primary_) {
            out.pointer=center;out.delta={center.x-previous_.x,center.y-previous_.y};out.down=true;out.ui=owner_==Owner::Ui;
        } else {
            out.released=owner_!=Owner::Camera;owner_=Owner::Camera;out.pointer=center;
        }
        previous_=center;separation_=separation;previousCount_=count;
        for(int i=0;i<count;i++)ids_[i]=fingers[i].id;
        return out;
    }
    void cancel(){owner_=Owner::Idle;previousCount_=0;}
private:
    enum class Owner {Idle,Ui,Scene,Camera};
    bool sameIds(const Finger* fingers,int count) const {
        for(int i=0;i<count;i++) {bool found=false;for(int j=0;j<count;j++)found=found||fingers[i].id==ids_[j];if(!found)return false;}
        return true;
    }
    Owner owner_=Owner::Idle;Point previous_{};float separation_=0;int primary_=0,previousCount_=0;std::array<int,8> ids_{};
};
}
