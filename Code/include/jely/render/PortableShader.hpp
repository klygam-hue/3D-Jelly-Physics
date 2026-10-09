#pragma once
#include "jely/render/Backend.hpp"
#include "raylib.h"

namespace jely {
// Desktop keeps GLSL 330; mobile uses the same shader body with an ES 3 header.
inline Shader loadPortableShader(const char* vertex,const char* fragment) {
#ifdef JELY_GLES
    const auto v=vertex?GraphicsRuntime::shaderSource(vertex,true):std::string{};
    const auto f=fragment?GraphicsRuntime::shaderSource(fragment,true):std::string{};
    return LoadShaderFromMemory(vertex?v.c_str():nullptr,fragment?f.c_str():nullptr);
#else
    return LoadShaderFromMemory(vertex,fragment);
#endif
}
}
