#pragma once
#include <string_view>
#include <initializer_list>

namespace jely {
enum class DesktopPlatform {Windows,Linux,MacOS,Other};
constexpr DesktopPlatform desktopPlatform() {
#if defined(_WIN32)
    return DesktopPlatform::Windows;
#elif defined(__APPLE__)
    return DesktopPlatform::MacOS;
#elif defined(__linux__)
    return DesktopPlatform::Linux;
#else
    return DesktopPlatform::Other;
#endif
}
constexpr bool supportsAngleVulkan(DesktopPlatform platform) {
    return platform==DesktopPlatform::Windows||platform==DesktopPlatform::Linux;
}
constexpr bool vulkanBuilt() {
#if defined(JELY_HAS_ANGLE)
    return true;
#else
    return false;
#endif
}
constexpr std::string_view vulkanUnavailableReason() {
    if(desktopPlatform()==DesktopPlatform::MacOS)
        return "macOS: this ANGLE backend has no Vulkan path. Native OpenGL is available; Metal is not Vulkan.";
    return "Vulkan was disabled for this build. Native OpenGL is available.";
}
// Compatibility is based on capabilities, never a GPU vendor allowlist.
constexpr bool adequateContext(int major,int minor,bool gles) {
    return major>3||(major==3&&minor>=(gles?0:3));
}
constexpr bool softwareRenderer(std::string_view lowercaseName) {
    for(auto name:{"swiftshader","llvmpipe","lavapipe","softpipe","software rasterizer"})
        if(lowercaseName.find(name)!=std::string_view::npos)return true;
    return false;
}
}
