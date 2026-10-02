#pragma once
// Renderer — forward renderer over the default Phong-ish shader.
// Per frame: BeginFrame (state + camera + lights) → Submit* (culled draws)
// → EndFrame. Draw calls and culling stats are exposed for the profiler
// (Phase 9).

#include <memory>

#include "af/core/Camera.h"
#include "af/math/Mat4.h"
#include "af/math/Vec3.h"
#include "af/math/Vec4.h"
#include "af/render/GLMesh.h"
#include "af/render/ShaderProgram.h"

namespace af {

enum class MeshId : uint8_t { Cube, Sphere, Plane, Count };

// ECS component (defined here so the render layer owns its own vocabulary):
// an entity with Transform + Renderable gets drawn by RenderSystem.
struct Renderable {
    MeshId mesh = MeshId::Cube;
    Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Must be called after an SDL GL context is current. Loads GL entry
    // points, compiles the default shader, builds the shared primitive meshes.
    bool Init();
    void SetViewportSize(int width, int height);
    bool IsReady() const { return ready_; }

    void BeginFrame(const Camera& camera, Vec3 lightDirection, Vec3 lightColor,
                    Vec3 cameraPosition);
    // Submits a draw if the mesh's world bounds intersect the camera frustum.
    void Submit(MeshId mesh, const Mat4& model, const Vec4& color,
                const Vec3& localBoundsMin, const Vec3& localBoundsMax,
                const Vec3& worldPosition, float worldRadius);
    void EndFrame();

    struct Stats {
        std::size_t submitted = 0;  // passed culling
        std::size_t culled = 0;     // rejected by the frustum
        std::size_t drawCalls = 0;
    };
    const Stats& LastStats() const { return stats_; }

private:
    bool ready_ = false;
    std::unique_ptr<ShaderProgram> shader_;
    std::unique_ptr<GLMesh> meshes_[static_cast<int>(MeshId::Count)];
    Camera camera_;
    Frustum frustum_;
    Vec3 lightDirection_{0.3f, -1.0f, 0.2f};
    Vec3 lightColor_{1.0f, 1.0f, 1.0f};
    Vec3 cameraPosition_{0.0f, 0.0f, 5.0f};
    float ambient_ = 0.40f;
    Stats stats_;
};

}  // namespace af
