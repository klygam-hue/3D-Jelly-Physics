#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include <stdexcept>
#include "jely/render/Compatibility.hpp"

namespace jely {
enum class GraphicsApi {OpenGL,Vulkan};
inline const char* apiName(GraphicsApi api){return api==GraphicsApi::Vulkan?"Vulkan (ANGLE)":"OpenGL";}
inline GraphicsApi parseApi(std::string_view value) {
    if(value=="opengl")return GraphicsApi::OpenGL;
    if(value=="vulkan")return GraphicsApi::Vulkan;
    throw std::invalid_argument("Graphics API must be opengl or vulkan");
}
// Owns the optional embedded ANGLE runtime; driver-specific context stays in GLFW.
class GraphicsRuntime {
public:
    GraphicsRuntime();
    ~GraphicsRuntime();
    GraphicsRuntime(const GraphicsRuntime&)=delete;
    GraphicsRuntime& operator=(const GraphicsRuntime&)=delete;
    void select(GraphicsApi api,bool probeChild=false);
    void verifyContext(GraphicsApi requested);
    void savePreference(GraphicsApi api) const;
    GraphicsApi loadPreference() const;
    const std::string& driver() const {return driver_;}
    bool vulkanDeviceConfirmed() const {return vulkanDeviceConfirmed_;}
    unsigned int errors() const;
    static std::string shaderSource(std::string_view source,bool vulkan) {
        std::string result(source);
        if(!vulkan)return result;
        auto version=result.find("#version 330");
        if(version==std::string::npos)throw std::invalid_argument("Unsupported shader version for Vulkan/ANGLE");
        auto end=result.find('\n',version);
        result.replace(version,end==std::string::npos?result.size()-version:end-version,"#version 300 es\nprecision highp float;\nprecision highp int;");
        return result;
    }
    static std::string licenses();
private:
    void installAngle();
    void probeVulkan();
    void* egl_{};
    void* gles_{};
    void* loader_{};
    std::string driver_;
    bool vulkanDeviceConfirmed_=false;
    bool probed_=false;
#ifdef _WIN32
    bool featureOverrideSet_=false;
    std::wstring originalFeatures_;
#endif
};
}
