#include <cstring>

#include "af/resources/Shaders.h"
#include "framework/af_test.hpp"

using namespace af;

AF_TEST("default shaders are non-empty GLSL 330 sources") {
    const char* vs = DefaultVertexShader();
    const char* fs = DefaultFragmentShader();
    AF_CHECK(vs != nullptr);
    AF_CHECK(fs != nullptr);
    AF_CHECK(std::strlen(vs) > 0);
    AF_CHECK(std::strlen(fs) > 0);
    AF_CHECK(std::strstr(vs, "#version 330 core") != nullptr);
    AF_CHECK(std::strstr(fs, "#version 330 core") != nullptr);
    AF_CHECK(std::strstr(vs, "void main") != nullptr);
    AF_CHECK(std::strstr(fs, "void main") != nullptr);
}

AF_TEST("vertex shader declares the expected inputs and uniforms") {
    const char* vs = DefaultVertexShader();
    AF_CHECK(std::strstr(vs, "aPosition") != nullptr);
    AF_CHECK(std::strstr(vs, "aNormal") != nullptr);
    AF_CHECK(std::strstr(vs, "uModel") != nullptr);
    AF_CHECK(std::strstr(vs, "uViewProjection") != nullptr);
    AF_CHECK(std::strstr(vs, "uNormalMatrix") != nullptr);
}

AF_TEST("fragment shader implements the phong model terms") {
    const char* fs = DefaultFragmentShader();
    AF_CHECK(std::strstr(fs, "diffuse") != nullptr);
    AF_CHECK(std::strstr(fs, "specular") != nullptr);
    AF_CHECK(std::strstr(fs, "uAmbientStrength") != nullptr);
}
