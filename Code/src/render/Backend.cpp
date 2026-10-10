#include "jely/render/Backend.hpp"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <cctype>
#include <chrono>
#include <thread>
#include <cstdlib>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <shlobj.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <signal.h>
#include <pwd.h>
#include <cerrno>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
extern char** environ;
#endif
extern "C" void* glfwGetEGLDisplay(void);

namespace {
// Reuse an existing project cache after the product rename; new installations
// create the current brand's directory. No legacy data is moved or removed.
std::filesystem::path brandedDataDirectory(const std::filesystem::path& parent,bool portable=false) {
    const auto current=parent/(portable?"3D_Jelly_PhysicsData":"3D_Jelly_Physics");
    const auto legacy=parent/(portable?"3D_JelyData":"3D_Jely");
    std::error_code error;
    if(!std::filesystem::exists(current,error)&&std::filesystem::is_directory(legacy,error))return legacy;
    return current;
}
// Only the main thread changes this gateway, with the old context fully destroyed.
bool useVulkan=false;
using ShaderSource=void(*)(unsigned int,int,const char*const*,const int*);
using ClearDepth=void(*)(float);
ShaderSource angleShaderSource=nullptr;
ClearDepth angleClearDepth=nullptr;
using TexParameter=void(*)(unsigned,int,const int*);
using GetError=unsigned(*)();
TexParameter angleTexParameter=nullptr;
GetError realGetError=nullptr;
unsigned int graphicsErrors=0;
std::string angleEglPath,angleGlesPath;
unsigned int getErrorAdapter(){auto error=realGetError();if(error)++graphicsErrors;return error;}
void texParameterAdapter(unsigned target,int name,const int* parameters) {
    if(name==0x8e46){for(int i=0;i<4;i++)angleTexParameter(target,0x8e42+i,parameters+i);}
    else angleTexParameter(target,name,parameters);
}
void clearDepthAdapter(double value){angleClearDepth(float(value));}
void shaderSourceAdapter(unsigned int shader,int count,const char*const* sources,const int* lengths) {
    try {
        std::string combined;
        for(int i=0;i<count;i++)if(sources[i])combined.append(sources[i],lengths&&lengths[i]>=0?std::size_t(lengths[i]):std::strlen(sources[i]));
        auto translated=jely::GraphicsRuntime::shaderSource(combined,true);
        const char* data=translated.c_str();angleShaderSource(shader,1,&data,nullptr);
    }catch(const std::exception& e){
        // Never throw through the C graphics ABI; compilation reports the failure normally.
        std::cerr<<"Shader adaptation failed: "<<e.what()<<'\n';angleShaderSource(shader,count,sources,lengths);
    }
}
#ifdef _WIN32
struct Resource {const unsigned char* bytes{};DWORD size{};};
[[maybe_unused]] Resource resource(int id) {
    auto module=GetModuleHandleW(nullptr);
    auto found=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));
    if(!found)throw std::runtime_error("Embedded graphics runtime resource missing");
    auto loaded=LoadResource(module,found);
    const auto* bytes=static_cast<const unsigned char*>(LockResource(loaded));
    const DWORD size=SizeofResource(module,found);
    if(!bytes||!size)throw std::runtime_error("Embedded graphics runtime resource empty");
    return {bytes,size};
}
std::string sha256(const unsigned char* bytes,std::size_t size) {
    BCRYPT_ALG_HANDLE algorithm{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Cannot initialize runtime integrity check");
    std::array<unsigned char,32> digest{};
    auto result=BCryptHash(algorithm,nullptr,0,const_cast<PUCHAR>(bytes),ULONG(size),digest.data(),ULONG(digest.size()));
    BCryptCloseAlgorithmProvider(algorithm,0);
    if(result<0)throw std::runtime_error("Runtime integrity check failed");
    constexpr char digits[]="0123456789ABCDEF";std::string output;output.reserve(64);
    for(auto byte:digest){output+=digits[byte>>4];output+=digits[byte&15];}
    return output;
}
std::filesystem::path appData() {
    PWSTR path{};
    // This is an unpackaged desktop app; keep one stable cache across sandbox sessions.
    if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,KF_FLAG_NO_PACKAGE_REDIRECTION,nullptr,&path)))throw std::runtime_error("Local application data directory unavailable");
    std::filesystem::path result(path);CoTaskMemFree(path);return brandedDataDirectory(result);
}
std::filesystem::path portableData() {
    std::array<wchar_t,32768> path{};
    auto size=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));
    if(!size||size>=path.size())throw std::runtime_error("Executable directory unavailable");
    return brandedDataDirectory(std::filesystem::path(path.data()).parent_path(),true);
}
std::filesystem::path writableData() {
    try{auto path=appData();std::filesystem::create_directories(path);return path;}
    catch(const std::exception&){auto path=portableData();std::filesystem::create_directories(path);return path;}
}
[[maybe_unused]] std::string fileHash(const std::filesystem::path& path,std::size_t expectedSize) {
    std::error_code error;
    if(std::filesystem::file_size(path,error)!=expectedSize||error)return {};
    std::ifstream file(path,std::ios::binary);if(!file)return {};
    std::vector<unsigned char> bytes(expectedSize);file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()));
    if(!file)return {};
    return sha256(bytes.data(),bytes.size());
}
void atomicWrite(const std::filesystem::path& path,const unsigned char* bytes,std::size_t size) {
    std::filesystem::create_directories(path.parent_path());
    auto temporary=path;temporary+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
    HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot write private graphics cache");
    DWORD written{};bool ok=WriteFile(file,bytes,DWORD(size),&written,nullptr)&&written==size&&FlushFileBuffers(file);
    CloseHandle(file);
    if(!ok||!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        const auto error=GetLastError();DeleteFileW(temporary.c_str());
        throw std::runtime_error("Cannot commit private graphics cache (Windows error "+std::to_string(error)+"): "+path.string());
    }
}
[[maybe_unused]] HMODULE loadChecked(const std::filesystem::path& path) {
    auto module=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module)throw std::runtime_error("Vulkan runtime dependency could not be loaded (Windows error "+std::to_string(GetLastError())+")");
    return module;
}
#else
std::filesystem::path executablePath() {
#ifdef __APPLE__
    std::uint32_t size=1024;std::vector<char> buffer(size);
    if(_NSGetExecutablePath(buffer.data(),&size)!=0){buffer.resize(size);if(_NSGetExecutablePath(buffer.data(),&size)!=0)throw std::runtime_error("Executable path unavailable");}
    return std::filesystem::canonical(buffer.data());
#else
    std::vector<char> buffer(1024);
    for(;;){auto size=readlink("/proc/self/exe",buffer.data(),buffer.size());if(size<0)throw std::runtime_error("Executable path unavailable");if(std::size_t(size)<buffer.size())return std::filesystem::path(std::string(buffer.data(),std::size_t(size)));if(buffer.size()>=65536)throw std::runtime_error("Executable path too long");buffer.resize(buffer.size()*2);}
#endif
}
std::filesystem::path appData() {
    std::filesystem::path base;
#ifndef __APPLE__
    if(const auto* xdg=std::getenv("XDG_CONFIG_HOME");xdg&&std::filesystem::path(xdg).is_absolute())return brandedDataDirectory(xdg);
#endif
    if(const auto* userHome=std::getenv("HOME");userHome&&std::filesystem::path(userHome).is_absolute())base=userHome;
    else {auto* entry=getpwuid(getuid());if(!entry||!entry->pw_dir)throw std::runtime_error("User data directory unavailable");base=entry->pw_dir;}
#ifdef __APPLE__
    return brandedDataDirectory(base/"Library"/"Application Support");
#else
    return brandedDataDirectory(base/".config");
#endif
}
std::filesystem::path portableData(){return brandedDataDirectory(executablePath().parent_path(),true);}
std::filesystem::path writableData(){try{auto path=appData();std::filesystem::create_directories(path);return path;}catch(const std::exception&){auto path=portableData();std::filesystem::create_directories(path);return path;}}
void atomicWrite(const std::filesystem::path& path,const unsigned char* bytes,std::size_t size) {
    std::filesystem::create_directories(path.parent_path());
    // mkstemp creates only our unique 0600 temporary; rename commits within the same directory.
    std::string temporary=path.string()+".tmp-XXXXXX";int file=mkstemp(temporary.data());
    if(file<0)throw std::runtime_error("Cannot write private graphics preference");
    std::size_t offset=0;bool ok=true;
    while(offset<size){auto written=write(file,bytes+offset,size-offset);if(written<0&&errno==EINTR)continue;if(written<=0){ok=false;break;}offset+=std::size_t(written);}
    if(fsync(file)!=0)ok=false;
    if(close(file)!=0)ok=false;
    if(!ok||rename(temporary.c_str(),path.c_str())!=0){unlink(temporary.c_str());throw std::runtime_error("Cannot commit private graphics preference");}
}
[[maybe_unused]] void* loadChecked(const std::filesystem::path& path) {
    auto* module=dlopen(path.c_str(),RTLD_NOW|RTLD_LOCAL);
    if(!module){const auto* error=dlerror();throw std::runtime_error(std::string("Cannot load packaged graphics runtime: ")+(error?error:"unknown loader error"));}
    return module;
}
#endif
}
extern "C" int jelyVulkanRequested(){return useVulkan?1:0;}
// GLFW must not accidentally load Mesa EGL/GLES when explicit ANGLE was requested.
extern "C" const char* jelyAngleLibraryPath(int client){if(!useVulkan)return nullptr;const auto& path=client?angleGlesPath:angleEglPath;return path.empty()?nullptr:path.c_str();}
extern "C" void* jelyGraphicsProcAddress(const char* name) {
    auto address=glfwGetProcAddress(name);
    if(!address&&std::strcmp(name,"glClearDepth")!=0)return nullptr;
    if(std::strcmp(name,"glGetError")==0){realGetError=reinterpret_cast<GetError>(address);return reinterpret_cast<void*>(getErrorAdapter);}
    if(useVulkan&&std::strcmp(name,"glShaderSource")==0){angleShaderSource=reinterpret_cast<ShaderSource>(address);return reinterpret_cast<void*>(shaderSourceAdapter);}
    if(useVulkan&&std::strcmp(name,"glClearDepth")==0){angleClearDepth=reinterpret_cast<ClearDepth>(glfwGetProcAddress("glClearDepthf"));return angleClearDepth?reinterpret_cast<void*>(clearDepthAdapter):nullptr;}
    if(useVulkan&&std::strcmp(name,"glTexParameteriv")==0){angleTexParameter=reinterpret_cast<TexParameter>(address);return reinterpret_cast<void*>(texParameterAdapter);}
    return reinterpret_cast<void*>(address);
}
namespace jely {
GraphicsRuntime::GraphicsRuntime()=default;
GraphicsRuntime::~GraphicsRuntime(){
#ifdef _WIN32
    if(egl_)FreeLibrary(static_cast<HMODULE>(egl_));
    if(gles_)FreeLibrary(static_cast<HMODULE>(gles_));
    if(loader_)FreeLibrary(static_cast<HMODULE>(loader_));
    if(featureOverrideSet_)SetEnvironmentVariableW(L"ANGLE_FEATURE_OVERRIDES_DISABLED",originalFeatures_.empty()?nullptr:originalFeatures_.c_str());
#else
    if(egl_)dlclose(egl_);
    if(gles_)dlclose(gles_);
    if(loader_)dlclose(loader_);
#endif
}
void GraphicsRuntime::installAngle() {
#if defined(_WIN32) && defined(JELY_HAS_ANGLE)
    if(egl_&&gles_&&loader_)return;
    struct Asset {int id;const wchar_t* name;const char* hash;};
    const Asset assets[]{
        {1001,L"libEGL.dll","AE32B2441B4AB85CD100FDEAA9DCFD99B1DA15E8FD41C8DE78512C69CB96F596"},
        {1002,L"libGLESv2.dll","B4F6D2158D1BD27DED6C65884EFE26005C45E6D15373FF1230A5DF2B6D2DFE00"},
        {1003,L"vulkan-1.dll","2D9509B6CAF660F6057BBEADADB8474E740772AB861C8489BADFAD18E76A4056"}};
    const auto folder=writableData()/L"runtime"/L"angle-electron-41.0.0-b4f6d2158d1bd27d";
    for(const auto& asset:assets) {
        auto data=resource(asset.id);
        if(sha256(data.bytes,data.size)!=asset.hash)throw std::runtime_error("Embedded Vulkan runtime integrity mismatch");
        auto path=folder/asset.name;
        if(fileHash(path,data.size)!=asset.hash)atomicWrite(path,data.bytes,data.size);
        if(fileHash(path,data.size)!=asset.hash)throw std::runtime_error("Cached Vulkan runtime integrity mismatch");
    }
    // The pinned ANGLE build resolves its Vulkan loader relative to its own DLL directory.
    HMODULE loader{},gles{},egl{};
    try{loader=loadChecked(folder/L"vulkan-1.dll");gles=loadChecked(folder/L"libGLESv2.dll");egl=loadChecked(folder/L"libEGL.dll");}
    catch(...){if(egl)FreeLibrary(egl);if(gles)FreeLibrary(gles);if(loader)FreeLibrary(loader);throw;}
    loader_=loader;gles_=gles;egl_=egl;
#elif defined(__linux__) && defined(JELY_HAS_ANGLE)
    if(egl_&&gles_&&loader_)return;
    const auto folder=executablePath().parent_path()/"runtime"/"angle";
    void* loader{};void* gles{};void* egl{};
    try{loader=loadChecked(folder/"libvulkan.so.1");gles=loadChecked(folder/"libGLESv2.so");egl=loadChecked(folder/"libEGL.so");}
    catch(...){if(egl)dlclose(egl);if(gles)dlclose(gles);if(loader)dlclose(loader);throw;}
    loader_=loader;gles_=gles;egl_=egl;
    angleEglPath=(folder/"libEGL.so").string();angleGlesPath=(folder/"libGLESv2.so").string();
#else
    throw std::runtime_error(std::string(vulkanUnavailableReason()));
#endif
}
void GraphicsRuntime::probeVulkan() {
#ifdef _WIN32
    if(probed_)return;
    std::array<wchar_t,32768> path{};auto size=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));
    if(!size||size>=path.size())throw std::runtime_error("Cannot locate Vulkan probe executable");
    std::wstring command=L"\""+std::wstring(path.data())+L"\" --backend vulkan --strict-backend --smoke 2 --probe-vulkan-child";
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
    PROCESS_INFORMATION process{};
    if(!CreateProcessW(path.data(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process))throw std::runtime_error("Cannot start isolated Vulkan driver probe");
    auto wait=WaitForSingleObject(process.hProcess,25000);DWORD exitCode{};
    if(wait!=WAIT_OBJECT_0){TerminateProcess(process.hProcess,1);WaitForSingleObject(process.hProcess,5000);}
    const bool gotCode=GetExitCodeProcess(process.hProcess,&exitCode);CloseHandle(process.hThread);CloseHandle(process.hProcess);
    if(wait!=WAIT_OBJECT_0||!gotCode||exitCode!=0)throw std::runtime_error("Isolated Vulkan driver probe failed (code "+std::to_string(exitCode)+"). OpenGL remains available.");
    probed_=true;
#else
    if(probed_)return;
    auto executable=executablePath().string();
    char backend[]="--backend",vulkan[]="vulkan",strict[]="--strict-backend",smoke[]="--smoke",frames[]="2",probe[]="--probe-vulkan-child";
    char* arguments[]{executable.data(),backend,vulkan,strict,smoke,frames,probe,nullptr};
    pid_t child{};if(posix_spawn(&child,executable.c_str(),nullptr,nullptr,arguments,environ)!=0)throw std::runtime_error("Cannot start isolated Vulkan driver probe");
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(25);int status{};
    for(;;){
        auto result=waitpid(child,&status,WNOHANG);
        if(result==child){if(!WIFEXITED(status)||WEXITSTATUS(status)!=0)throw std::runtime_error("Isolated Vulkan driver probe failed. OpenGL remains available.");break;}
        if(result<0&&errno!=EINTR)throw std::runtime_error("Cannot wait for isolated Vulkan driver probe");
        if(std::chrono::steady_clock::now()>=deadline){kill(child,SIGKILL);while(waitpid(child,&status,0)<0&&errno==EINTR){}throw std::runtime_error("Vulkan driver probe timed out. OpenGL remains available.");}
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    probed_=true;
#endif
}
void GraphicsRuntime::select(GraphicsApi api,bool probeChild) {
    if(api==GraphicsApi::Vulkan) {
        if(!vulkanBuilt())throw std::runtime_error(std::string(vulkanUnavailableReason()));
#ifdef _WIN32
        if(!featureOverrideSet_) {
            std::array<wchar_t,4096> previous{};GetEnvironmentVariableW(L"ANGLE_FEATURE_OVERRIDES_DISABLED",previous.data(),DWORD(previous.size()));
            originalFeatures_=previous.data();
            // The optional maintenance presentation path crashes on the tested AMD driver.
            // Use standard swapchain synchronization; no visual quality is reduced.
            auto disabled=originalFeatures_+L":supportsSwapchainMaintenance1:supportsSurfaceMaintenance1";
            SetEnvironmentVariableW(L"ANGLE_FEATURE_OVERRIDES_DISABLED",disabled.c_str());featureOverrideSet_=true;
        }
#endif
        if(!probeChild)probeVulkan();
        installAngle();
    }
    useVulkan=api==GraphicsApi::Vulkan;vulkanDeviceConfirmed_=false;graphicsErrors=0;
}
unsigned int GraphicsRuntime::errors() const {return graphicsErrors;}
void GraphicsRuntime::verifyContext(GraphicsApi requested) {
    auto window=glfwGetCurrentContext();if(!window)throw std::runtime_error("No current graphics context");
    using GetString=const unsigned char*(*)(unsigned);
    auto getString=reinterpret_cast<GetString>(glfwGetProcAddress("glGetString"));
    if(!getString)throw std::runtime_error("Graphics driver query unavailable");
    auto renderer=getString(0x1f01);if(!renderer)throw std::runtime_error("Graphics driver description unavailable");
    driver_=reinterpret_cast<const char*>(renderer);
    const bool gles=glfwGetWindowAttrib(window,GLFW_CLIENT_API)==GLFW_OPENGL_ES_API;
    if(!adequateContext(glfwGetWindowAttrib(window,GLFW_CONTEXT_VERSION_MAJOR),glfwGetWindowAttrib(window,GLFW_CONTEXT_VERSION_MINOR),gles))
        throw std::runtime_error("OpenGL 3.3 or OpenGL ES 3.0 is required: "+driver_);
    if(requested==GraphicsApi::OpenGL) {
        if(glfwGetWindowAttrib(window,GLFW_CLIENT_API)!=GLFW_OPENGL_API)throw std::runtime_error("Native OpenGL context verification failed");
        return;
    }
    std::string lower=driver_;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});
    if(driver_.find("Vulkan")==std::string::npos||softwareRenderer(lower)||!gles)
        throw std::runtime_error("Requested hardware Vulkan backend was not activated: "+driver_);
    using GetProc=void*(*)(const char*);
