#include "TouchInput.hpp"
#include <iostream>
#include <stdexcept>
using namespace jely::mobile;
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
int main() {
    try {
        TouchInput input;Finger f[]{{4,{100,100}},{9,{200,100}},{11,{150,150}}};
        auto t=input.advance(f,1,true);require(t.pressed&&t.down&&t.ui,"UI capture");
        f[0].position.x=130;t=input.advance(f,1,false);require(t.down&&t.ui&&t.delta.x==30&&!t.pressed,"UI ownership leaked");
        t=input.advance(f,0,false);require(t.released&&!t.down,"release");
        t=input.advance(f,1,false);require(t.pressed&&t.down&&!t.ui,"scene capture");
        t=input.advance(f,2,false);require(t.released&&!t.down&&t.wheel==0,"pinch must cancel grab without jump");
        f[0].position.x-=10;f[1].position.x+=10;t=input.advance(f,2,false);require(t.wheel>0&&t.orbit.x==0,"pinch zoom");
        t=input.advance(f,1,true);require(!t.pressed&&!t.down,"pinch-to-one generated accidental UI press");
        input.advance(f,0,false);t=input.advance(f,1,false);require(t.pressed,"new contact after release");
        f[0].id=70;t=input.advance(f,1,false);require(t.released&&!t.down&&!t.pressed,"finger replacement continued drag");
        input.cancel();input.advance(f,3,false);for(auto& finger:f){finger.position.x+=20;finger.position.y+=10;}
        t=input.advance(f,3,false);require(std::abs(t.pan.x-20)<0.001&&std::abs(t.pan.y-10)<0.001&&t.orbit.x==0,"three finger pan");
        input.advance(f,0,false);input.advance(f,2,false);std::swap(f[0],f[1]);t=input.advance(f,2,false);require(t.wheel==0&&t.orbit.x==0,"finger ordering caused jump");
        std::cout<<"PASS touch capture, cancellation, pinch, pan, identity and handover\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
