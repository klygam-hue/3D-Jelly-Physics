#include "jely/ui/Panel.hpp"
#include "EmbeddedFont.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace jely {
namespace {
constexpr Color ink{244,248,253,255},muted{187,201,219,255},accent{118,244,218,255};
}
Panel::Panel() {
    font_=LoadFontFromMemory(".ttf",embeddedFont,static_cast<int>(sizeof(embeddedFont)),36,nullptr,0);
    ownsFont_=font_.texture.id!=0&&font_.texture.id!=GetFontDefault().texture.id;
    if(!ownsFont_) font_=GetFontDefault();
    SetTextureFilter(font_.texture,TEXTURE_FILTER_BILINEAR);
    if(ownsFont_) {
        previousShapesTexture_=GetShapesTexture();previousShapesRectangle_=GetShapesTextureRectangle();
        // The pinned raylib font builder reserves a 3x3 white patch here.
        // Its center texel lets text and UI shapes share the same batch safely.
        SetShapesTexture(font_.texture,{float(font_.texture.width-2),float(font_.texture.height-2),1,1});
    }
    glyphIndex_.fill(GetGlyphIndex(font_,'?'));
    for(int i=0;i<font_.glyphCount;i++)if(font_.glyphs[i].value>=0&&font_.glyphs[i].value<128)glyphIndex_[font_.glyphs[i].value]=i;
}
Panel::~Panel() { if(ownsFont_) {SetShapesTexture(previousShapesTexture_,previousShapesRectangle_);UnloadFont(font_);} }
void Panel::text(const char* value,float x,float y,float size,Color color) const {
    x+=offsetX_;color=this->color(color);
    for(const unsigned char* c=reinterpret_cast<const unsigned char*>(value);*c;c++)if(*c>=128||*c=='\n'||*c=='\r'){DrawTextEx(font_,value,{x,y},size,0,color);return;}
    const float scale=size/font_.baseSize,pad=float(font_.glyphPadding);float offset=0;
    for(const unsigned char* c=reinterpret_cast<const unsigned char*>(value);*c;c++) {
        int index=glyphIndex_[*c];const auto& glyph=font_.glyphs[index];const auto& rectangle=font_.recs[index];
        if(*c!=' '&&*c!='\t') {
            Rectangle source{rectangle.x-pad,rectangle.y-pad,rectangle.width+2*pad,rectangle.height+2*pad};
            Rectangle destination{x+offset+glyph.offsetX*scale-pad*scale,y+glyph.offsetY*scale-pad*scale,source.width*scale,source.height*scale};
            DrawTexturePro(font_.texture,source,destination,{0,0},0,color);
        }
        offset+=(glyph.advanceX==0?rectangle.width:float(glyph.advanceX))*scale;
    }
}
Vector2 Panel::measure(const char* value,float size) const {
    float width=0;
    for(const unsigned char* c=reinterpret_cast<const unsigned char*>(value);*c;c++) {
        if(*c>=128||*c=='\n'||*c=='\r')return MeasureTextEx(font_,value,size,0);
        int index=glyphIndex_[*c];const auto& glyph=font_.glyphs[index];
        width+=glyph.advanceX>0?float(glyph.advanceX):font_.recs[index].width+glyph.offsetX;
    }
    return {width*(size/font_.baseSize),size};
}
Color Panel::color(Color value) const {value.a=static_cast<unsigned char>(std::clamp(float(value.a)*opacity_,0.0f,255.0f));return value;}
Rectangle Panel::translated(Rectangle r) const {r.x+=offsetX_;return r;}
void Panel::rounded(Rectangle r,float radius,Color value) const {DrawRectangleRounded(translated(r),2*radius/std::min(r.width,r.height),12,color(value));}
float Panel::width(const UiState&) const {return MenuLayout(menu_.value).reservedWidth();}
Rectangle Panel::toggleBounds() const {auto r=MenuLayout(menu_.value).toggle();return {r.x,r.y,r.width,r.height};}
bool Panel::containsMouse(const UiState& state) const {
    const UiPoint mouse{input_.mouse.x,input_.mouse.y};
    const float sw=float(GetScreenWidth()),sh=float(GetScreenHeight());
    const MenuLayout layout(menu_.value);
    const float center=layout.reservedWidth()+(sw-layout.reservedWidth())*0.5f;
    return (activeSlider_!=-1&&input_.down)||layout.blocks(mouse,sh)||
        UiRect{sw*0.5f-110,20,220,54}.contains(mouse)||
        UiRect{214,20,138,54}.contains(mouse)||
        roundedContains({sw-254,99,234,155},24,mouse)||roundedContains({sw-230,20,210,54},27,mouse)||
        roundedContains({center-315,sh-72,630,54},27,mouse)||
        (layout.progress>0.01f&&UiRect{20+layout.offset(),sh-65,310,42}.contains(mouse))||
        (!state.error.empty()&&UiRect{sw*0.5f-350,sh*0.5f-50,700,100}.contains(mouse));
}
void Panel::update(UiState& state,double dt,Input input) {
    input_=input;dt_=std::clamp(dt,0.0,0.1);reducedMotion_=state.reducedMotion;
    const auto layout=MenuLayout(menu_.value);
    const UiPoint mouse{input.mouse.x,input.mouse.y};
    const auto toggle=layout.toggle();
    if(input.pressed&&(roundedContains(toggle,21,mouse)||(state.hidden&&roundedContains(layout.header(),28,mouse))))state.hidden=!state.hidden;
    if(!input.down||state.hidden)activeSlider_=-1;
    menu_.advance(state.hidden?0:1,dt_,state.reducedMotion,18);
    if(lastTab_!=state.tab){lastTab_=state.tab;contentFade_.reset(state.reducedMotion?1:0);}
    tabIndicator_.advance(state.tab==PanelTab::Ground?1:static_cast<int>(state.tab),dt_,state.reducedMotion,24);
    contentFade_.advance(1,dt_,state.reducedMotion,22);
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}
void Panel::prepareBackdrop(Texture2D scene,std::uint64_t revision,const UiState& state) {glass_.prepare(scene,revision,!state.solidUi);}
void Panel::restoreLayout(const UiState& state) {
    menu_.reset(state.hidden?0:1);tabIndicator_.reset(state.tab==PanelTab::Ground?1:static_cast<int>(state.tab));contentFade_.reset(1);lastTab_=state.tab;
}
void Panel::selectTab(UiState& state,PanelTab tab) {
    if(state.tab==tab)return;
    state.tab=tab;lastTab_=tab;activeSlider_=-1;contentFade_.reset(state.reducedMotion?1:0);
}
bool Panel::button(Rectangle r,const char* label,bool selected,float fontSize,int animationId) {
    const bool hover=interactive_&&CheckCollisionPointRec(pointer(),r);
    // Responsive controls keep their identities across arbitrary window sizes.
    const int key=animationId!=0?animationId:int(r.x)+int(r.y)*1000;
    Response* response=nullptr;
    for(auto& candidate:responses_)if(candidate.key==key){response=&candidate;break;}
    if(!response)for(auto& candidate:responses_)if(candidate.key==-1){candidate.key=key;response=&candidate;break;}
    float h=hover?1:0,press=hover&&input_.down?1:0,selection=selected?1:0;
    if(response){
        response->hover.advance(h,dt_,reducedMotion_,26);response->press.advance(press,dt_,reducedMotion_,32);response->selection.advance(selection,dt_,reducedMotion_,22);
        h=float(response->hover.value);press=float(response->press.value);selection=float(response->selection.value);
    }
    Rectangle draw=r;draw.x+=press*1.5f;draw.y+=press;draw.width-=press*3;draw.height-=press*2;
    const float radius=std::min(draw.height*0.5f,16.0f);
    rounded(draw,radius,{226,242,255,static_cast<unsigned char>(15+20*h)});
    if(selection>0.001f)rounded(draw,radius,{120,243,215,static_cast<unsigned char>(60*selection)});
    DrawRectangleRoundedLinesEx(translated(draw),2*radius/std::min(draw.width,draw.height),12,1,color({228,246,255,static_cast<unsigned char>(28+45*h+35*selection)}));
    auto size=measure(label,fontSize);
    text(label,draw.x+(draw.width-size.x)/2,draw.y+(draw.height-size.y)/2,fontSize,selected?accent:ink);
    if(hover)SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    return hover&&input_.pressed;
}
bool Panel::slider(const char* name,double& value,double minimum,double maximum,float y,const char* format) {
    const double previousValue=value;
    int id=static_cast<int>(y);
    Rectangle hit{34,y+21,258,25}; Vector2 mouse=pointer();
    bool hover=interactive_&&CheckCollisionPointRec(mouse,hit);
    if(input_.pressed&&hover)activeSlider_=id;
    if(!input_.down||!interactive_)activeSlider_=-1;
    if(activeSlider_==id) value=minimum+(maximum-minimum)*std::clamp(double(mouse.x-38)/244.0,0.0,1.0);
    text(name,36,y,15,muted);
    char number[48];std::snprintf(number,sizeof(number),format,value);
    float w=measure(number,15).x;text(number,288-w,y,15,ink);
    rounded({38,y+29,244,6},3,{225,238,253,55});
    float t=float((value-minimum)/(maximum-minimum));
    rounded({38,y+29,std::max(6.0f,244*t),6},3,accent);
    const Vector2 thumb{offsetX_+38+244*t,y+32};
    if(hover||activeSlider_==id)DrawCircleV(thumb,12,color({175,255,238,35}));
    DrawCircleV({thumb.x,thumb.y+2},8,color({0,0,0,50}));
    DrawCircleV(thumb,activeSlider_==id?9.0f:8.0f,color({247,255,253,255}));
    DrawCircleV({thumb.x-2,thumb.y-2},2,color(WHITE));
    if(hover||activeSlider_==id)SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    return value!=previousValue;
}
void Panel::drawAppearance(Appearance& a) {
    const char* themes[]{"Ocean","Sand","Slate"};
    for(int i=0;i<3;i++) if(button({36+86.0f*i,167,78,30},themes[i])) a.setTheme(i);
    int minutes=static_cast<int>(std::lround(a.timeOfDay*60))%1440;
    char clock[64];std::snprintf(clock,sizeof(clock),"Time of day  %02d:%02d",minutes/60,minutes%60);
    slider(clock,a.timeOfDay,0,24,217,"%.1f h");
    const char* times[]{"Night","Dawn","Day","Sunset"};
    constexpr double hours[]{0,6.4,12,17.6};
    for(int i=0;i<4;i++) if(button({36+65.0f*i,267,59,30},times[i],std::abs(a.timeOfDay-hours[i])<0.1)) a.timeOfDay=hours[i];
    slider("Contrast",a.contrast,0.8,2.0,313,"%.2fx");
    auto swatch=[&](const char* title,const HsvColor& c,float y) {
        text(title,36,y,13,accent);Vec3 rgb=c.rgb();
        Color color{static_cast<unsigned char>(rgb.x*255),static_cast<unsigned char>(rgb.y*255),static_cast<unsigned char>(rgb.z*255),255};
        rounded({257,y-2,28,18},7,color);
    };
    swatch("BACKGROUND",a.background,371);
    slider("Hue",a.background.hue,0,360,396,"%.0f deg");
    slider("Saturation",a.background.saturation,0,100,444,"%.0f%%");
    slider("Brightness",a.background.brightness,0,100,492,"%.0f%%");
    swatch("PLATFORM",a.platform,550);
    slider("Hue",a.platform.hue,0,360,574,"%.0f deg");
    slider("Saturation",a.platform.saturation,0,100,622,"%.0f%%");
    slider("Brightness",a.platform.brightness,0,100,670,"%.0f%%");
    if(button({36,730,78,30},"Grid",a.grid)) a.grid=!a.grid;
    if(button({122,730,78,30},"Ring",a.ring)) a.ring=!a.ring;
    if(button({208,730,78,30},"Auto day",a.cycleTime,13)) a.cycleTime=!a.cycleTime;
    if(button({36,775,250,28},"Reset scene look")) a.resetEnvironment();
}
void Panel::drawGround(UiState& state) {
    auto& a=state.appearance;
    text("GROUND / PLATFORM",36,162,13,accent);
    const char* styles[]{"Plain","Grid","Tiles"};
    for(int i=0;i<3;i++)if(button({36+86.0f*i,192,78,32},styles[i],i==0?!a.grid:(a.grid&&a.groundPattern==i-1))){a.grid=i!=0;if(i!=0)a.groundPattern=i-1;}
    slider("Width",a.groundWidth,6,32,237,"%.1f m");
    slider("Depth",a.groundDepth,6,32,287,"%.1f m");
    slider("Roughness",a.groundRoughness,0,1,337,"%.2f");
    slider("Pattern scale",a.groundPatternScale,0.25,4,387,"%.2f m");
    slider("Pattern strength",a.groundPatternStrength,0,1,437,"%.2f");
    slider("Color hue",a.platform.hue,0,360,507,"%.0f deg");
    slider("Saturation",a.platform.saturation,0,100,555,"%.0f%%");
    slider("Brightness",a.platform.brightness,0,100,603,"%.0f%%");
    if(button({36,660,121,30},"Ring",a.ring))a.ring=!a.ring;
    if(button({166,660,120,30},"Outline",a.groundOutline))a.groundOutline=!a.groundOutline;
    slider("Ring radius",a.groundRingRadius,0.5,10,708,"%.1f m");
    if(button({36,775,250,28},"Reset ground"))a.resetGround();
    if(!state.groundMessage.empty())text("Size blocked: spread bodies / reset",36,753,10,{255,207,134,255});
}
void Panel::drawJelly(Appearance& a) {
    text("JELLY MATERIAL",36,162,13,accent);
    const char* names[]{"Mint","Berry","Honey"};
    for(int i=0;i<3;i++) {
        Appearance preset;preset.setMaterialPreset(i);
        if(button({36+86.0f*i,192,78,32},names[i],(preset.jellyColor.rgb()-a.jellyColor.rgb()).length()<0.01))a.setMaterialPreset(i);
    }
    slider("Transparency",a.transparency,0,100,252,"%.0f%%");
    slider("Refraction",a.refraction,0,1,314,"%.2f");
    slider("Gloss",a.gloss,0,1,376,"%.2f");
    slider("Tint strength",a.tintStrength,0,1,438,"%.2f");
    text("CUSTOM COLOR",36,506,13,accent);
    auto rgb=a.jellyColor.rgb();
    rounded({257,503,28,18},7,{static_cast<unsigned char>(rgb.x*255),static_cast<unsigned char>(rgb.y*255),static_cast<unsigned char>(rgb.z*255),255});
    slider("Hue",a.jellyColor.hue,0,360,534,"%.0f deg");
    slider("Saturation",a.jellyColor.saturation,0,100,588,"%.0f%%");
    slider("Brightness",a.jellyColor.brightness,0,100,642,"%.0f%%");
    text("0% solid / 100% invisible",36,706,13,muted);
    text("Shadow fades with transparency",36,729,12,muted);
    if(button({36,775,250,28},"Reset material"))a.resetMaterial();
}
void Panel::drawGraphics(UiState& state) {
    text("GRAPHICS API",36,167,13,accent);
    if(button({36,203,121,38},"OpenGL",state.graphicsApi==GraphicsApi::OpenGL,15)&&state.graphicsApi!=GraphicsApi::OpenGL){state.requestedApi=GraphicsApi::OpenGL;state.graphicsSwitchRequested=true;}
    if(button({166,203,120,38},vulkanBuilt()?"Vulkan":"Unavailable",state.graphicsApi==GraphicsApi::Vulkan,13)&&state.graphicsApi!=GraphicsApi::Vulkan){
        if(vulkanBuilt()){state.requestedApi=GraphicsApi::Vulkan;state.graphicsSwitchRequested=true;}
        else state.graphicsMessage=std::string(vulkanUnavailableReason());
    }
    text("Active renderer",36,272,13,muted);text(apiName(state.graphicsApi),36,296,20,ink);
    auto paragraph=[&](const std::string& value,float y,Color c) {
        std::size_t start=0;int line=0;
        while(start<value.size()&&line<8) {
            std::size_t end=std::min(start+36,value.size());
            if(end<value.size()){auto space=value.rfind(' ',end);if(space!=std::string::npos&&space>start)end=space;}
            char row[40];std::snprintf(row,sizeof(row),"%.*s",int(end-start),value.c_str()+start);text(row,36,y+19*line,12,c);
            start=end;while(start<value.size()&&value[start]==' ')++start;++line;
        }
    };
    paragraph(state.graphicsDriver,343,muted);
    text(vulkanBuilt()?"Vulkan uses hardware via ANGLE.":"Vulkan is not included on this target.",36,456,12,ink);
    text("Scene, shadows and UI use that API.",36,479,12,muted);
    text("OpenGL uses your native driver.",36,521,13,ink);
    text("Switching rebuilds GPU resources.",36,563,13,muted);
    text("Simulation and settings are preserved.",36,586,12,muted);
    text("Choice is saved for future launches.",36,621,12,muted);
    if(!state.graphicsMessage.empty())paragraph(state.graphicsMessage,658,{255,207,134,255});
}
void Panel::draw(PhysicsWorld& world,UiState& state) {
    float sw=float(GetScreenWidth()),sh=float(GetScreenHeight());
    offsetX_=0;opacity_=1;interactive_=true;
    const MenuLayout layout(menu_.value);
    const auto header=layout.header();
    glass_.draw({header.x,header.y,header.width,header.height},28,1,input_.mouse,state.solidUi);
    const bool toggleHover=CheckCollisionPointRec(input_.mouse,toggleBounds());
    rounded(toggleBounds(),21,{225,245,255,static_cast<unsigned char>(toggleHover?45:20)});
    for(int i=0;i<3;i++){
        const float y=43+6.0f*i;
        DrawLineEx({40,y},{58,y},1.8f,ink);
    }
    if(toggleHover)SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    text(state.hidden?"Controls":"3D Jely",82,30,23,ink);
    if(layout.progress>0.65f){opacity_=(layout.progress-0.65f)/0.35f;text("Soft body studio",82,58,12,muted);opacity_=1;}
    glass_.draw({sw-230,20,210,54},27,1,input_.mouse,state.solidUi);
    DrawCircleV({sw-204,47},4,state.paused?Color{240,181,79,255}:accent);
    char status[128];std::snprintf(status,sizeof(status),"%s  /  %d FPS",state.paused?"PAUSED":"LIVE",GetFPS());
    text(status,sw-190,38,15,ink);
    if(layout.progress>0.001f) {
        auto body=layout.body(sh);glass_.draw({body.x,body.y,body.width,body.height},28,layout.progress,input_.mouse,state.solidUi);
        offsetX_=layout.offset();opacity_=layout.progress;interactive_=!state.hidden&&layout.progress>0.85f;
        BeginScissorMode(0,99,350,int(sh-181));
        rounded({36+69*float(tabIndicator_.value),113,62,31},15,{145,250,226,38});
        constexpr const char* tabs[]{"Physics","Appearance","Jelly","Graphics"};
        for(int i=0;i<4;i++)if(button({36+69.0f*i,113,62,31},tabs[i],state.tab==static_cast<PanelTab>(i)||(i==1&&state.tab==PanelTab::Ground),i==1?10:12))selectTab(state,static_cast<PanelTab>(i));
        opacity_*=float(contentFade_.value);interactive_=interactive_&&contentFade_.value>0.8;
        if(state.tab==PanelTab::Jelly)drawJelly(state.appearance);
        else if(state.tab==PanelTab::Appearance) drawAppearance(state.appearance);
        else if(state.tab==PanelTab::Graphics)drawGraphics(state);
        else if(state.tab==PanelTab::Ground)drawGround(state);
        else {
        text("Scene",36,149,15,muted);
        const char* presets[]{"Drop","High drop","Duet"};
        for(int i=0;i<3;i++) if(button({36+86.0f*i,176,78,34},presets[i],state.preset==i)) { state.preset=i;state.resetRequested=true; }
        text("Material controls: Jelly tab",36,249,13,muted);
        double softness=world.settings.softness();
        // Opening a tab must not change physical parameters through log/pow rounding.
        if(slider("Softness",softness,0,100,310,"%.0f%%"))world.settings.setSoftness(softness);
        slider("Gravity",world.settings.gravity,0,20,368,"%.1f m/s2");
        slider("Damping",world.settings.damping,0,3,426,"%.2f");
        slider("Friction",world.settings.friction,0,1,484,"%.2f");
        text("Quality",36,546,15,muted);
        if(button({36,573,121,32},"Balanced",state.resolution==5)) { if(state.resolution!=5) {state.resolution=5;world.settings.iterations=8;state.resetRequested=true;} }
        if(button({166,573,120,32},"Detailed",state.resolution==7)) { if(state.resolution!=7) {state.resolution=7;world.settings.iterations=10;state.resetRequested=true;} }
        if(button({36,624,121,34},state.paused?"Resume":"Pause",state.paused)) state.paused=!state.paused;
        if(button({166,624,120,34},"Reset")) state.resetRequested=true;
        if(button({36,672,121,34},"Launch")) state.impulseRequested=true;
        if(button({166,672,120,34},"Debug",state.debug)) state.debug=!state.debug;
        }
        EndScissorMode();
        opacity_=layout.progress;interactive_=!state.hidden&&layout.progress>0.85f;
        if(button({20,sh-65,145,42},state.reducedMotion?"Motion off":"Motion on",!state.reducedMotion,13,MotionControl))state.reducedMotion=!state.reducedMotion;
        if(button({175,sh-65,145,42},state.solidUi?"Glass off":"Glass on",!state.solidUi,13,GlassControl))state.solidUi=!state.solidUi;
    }
    offsetX_=0;opacity_=1;interactive_=true;
    glass_.draw({214,20,138,54},27,1,input_.mouse,state.solidUi);
    if(button({214,20,138,54},"Ground",state.tab==PanelTab::Ground,16,GroundControl)){state.hidden=false;selectTab(state,PanelTab::Ground);}
    const auto spawn=spawnBounds();
    glass_.draw(spawn,27,1,input_.mouse,state.solidUi);
    char spawnLabel[64];std::snprintf(spawnLabel,sizeof(spawnLabel),"Spawn jelly  %zu/%zu",world.bodies().size(),PhysicsWorld::maxBodies);
    if(button(spawn,spawnLabel,false,15,SpawnControl))state.spawnRequested=true;
    if(!state.spawnMessage.empty())text(state.spawnMessage.c_str(),spawn.x,80,11,{255,207,134,255});
    std::size_t nodes=0,tets=0;
    for(const auto& body:world.bodies()){nodes+=body.nodes().size();tets+=body.tetrahedra().size();}
    if(GetTime()>=nextTelemetryUpdate_||nodes!=cachedNodes_||world.stepCount()<telemetryStep_) {
        double volume=0;for(const auto& body:world.bodies())volume+=body.stats().volumeRatio;
        cachedVolume_=volume/static_cast<double>(world.bodies().size());
        cachedNodes_=nodes;telemetryStep_=world.stepCount();nextTelemetryUpdate_=GetTime()+0.1;
    }
    const double volume=cachedVolume_;
    float statX=sw-254;
    glass_.draw({statX,99,234,155},24,1,input_.mouse,state.solidUi);
    text("Simulation",statX+18,116,17,ink);
    char row[128];std::snprintf(row,sizeof(row),"%zu nodes / %zu tets",nodes,tets);text(row,statX+16,142,14,ink);
    std::snprintf(row,sizeof(row),"Volume  %5.1f%%",volume*100);text(row,statX+16,166,15,accent);
    if(state.monitorHz>0)std::snprintf(row,sizeof(row),"Monitor %d Hz / %d FPS",state.monitorHz,GetFPS());else std::snprintf(row,sizeof(row),"Monitor Hz unavailable / %d FPS",GetFPS());
    text(row,statX+16,191,12,ink);
    std::snprintf(row,sizeof(row),"Physics 120 Hz / %.2f ms%s",state.physicsMs,world.sleeping()?" / sleep":"");text(row,statX+16,216,11,muted);
    text(apiName(state.graphicsApi),statX+16,237,10,muted);
    const char* help="Drag & throw     Orbit     Pan     Zoom";
    float center=width(state)+(sw-width(state))*0.5f;
    glass_.draw({center-315,sh-72,630,54},27,1,input_.mouse,state.solidUi);
    auto hw=measure(help,14).x;
    text(help,center-hw/2,sh-62,14,ink);
    const char* keys="LMB / RMB / MMB / Wheel  |  Space pause  R reset  J launch  B spawn  Tab menu";
    float kw=measure(keys,11).x;text(keys,center-kw/2,sh-41,11,muted);
    if(!state.error.empty()) {
        DrawRectangleRounded({sw*0.5f-350,sh*0.5f-50,700,100},0.12f,8,{99,35,43,245});
        text("Simulation stopped. Press R to reset.",sw*0.5f-328,sh*0.5f-33,18,ink);
        text(state.error.c_str(),sw*0.5f-328,sh*0.5f+2,13,{255,181,177,255});
    }
}
}