#ifdef _WIN32
    const auto rawProc=GetProcAddress(static_cast<HMODULE>(egl_),"eglGetProcAddress");GetProc proc{};
    static_assert(sizeof(proc)==sizeof(rawProc));std::memcpy(&proc,&rawProc,sizeof(proc));
#else
    auto proc=reinterpret_cast<GetProc>(dlsym(egl_,"eglGetProcAddress"));
#endif
    if(!proc)throw std::runtime_error("ANGLE EGL procedure query unavailable");
    auto queryDisplay=reinterpret_cast<unsigned(*)(void*,int,std::intptr_t*)>(proc("eglQueryDisplayAttribEXT"));
    auto queryDevice=reinterpret_cast<unsigned(*)(void*,int,std::intptr_t*)>(proc("eglQueryDeviceAttribEXT"));
    std::intptr_t device{},vkDevice{},vkPhysical{};
    if(queryDisplay&&queryDevice&&queryDisplay(glfwGetEGLDisplay(),0x322c,&device)&&
       queryDevice(reinterpret_cast<void*>(device),0x34ac,&vkDevice)&&queryDevice(reinterpret_cast<void*>(device),0x34ab,&vkPhysical))
        vulkanDeviceConfirmed_=vkDevice!=0&&vkPhysical!=0;
    if(!vulkanDeviceConfirmed_)throw std::runtime_error("ANGLE Vulkan device/physical-device handles could not be verified");
}
GraphicsApi GraphicsRuntime::loadPreference() const {
    try {
        for(const auto& folder:{appData(),portableData()}) {
            auto path=folder/L"graphics.txt";std::error_code error;
            if(std::filesystem::file_size(path,error)>64||error)continue;
            std::ifstream file(path);std::string value;std::getline(file,value);
            if(value=="vulkan")return GraphicsApi::Vulkan;
            if(value=="opengl")return GraphicsApi::OpenGL;
        }
    }catch(...){/* Missing/corrupt preferences never prevent a safe default startup. */}
    return GraphicsApi::OpenGL;
}
void GraphicsRuntime::savePreference(GraphicsApi api) const {
    const std::string value=api==GraphicsApi::Vulkan?"vulkan\n":"opengl\n";
    atomicWrite(writableData()/L"graphics.txt",reinterpret_cast<const unsigned char*>(value.data()),value.size());
}
std::string GraphicsRuntime::licenses() {
#if defined(_WIN32) && defined(JELY_HAS_ANGLE)
    auto data=resource(1004);return {reinterpret_cast<const char*>(data.bytes),data.size};
#elif defined(__linux__) && defined(JELY_HAS_ANGLE)
    std::ifstream file(executablePath().parent_path()/"runtime"/"angle"/"LICENSES.chromium.html",std::ios::binary);
    if(!file)throw std::runtime_error("Packaged ANGLE license notices missing");
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
#else
    return {};
#endif
}
}
