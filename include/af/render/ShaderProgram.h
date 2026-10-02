#pragma once
// ShaderProgram — compile + link a vertex/fragment pair, uniform setters.
// All GL interaction goes through af::gl (the loader must be initialized
// first). Invalid programs log the GL info log and IsValid() reports false.

#include "af/math/Mat4.h"
#include "af/math/Vec3.h"
#include "af/render/GL.h"

namespace af {

class ShaderProgram {
public:
    ShaderProgram() = default;
    ShaderProgram(const char* vertexSource, const char* fragmentSource);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    bool IsValid() const { return program_ != 0; }
    void Use() const;
    void SetUniform(const char* name, const Mat4& value) const;
    void SetUniform(const char* name, const Vec3& value) const;
    void SetUniform(const char* name, float value) const;
    void SetUniform(const char* name, int value) const;

private:
    void Destroy();
    gl::GLint Location(const char* name) const;

    gl::GLuint program_ = 0;
};

}  // namespace af
