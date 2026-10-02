#include "af/render/ShaderProgram.h"

#include <cstdio>
#include <vector>

namespace af {

namespace {

gl::GLuint CompileShader(gl::GLenum type, const char* source) {
    const gl::GLuint shader = gl::CreateShader(type);
    const gl::GLchar* src = source;
    gl::ShaderSource(shader, 1, &src, nullptr);
    gl::CompileShader(shader);
    gl::GLint status = 0;
    gl::GetShaderiv(shader, gl::CompileStatus, &status);
    if (status == 0) {
        gl::GLchar log[1024];
        gl::GLsizei length = 0;
        gl::GetShaderInfoLog(shader, sizeof(log), &length, log);
        std::fprintf(stderr, "AstraForge: shader compile failed:\n%s\n", log);
        gl::DeleteShader(shader);
        return 0;
    }
    return shader;
}

}  // namespace

ShaderProgram::ShaderProgram(const char* vertexSource, const char* fragmentSource) {
    const gl::GLuint vs = CompileShader(gl::VertexShader, vertexSource);
    const gl::GLuint fs = CompileShader(gl::FragmentShader, fragmentSource);
    if (vs == 0 || fs == 0) {
        if (vs) gl::DeleteShader(vs);
        if (fs) gl::DeleteShader(fs);
        return;  // invalid
    }
    program_ = gl::CreateProgram();
    gl::AttachShader(program_, vs);
    gl::AttachShader(program_, fs);
    gl::LinkProgram(program_);
    gl::GLint status = 0;
    gl::GetProgramiv(program_, gl::LinkStatus, &status);
    if (status == 0) {
        gl::GLchar log[1024];
        gl::GLsizei length = 0;
        gl::GetProgramInfoLog(program_, sizeof(log), &length, log);
        std::fprintf(stderr, "AstraForge: program link failed:\n%s\n", log);
        Destroy();
    }
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);
}

ShaderProgram::~ShaderProgram() { Destroy(); }

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : program_(other.program_) {
    other.program_ = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        Destroy();
        program_ = other.program_;
        other.program_ = 0;
    }
    return *this;
}

void ShaderProgram::Destroy() {
    if (program_ != 0) {
        gl::DeleteProgram(program_);
        program_ = 0;
    }
}

void ShaderProgram::Use() const { gl::UseProgram(program_); }

gl::GLint ShaderProgram::Location(const char* name) const {
    return gl::GetUniformLocation(program_, name);
}

void ShaderProgram::SetUniform(const char* name, const Mat4& value) const {
    gl::UniformMatrix4fv(Location(name), 1, false, value.Data());
}

void ShaderProgram::SetUniform(const char* name, const Vec3& value) const {
    gl::Uniform3fv(Location(name), 1, &value.x);
}

void ShaderProgram::SetUniform(const char* name, float value) const {
    gl::Uniform1f(Location(name), value);
}

void ShaderProgram::SetUniform(const char* name, int value) const {
    gl::Uniform1i(Location(name), value);
}

}  // namespace af
