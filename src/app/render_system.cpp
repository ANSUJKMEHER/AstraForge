#include "af/render/RenderSystem.h"

#include <array>
#include <algorithm>

#include "af/core/Components.h"
#include "af/math/MathUtil.h"
#include "af/resources/Mesh.h"

namespace af {

namespace {

struct Bounds {
    Vec3 min;
    Vec3 max;
};

Bounds MeshBounds(MeshId mesh) {
    static const std::array<Bounds, static_cast<int>(MeshId::Count)> cache = [] {
        std::array<Bounds, static_cast<int>(MeshId::Count)> out;
        const Mesh cube = MeshGen::Cube(1.0f);
        out[static_cast<int>(MeshId::Cube)] = {cube.boundsMin, cube.boundsMax};
        const Mesh sphere = MeshGen::UVSphere(0.5f, 16, 24);
        out[static_cast<int>(MeshId::Sphere)] = {sphere.boundsMin, sphere.boundsMax};
        const Mesh plane = MeshGen::Plane(1.0f, 1.0f);
        out[static_cast<int>(MeshId::Plane)] = {plane.boundsMin, plane.boundsMax};
        return out;
    }();
    return cache[static_cast<int>(mesh)];
}

}  // namespace

void RenderScene(World& world, Renderer& renderer, const Camera& camera) {
    (void)camera;

    // Pass 1: Render soft contact drop shadows directly on the ground
    for (auto [entity, transform, collider] : world.ViewComponents<Transform, Collider>()) {
        (void)entity;
        if (!collider.isStatic && transform.position.y >= -0.2f && transform.position.y <= 12.0f) {
            const float r = (collider.kind == ColliderKind::Sphere) ?
                            collider.radius : Max(collider.halfExtents.x, collider.halfExtents.z);
            const float h = Max(0.0f, transform.position.y);
            const float shadowRadius = r * Max(0.4f, 1.25f - h * 0.08f);
            const float shadowAlpha = std::clamp(0.55f - h * 0.06f, 0.12f, 0.55f);

            const Mat4 shadowModel = Mat4::Translation(Vec3{transform.position.x, 0.02f, transform.position.z}) *
                                     Mat4::Scaling(Vec3{shadowRadius * 2.0f, 0.001f, shadowRadius * 2.0f});
            renderer.Submit(MeshId::Sphere, shadowModel, Vec4{0.02f, 0.03f, 0.05f, shadowAlpha},
                            Vec3{-shadowRadius, -0.01f, -shadowRadius},
                            Vec3{shadowRadius, 0.01f, shadowRadius},
                            Vec3{transform.position.x, 0.02f, transform.position.z}, shadowRadius);
        }
    }

    // Pass 2: Render 3D entity geometry
    for (auto [entity, transform, renderable] :
         world.ViewComponents<Transform, Renderable>()) {
        (void)entity;
        const Bounds b = MeshBounds(renderable.mesh);
        const float radius = Length(b.max - b.min) * 0.5f;
        const float maxScale = Max(Max(transform.scale.x, transform.scale.y),
                                   transform.scale.z);
        renderer.Submit(renderable.mesh, transform.ToMat4(), renderable.color,
                        b.min, b.max, transform.position, radius * maxScale);
    }
}

}  // namespace af
