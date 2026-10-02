#pragma once
// Minimal self-contained OpenGL 3.3 loader.
//
// Decision (context.md): instead of GLAD/GLEW, this header defines the GL
// types, the constants we use, and the ~25 core-profile entry points we call,
// as function pointers in namespace af::gl. gl_load.cpp resolves them once
// via SDL_GL_GetProcAddress (which also falls back to opengl32.dll exports on
// Windows, covering the legacy 1.1 functions). A missing entry point is a
// hard, clearly-reported failure.
//
// Constraint: only symbols we actually use are declared — if a renderer
// change needs a new GL call, add it here AND to gl_load.cpp.

#include <cstddef>
#include <cstdint>

namespace af::gl {

// --- Types ---------------------------------------------------------------
using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLsizeiptr = std::ptrdiff_t;
using GLfloat = float;
using GLchar = char;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;

// --- Constants (values from the OpenGL 3.3 specification) -----------------
inline constexpr GLenum ColorBufferBit = 0x00004000u;
inline constexpr GLenum DepthBufferBit = 0x00000100u;
inline constexpr GLenum DepthTest = 0x0B71u;
inline constexpr GLenum CullFaceCap = 0x0B44u;  // cap (the function is CullFace)
inline constexpr GLenum Back = 0x0405u;
inline constexpr GLenum Ccw = 0x0901u;
inline constexpr GLenum Less = 0x0201u;
inline constexpr GLenum Triangles = 0x0004u;
inline constexpr GLenum UnsignedInt = 0x1405u;
inline constexpr GLenum Float = 0x1406u;
inline constexpr GLenum ArrayBuffer = 0x8892u;
inline constexpr GLenum ElementArrayBuffer = 0x8893u;
inline constexpr GLenum StaticDraw = 0x88E4u;
inline constexpr GLenum VertexShader = 0x8B31u;
inline constexpr GLenum FragmentShader = 0x8B30u;
inline constexpr GLenum CompileStatus = 0x8B81u;
inline constexpr GLenum LinkStatus = 0x8B82u;
inline constexpr GLenum FrontAndBack = 0x0408u;
inline constexpr GLenum Line = 0x1B01u;
inline constexpr GLenum Fill = 0x1B02u;
inline constexpr GLenum Blend = 0x0BE2u;
inline constexpr GLenum SrcAlpha = 0x0302u;
inline constexpr GLenum OneMinusSrcAlpha = 0x0303u;
inline constexpr GLenum Rgb = 0x1907u;
inline constexpr GLenum UnsignedByte = 0x1401u;
inline constexpr GLenum NoError = 0u;

// --- Entry points ---------------------------------------------------------
#define AF_GL_ENTRY_POINTS(X) \
    X(void, Clear, (GLbitfield mask)) \
    X(void, ClearColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a)) \
    X(void, Viewport, (GLint x, GLint y, GLsizei w, GLsizei h)) \
    X(void, Enable, (GLenum cap)) \
    X(void, Disable, (GLenum cap)) \
    X(void, BlendFunc, (GLenum sfactor, GLenum dfactor)) \
    X(void, ReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* data)) \
    X(void, CullFace, (GLenum mode)) \
    X(void, DepthFunc, (GLenum func)) \
    X(void, PolygonMode, (GLenum face, GLenum mode)) \
    X(GLenum, GetError, ()) \
    X(void, GenVertexArrays, (GLsizei n, GLuint* arrays)) \
    X(void, BindVertexArray, (GLuint array)) \
    X(void, DeleteVertexArrays, (GLsizei n, const GLuint* arrays)) \
    X(void, GenBuffers, (GLsizei n, GLuint* buffers)) \
    X(void, BindBuffer, (GLenum target, GLuint buffer)) \
    X(void, BufferData, (GLenum target, GLsizeiptr size, const void* data, GLenum usage)) \
    X(void, DeleteBuffers, (GLsizei n, const GLuint* buffers)) \
    X(void, VertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer)) \
    X(void, EnableVertexAttribArray, (GLuint index)) \
    X(GLuint, CreateShader, (GLenum type)) \
    X(void, ShaderSource, (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)) \
    X(void, CompileShader, (GLuint shader)) \
    X(void, GetShaderiv, (GLuint shader, GLenum pname, GLint* params)) \
    X(void, GetShaderInfoLog, (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)) \
    X(void, DeleteShader, (GLuint shader)) \
    X(GLuint, CreateProgram, ()) \
    X(void, AttachShader, (GLuint program, GLuint shader)) \
    X(void, LinkProgram, (GLuint program)) \
    X(void, GetProgramiv, (GLuint program, GLenum pname, GLint* params)) \
    X(void, GetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)) \
    X(void, DeleteProgram, (GLuint program)) \
    X(void, UseProgram, (GLuint program)) \
    X(GLint, GetUniformLocation, (GLuint program, const GLchar* name)) \
    X(void, UniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) \
    X(void, Uniform3fv, (GLint location, GLsizei count, const GLfloat* value)) \
    X(void, Uniform1f, (GLint location, GLfloat v0)) \
    X(void, Uniform1i, (GLint location, GLint v0)) \
    X(void, DrawElements, (GLenum mode, GLsizei count, GLenum type, const void* indices)) \
    X(void, DrawArrays, (GLenum mode, GLint first, GLsizei count))

#define AF_GL_DECLARE(ret, name, args) extern ret (*name) args;
AF_GL_ENTRY_POINTS(AF_GL_DECLARE)
#undef AF_GL_DECLARE

// Resolves every entry point above. Returns false and prints the missing
// symbol names if any resolution failed.
bool LoadGL();

}  // namespace af::gl
