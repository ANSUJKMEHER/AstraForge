#include "af/render/Renderer.h"

#include <cstdio>

#include "af/math/MathUtil.h"
#include "af/resources/Mesh.h"
#include "af/resources/Shaders.h"

namespace af {

namespace {

// Normal matrix for the shader: inverse-transpose of the model's upper 3x3
// (correct under non-uniform scale). Computed with our own Mat4 inverse —
// the math library dogfoods itself.
Mat4 NormalMatrix(const Mat4& model) {
    Mat4 m = Transpose(Inverse(model));
    return m;
}

}  // namespace

Renderer::~Renderer() = default;

bool Renderer::Init() {
    if (!gl::LoadGL()) {
        std::fprintf(stderr, "AstraForge: GL entry point loading failed\n");
        return false;
    }

    shader_ = std::make_unique<ShaderProgram>(DefaultVertexShader(),
                                              DefaultFragmentShader());
    if (!shader_->IsValid()) {
        std::fprintf(stderr, "AstraForge: default shader failed to compile\n");
        return false;
    }

    meshes_[static_cast<int>(MeshId::Cube)] =
        std::make_unique<GLMesh>(MeshGen::Cube(1.0f));
    meshes_[static_cast<int>(MeshId::Sphere)] =
        std::make_unique<GLMesh>(MeshGen::UVSphere(0.5f, 16, 24));
    meshes_[static_cast<int>(MeshId::Plane)] =
        std::make_unique<GLMesh>(MeshGen::Plane(1.0f, 1.0f));

    gl::Enable(gl::DepthTest);
    gl::DepthFunc(gl::Less);
    gl::Enable(gl::CullFaceCap);
    gl::CullFace(gl::Back);
    gl::Enable(gl::Blend);
    gl::BlendFunc(gl::SrcAlpha, gl::OneMinusSrcAlpha);

    ready_ = true;
    return true;
}

void Renderer::SetViewportSize(int width, int height) {
    gl::Viewport(0, 0, width, height);
}

void Renderer::BeginFrame(const Camera& camera, Vec3 lightDirection,
                          Vec3 lightColor, Vec3 cameraPosition) {
    camera_ = camera;
    frustum_ = camera.GetFrustum();
    lightDirection_ = Normalize(lightDirection);
    lightColor_ = lightColor;
    cameraPosition_ = cameraPosition;
    stats_ = {};

    gl::ClearColor(0.08f, 0.11f, 0.18f, 1.0f);
    gl::Clear(gl::ColorBufferBit | gl::DepthBufferBit);
}

void Renderer::Submit(MeshId mesh, const Mat4& model, const Vec4& color,
                      const Vec3& localBoundsMin, const Vec3& localBoundsMax,
                      const Vec3& worldPosition, float worldRadius) {
    (void)localBoundsMin;
    (void)localBoundsMax;
    // Frustum culling: conservative sphere test around the entity.
    if (!frustum_.IntersectsSphere(worldPosition, worldRadius)) {
        ++stats_.culled;
        return;
    }
    ++stats_.submitted;

    shader_->Use();
    shader_->SetUniform("uModel", model);
    shader_->SetUniform("uViewProjection", camera_.ViewProjection());
    shader_->SetUniform("uNormalMatrix", NormalMatrix(model));
    shader_->SetUniform("uColor", Vec3{color.x, color.y, color.z});
    shader_->SetUniform("uAlpha", color.w);
    shader_->SetUniform("uLightDirection", lightDirection_);
    shader_->SetUniform("uLightColor", lightColor_);
    shader_->SetUniform("uCameraPosition", cameraPosition_);
    shader_->SetUniform("uAmbientStrength", ambient_);

    meshes_[static_cast<int>(mesh)]->Draw();
    ++stats_.drawCalls;
}

void Renderer::EndFrame() {}

}  // namespace af
