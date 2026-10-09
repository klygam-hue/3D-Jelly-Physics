#include "MobilePlatform.hpp"
#include "raylib.h"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#ifdef JELY_IPADOS
#include <SDL.h>
#include <OpenGLES/ES3/gl.h>
static unsigned int defaultFramebuffer=0,defaultRenderbuffer=0;
extern "C" unsigned int JelyDefaultFramebuffer(void){return defaultFramebuffer;}
// SDL/EAGL presents the bound drawable renderbuffer. Offscreen targets change
// that binding, so restore the captured color renderbuffer immediately at swap.
extern "C" void JelyPreparePresent(void){if(defaultRenderbuffer)glBindRenderbuffer(GL_RENDERBUFFER,defaultRenderbuffer);}
#else
#include <android_native_app_glue.h>
#include <android/configuration.h>
#include <GLES3/gl3.h>
extern "C" android_app* GetAndroidApp();
#endif
namespace jely::mobile {
void captureFramebuffer() {
#ifdef JELY_IPADOS
    GLint framebuffer=0;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&framebuffer);defaultFramebuffer=static_cast<unsigned>(framebuffer);
    GLint renderbuffer=0;glGetIntegerv(GL_RENDERBUFFER_BINDING,&renderbuffer);defaultRenderbuffer=static_cast<unsigned>(renderbuffer);
#endif
}
bool active() {
#ifdef JELY_IPADOS
    return (SDL_GetWindowFlags(static_cast<SDL_Window*>(GetWindowHandle()))&SDL_WINDOW_INPUT_FOCUS)!=0;
#else
    return IsWindowFocused()&&!IsWindowMinimized();
#endif
}
int smokeFrames(int argc,char** argv) {
    for(int i=1;i+1<argc;i++)if(std::string(argv[i])=="--smoke")return std::clamp(std::atoi(argv[i+1]),2,600);
#ifdef __ANDROID__
    auto* app=GetAndroidApp();JNIEnv* env=nullptr;
    if(app->activity->vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return 0;
    auto activity=app->activity->clazz;auto cls=env->GetObjectClass(activity);
    auto getIntent=env->GetMethodID(cls,"getIntent","()Landroid/content/Intent;");
    auto intent=env->CallObjectMethod(activity,getIntent);auto intentCls=env->GetObjectClass(intent);
    auto extra=env->GetMethodID(intentCls,"getIntExtra","(Ljava/lang/String;I)I");
    auto key=env->NewStringUTF("jely_smoke_frames");int result=env->CallIntMethod(intent,extra,key,0);
    if(env->ExceptionCheck()){env->ExceptionClear();result=0;}
    env->DeleteLocalRef(key);env->DeleteLocalRef(intentCls);env->DeleteLocalRef(intent);env->DeleteLocalRef(cls);app->activity->vm->DetachCurrentThread();
    return result>0?std::clamp(result,2,600):0;
#else
    return 0;
#endif
}
std::string storagePath() {
#ifdef JELY_IPADOS
    char* path=SDL_GetPrefPath("3djely","mobile");std::string result=path?path:"./";SDL_free(path);return result;
#else
    const std::string path=std::string(GetAndroidApp()->activity->internalDataPath)+"/";
    std::filesystem::create_directories(path);
    return path;
#endif
}
float uiScale() {
#ifdef __ANDROID__
    const int density=AConfiguration_getDensity(GetAndroidApp()->config);
    if(density>0&&density<1000)return std::clamp(density/160.0f,1.0f,5.0f);
#endif
    return 1.0f;
}
std::string verifyContext() {
    GLint major=0;glGetIntegerv(GL_MAJOR_VERSION,&major);
    const auto* version=glGetString(GL_VERSION);const auto* driver=glGetString(GL_RENDERER);
    if(major<3||!version||!driver)throw std::runtime_error("OpenGL ES 3.0 is required");
    return std::string(reinterpret_cast<const char*>(version))+" / "+reinterpret_cast<const char*>(driver);
}
unsigned int graphicsErrors() {
    unsigned int count=0;GLenum error=GL_NO_ERROR;
    while(count<32&&(error=glGetError())!=GL_NO_ERROR){
#ifdef JELY_IPADOS
        static unsigned reported=0;
        if(reported++<12)std::fprintf(stderr,"Mobile GL error: 0x%04x\n",static_cast<unsigned>(error));
#endif
        ++count;
    }
    return count;
}
}
