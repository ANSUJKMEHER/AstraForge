#pragma once
// GLMesh — GPU-side vertex/index buffers for an af::Mesh (VAO + 3 VBOs for
// position/normal/uv + EBO). Draw() issues one indexed draw call.

#include "af/render/GL.h"
#include "af/resources/Mesh.h"

namespace af {

class GLMesh {
public:
    GLMesh() = default;
    explicit GLMesh(const Mesh& mesh);
    ~GLMesh();

    GLMesh(const GLMesh&) = delete;
    GLMesh& operator=(const GLMesh&) = delete;
    GLMesh(GLMesh&& other) noexcept;
    GLMesh& operator=(GLMesh&& other) noexcept;

    void Draw() const;
    bool IsValid() const { return vao_ != 0; }

private:
    void Destroy();

    gl::GLuint vao_ = 0;
    gl::GLuint vbos_[3] = {0, 0, 0};
    gl::GLuint ebo_ = 0;
    gl::GLsizei indexCount_ = 0;
};

}  // namespace af
