#pragma once

namespace jely::shaders {
inline constexpr const char* vertex=R"GLSL(#version 330
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 lightVP;
out vec3 worldPosition;
out vec3 worldNormal;
out vec4 lightPosition;
void main() {
    vec4 world=matModel*vec4(vertexPosition,1.0);
    worldPosition=world.xyz;
    worldNormal=normalize((matNormal*vec4(vertexNormal,0.0)).xyz);
    lightPosition=lightVP*world;
    gl_Position=mvp*vec4(vertexPosition,1.0);
}
)GLSL";
inline constexpr const char* fragment=R"GLSL(#version 330
in vec3 worldPosition;
in vec3 worldNormal;
in vec4 lightPosition;
out vec4 finalColor;
uniform vec4 baseColor;
uniform vec3 eye;
uniform vec3 lightDirection;
uniform sampler2D shadowMap;
uniform float jelly;
uniform float ground;
uniform vec3 keyColor;
uniform vec3 ambientTop;
uniform vec3 ambientBottom;
uniform vec3 fogColor;
uniform float keyIntensity;
uniform float rimIntensity;
uniform float contrast;
uniform float showGrid;
uniform float showRing;
uniform sampler2D sceneColor;
uniform sampler2D backDepth;
uniform mat4 viewMatrix;
uniform vec2 viewportSize;
uniform float transparency;
uniform float refractionStrength;
uniform float gloss;
uniform float tintStrength;
uniform vec2 groundHalfSize;
uniform float groundRoughness;
uniform float groundPattern;
uniform float patternScale;
uniform float patternStrength;
uniform float groundRingRadius;
uniform float groundOutline;
float unpackDepth(vec4 d) {
    return dot(d,vec4(1.0/16777216.0,1.0/65536.0,1.0/256.0,1.0));
}
float visibility(vec3 N,vec3 L) {
    if(transparency>=1.0)return 1.0;
    vec3 p=lightPosition.xyz/lightPosition.w*0.5+0.5;
    if(p.x<0.0||p.x>1.0||p.y<0.0||p.y>1.0||p.z>1.0) return 1.0;
    float bias=max(0.000025,0.00010*(1.0-max(dot(N,L),0.0)));
    float sum=0.0;
    for(int x=-1;x<=1;x++) for(int y=-1;y<=1;y++) {
        float d=unpackDepth(texture(shadowMap,p.xy+vec2(x,y)/2048.0));
        sum+=p.z-bias<=d?1.0:0.0;
    }
    return mix(1.0,sum/9.0,1.0-transparency);
}
void main() {
    vec3 N=normalize(worldNormal),L=normalize(lightDirection),V=normalize(eye-worldPosition),H=normalize(L+V);
    float diffuse=max(dot(N,L),0.0),shadow=visibility(N,L);
    vec3 base=pow(baseColor.rgb,vec3(2.2));
    if(ground>0.5) {
        vec2 coord=worldPosition.xz/patternScale;
        vec2 fw=max(fwidth(coord),vec2(0.0001));
        vec2 line=abs(fract(coord-0.5)-0.5)/fw;
        float grid=1.0-min(min(line.x,line.y),1.0);
        if(groundPattern<0.5)base+=vec3(0.05)*grid*showGrid*patternStrength;
        else {
            // Integrate the square wave over the pixel footprint for stable distant tiles.
            vec2 width=max(fwidth(coord),vec2(0.0001));
            vec2 lo=coord-width*0.5,hi=coord+width*0.5;
            vec2 iLo=floor(lo*0.5)+max(fract(lo*0.5)*2.0-1.0,0.0);
            vec2 iHi=floor(hi*0.5)+max(fract(hi*0.5)*2.0-1.0,0.0);
            vec2 wave=clamp((iHi-iLo)/width,0.0,1.0)*2.0-1.0;
            base*=1.0+wave.x*wave.y*0.35*patternStrength*showGrid;
        }
        float ring=1.0-smoothstep(0.015,0.04,abs(length(worldPosition.xz)-groundRingRadius));
        base+=vec3(0.012,0.12,0.09)*ring*showRing;
        float edge=min(groundHalfSize.x-abs(worldPosition.x),groundHalfSize.y-abs(worldPosition.z));
        base+=vec3(0.025,0.10,0.08)*(1.0-smoothstep(0.03,0.09,edge))*groundOutline;
    }
    vec3 ambient=mix(ambientBottom,ambientTop,N.y*0.5+0.5);
    vec3 color=base*(ambient+keyColor*keyIntensity*diffuse*shadow);
    float fresnel=pow(1.0-max(dot(N,V),0.0),4.0);
    float specular=pow(max(dot(N,H),0.0),mix(mix(160.0,8.0,groundRoughness),mix(12.0,160.0,gloss*gloss),jelly));
    color+=keyColor*specular*shadow*mix(mix(0.35,0.015,groundRoughness),0.04+1.1*gloss,jelly)*keyIntensity;
    if(jelly>0.5) {
        // Approximate scattering supplements the transmitted scene and surface reflection.
        float wrapped=pow(max(dot(-N,L)*0.5+0.5,0.0),2.0);
        color+=base*wrapped*(0.12+keyIntensity*0.08)+vec3(0.18,0.43,0.50)*fresnel*rimIntensity;
        float contact=1.0-0.23*exp(-max(worldPosition.y,0.0)*3.0);
        color*=contact;
    }
    float fog=1.0-exp(-pow(length(eye-worldPosition)*0.022,2.0));
    // Filmic mapping retains bright highlights while leaving shadows visibly deep.
    color=clamp((color*(2.51*color+0.03))/(color*(2.43*color+0.59)+0.14),0.0,1.0);
    color=pow(color,vec3(1.0/2.2));
    color=clamp((color-0.5)*contrast+0.5,0.0,1.0);
    color=mix(color,fogColor,fog);
    if(jelly>0.5&&transparency>0.0) {
        if(worldPosition.y<0.0)discard;
        vec2 uv=gl_FragCoord.xy/viewportSize;
        float exitDepth=unpackDepth(texture(backDepth,uv))*128.0;
        float entryDepth=-(viewMatrix*vec4(worldPosition,1.0)).z;
        vec3 viewRay=normalize((viewMatrix*vec4(V,0.0)).xyz);
        float thickness=clamp((exitDepth-entryDepth)/max(abs(viewRay.z),0.2),0.03,5.0);
        if(exitDepth>127.0||exitDepth<entryDepth)thickness=1.0;
        vec2 bend=(viewMatrix*vec4(N,0.0)).xy;
        // Fade both tint and distortion to zero at 100% transparency.
        vec2 offset=bend*refractionStrength*transparency*(1.0-transparency)*thickness*18.0/viewportSize;
        vec2 sampleUv=clamp(uv+offset,0.5/viewportSize,1.0-0.5/viewportSize);
        vec3 transmitted=texture(sceneColor,sampleUv).rgb;
        vec3 absorption=exp(-(vec3(1.0)-baseColor.rgb)*thickness*tintStrength*(1.0-transparency));
        transmitted*=absorption;
        float surfaceWeight=clamp((1.0-transparency)*(1.0+0.8*fresnel*transparency),0.0,1.0);
        color=mix(transmitted,color,surfaceWeight);
    }
    finalColor=vec4(color,1.0);
}
)GLSL";
inline constexpr const char* shadowVertex=R"GLSL(#version 330
in vec3 vertexPosition;
uniform mat4 mvp;
void main(){gl_Position=mvp*vec4(vertexPosition,1.0);}
)GLSL";
inline constexpr const char* shadowFragment=R"GLSL(#version 330
out vec4 finalColor;
void main(){
    vec4 encoded=fract(gl_FragCoord.z*vec4(16777216.0,65536.0,256.0,1.0));
    finalColor=encoded-encoded.xxyz*vec4(0.0,1.0/256.0,1.0/256.0,1.0/256.0);
}
)GLSL";
inline constexpr const char* backVertex=R"GLSL(#version 330
in vec3 vertexPosition;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 viewMatrix;
out float eyeDepth;
void main(){
    eyeDepth=-(viewMatrix*matModel*vec4(vertexPosition,1.0)).z;
    gl_Position=mvp*vec4(vertexPosition,1.0);
}
)GLSL";
inline constexpr const char* backFragment=R"GLSL(#version 330
in float eyeDepth;
out vec4 finalColor;
void main(){
    float d=clamp(eyeDepth/128.0,0.0,0.999999);
    vec4 encoded=fract(d*vec4(16777216.0,65536.0,256.0,1.0));
    finalColor=encoded-encoded.xxyz*vec4(0.0,1.0/256.0,1.0/256.0,1.0/256.0);
}
)GLSL";
}
