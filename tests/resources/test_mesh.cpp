#include <cmath>

#include "af/resources/Mesh.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

bool AllIndicesInRange(const Mesh& m) {
    for (const uint32_t i : m.indices) {
        if (i >= m.positions.size()) return false;
    }
    return true;
}

bool AllNormalsUnit(const Mesh& m) {
    for (const Vec3& n : m.normals) {
        if (std::abs(Length(n) - 1.0f) > 1e-5f) return false;
    }
    return true;
}

}  // namespace

AF_TEST("cube mesh has 24 vertices and 36 indices") {
    const Mesh m = MeshGen::Cube(2.0f);
    AF_CHECK_EQ(m.positions.size(), 24u);
    AF_CHECK_EQ(m.normals.size(), 24u);
    AF_CHECK_EQ(m.indices.size(), 36u);
    AF_CHECK(AllIndicesInRange(m));
    AF_CHECK(AllNormalsUnit(m));
    // Normals are axis-aligned per face.
    bool sawPlusX = false;
    for (const Vec3& n : m.normals) {
        const float dominant = Max(Max(std::abs(n.x), std::abs(n.y)), std::abs(n.z));
        AF_CHECK_NEAR(dominant, 1.0f, 1e-5f);
        if (n.x > 0.9f) sawPlusX = true;
    }
    AF_CHECK(sawPlusX);
    // Bounds: cube of size 2 → ±1.
    AF_CHECK_NEAR(m.boundsMin.x, -1.0f, 1e-5f);
    AF_CHECK_NEAR(m.boundsMax.x, 1.0f, 1e-5f);
}

AF_TEST("cube mesh has correct index count per face") {
    const Mesh m = MeshGen::Cube(1.0f);
    // 6 faces × 2 triangles × 3 indices.
    AF_CHECK_EQ(m.indices.size() / 3u, 12u);
}

AF_TEST("uv sphere vertices lie on the sphere") {
    const Mesh m = MeshGen::UVSphere(2.0f, 8, 12);
    AF_CHECK_EQ(m.positions.size(), 9u * 13u);
    AF_CHECK_EQ(m.indices.size(), 8u * 12u * 6u);
    AF_CHECK(AllIndicesInRange(m));
    AF_CHECK(AllNormalsUnit(m));
    for (std::size_t i = 0; i < m.positions.size(); ++i) {
        AF_CHECK_NEAR(Length(m.positions[i]), 2.0f, 1e-3f);
        // Normals match the radial direction for a sphere.
        const Vec3 radial = Normalize(m.positions[i]);
        AF_CHECK_NEAR(Dot(radial, m.normals[i]), 1.0f, 1e-3f);
    }
    AF_CHECK_NEAR(m.boundsMin.x, -2.0f, 1e-3f);
    AF_CHECK_NEAR(m.boundsMax.y, 2.0f, 1e-3f);
}

AF_TEST("plane mesh is a single quad facing +Y") {
    const Mesh m = MeshGen::Plane(10.0f, 10.0f);
    AF_CHECK_EQ(m.positions.size(), 4u);
    AF_CHECK_EQ(m.indices.size(), 6u);
    for (const Vec3& n : m.normals) {
        AF_CHECK_NEAR(n.y, 1.0f, 1e-6f);
    }
    // Corners at ±5 in XZ, y = 0.
    bool sawMinX = false;
    bool sawMaxZ = false;
    for (const Vec3& p : m.positions) {
        AF_CHECK_NEAR(p.y, 0.0f, 1e-6f);
        if (p.x < -4.9f) sawMinX = true;
        if (p.z > 4.9f) sawMaxZ = true;
    }
    AF_CHECK(sawMinX);
    AF_CHECK(sawMaxZ);
}
