#pragma once
// Procedurally generated meshes — no asset pipeline needed for the primitives
// the arena uses (deliberate: see context.md, laptop constraint).

#include <cstdint>
#include <vector>

#include "af/math/Vec2.h"
#include "af/math/Vec3.h"

namespace af {

struct Mesh {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<uint32_t> indices;
    Vec3 boundsMin{0.0f, 0.0f, 0.0f};  // local-space AABB (for culling)
    Vec3 boundsMax{0.0f, 0.0f, 0.0f};
};

namespace MeshGen {

// 24 vertices (one normal per face), 36 indices. size = full edge length.
Mesh Cube(float size = 1.0f);

// UV sphere: (stacks + 1) * (slices + 1) vertices, indexed.
Mesh UVSphere(float radius, int stacks, int slices);

// XZ plane centered at the origin, +Y normal, 4 vertices / 2 triangles.
Mesh Plane(float width, float depth);

}  // namespace MeshGen

}  // namespace af
