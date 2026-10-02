#include "af/render/GLMesh.h"

#include <cstddef>

namespace af {

GLMesh::GLMesh(const Mesh& mesh) {
    gl::GenVertexArrays(1, &vao_);
    gl::BindVertexArray(vao_);

    gl::GenBuffers(3, vbos_);

    gl::BindBuffer(gl::ArrayBuffer, vbos_[0]);
    gl::BufferData(gl::ArrayBuffer,
                   static_cast<gl::GLsizeiptr>(mesh.positions.size() * sizeof(Vec3)),
                   mesh.positions.data(), gl::StaticDraw);
    gl::VertexAttribPointer(0, 3, gl::Float, false, 0, nullptr);
    gl::EnableVertexAttribArray(0);

    gl::BindBuffer(gl::ArrayBuffer, vbos_[1]);
    gl::BufferData(gl::ArrayBuffer,
                   static_cast<gl::GLsizeiptr>(mesh.normals.size() * sizeof(Vec3)),
                   mesh.normals.data(), gl::StaticDraw);
    gl::VertexAttribPointer(1, 3, gl::Float, false, 0, nullptr);
    gl::EnableVertexAttribArray(1);

    gl::BindBuffer(gl::ArrayBuffer, vbos_[2]);
    gl::BufferData(gl::ArrayBuffer,
                   static_cast<gl::GLsizeiptr>(mesh.uvs.size() * sizeof(Vec2)),
                   mesh.uvs.data(), gl::StaticDraw);
    gl::VertexAttribPointer(2, 2, gl::Float, false, 0, nullptr);
    gl::EnableVertexAttribArray(2);

    gl::GenBuffers(1, &ebo_);
    gl::BindBuffer(gl::ElementArrayBuffer, ebo_);
    gl::BufferData(gl::ElementArrayBuffer,
                   static_cast<gl::GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
                   mesh.indices.data(), gl::StaticDraw);

    indexCount_ = static_cast<gl::GLsizei>(mesh.indices.size());
    gl::BindVertexArray(0);
}

GLMesh::~GLMesh() { Destroy(); }

GLMesh::GLMesh(GLMesh&& other) noexcept
    : vao_(other.vao_), ebo_(other.ebo_), indexCount_(other.indexCount_) {
    for (int i = 0; i < 3; ++i) vbos_[i] = other.vbos_[i];
    other.vao_ = 0;
    other.ebo_ = 0;
    for (int i = 0; i < 3; ++i) other.vbos_[i] = 0;
}

GLMesh& GLMesh::operator=(GLMesh&& other) noexcept {
    if (this != &other) {
        Destroy();
        vao_ = other.vao_;
        ebo_ = other.ebo_;
        indexCount_ = other.indexCount_;
        for (int i = 0; i < 3; ++i) vbos_[i] = other.vbos_[i];
        other.vao_ = 0;
        other.ebo_ = 0;
        for (int i = 0; i < 3; ++i) other.vbos_[i] = 0;
    }
    return *this;
}

void GLMesh::Destroy() {
    if (vao_ != 0) {
        gl::DeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (ebo_ != 0) {
        gl::DeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbos_[0] != 0) gl::DeleteBuffers(3, vbos_);
    for (int i = 0; i < 3; ++i) vbos_[i] = 0;
}

void GLMesh::Draw() const {
    gl::BindVertexArray(vao_);
    gl::DrawElements(gl::Triangles, indexCount_, gl::UnsignedInt, nullptr);
    gl::BindVertexArray(0);
}

}  // namespace af
