#include "af/resources/Mesh.h"

#include <cmath>

#include "af/math/MathUtil.h"

namespace af::MeshGen {

namespace {

void AddVertex(Mesh& m, Vec3 p, Vec3 n, Vec2 uv) {
    m.positions.push_back(p);
    m.normals.push_back(n);
    m.uvs.push_back(uv);
}

void UpdateBounds(Mesh& m, const Vec3& p) {
    m.boundsMin.x = Min(m.boundsMin.x, p.x);
    m.boundsMin.y = Min(m.boundsMin.y, p.y);
    m.boundsMin.z = Min(m.boundsMin.z, p.z);
    m.boundsMax.x = Max(m.boundsMax.x, p.x);
    m.boundsMax.y = Max(m.boundsMax.y, p.y);
    m.boundsMax.z = Max(m.boundsMax.z, p.z);
}

}  // namespace

Mesh Cube(float size) {
    Mesh m;
    m.boundsMin = Vec3{1e9f, 1e9f, 1e9f};
    m.boundsMax = Vec3{-1e9f, -1e9f, -1e9f};
    const float h = size * 0.5f;

    // One quad per face with its own normal (flat shading).
    struct Face {
        Vec3 normal;
        Vec3 u, v;  // tangent axes
    };
    const Face faces[6] = {
        {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},   // +X
        {{-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},  // -X
        {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // +Y
        {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // -Y
        {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},   // +Z
        {{0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // -Z
    };

    for (const Face& f : faces) {
        const uint32_t base = static_cast<uint32_t>(m.positions.size());
        const Vec3 center = f.normal * h;
        const Vec3 corners[4] = {
            center + (-f.u - f.v) * h,
            center + (f.u - f.v) * h,
            center + (f.u + f.v) * h,
            center + (-f.u + f.v) * h,
        };
        const Vec2 uvs[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
        for (int i = 0; i < 4; ++i) {
            AddVertex(m, corners[i], f.normal, uvs[i]);
            UpdateBounds(m, corners[i]);
        }
        const uint32_t idx[6] = {base, base + 1, base + 2, base, base + 2, base + 3};
        for (const uint32_t i : idx) m.indices.push_back(i);
    }
    return m;
}

Mesh UVSphere(float radius, int stacks, int slices) {
    Mesh m;
    m.boundsMin = Vec3{1e9f, 1e9f, 1e9f};
    m.boundsMax = Vec3{-1e9f, -1e9f, -1e9f};
    for (int stack = 0; stack <= stacks; ++stack) {
        const float phi = Pi * static_cast<float>(stack) / static_cast<float>(stacks);
        const float y = std::cos(phi);
        const float r = std::sin(phi);
        for (int slice = 0; slice <= slices; ++slice) {
            const float theta = 2.0f * Pi * static_cast<float>(slice) / static_cast<float>(slices);
            const Vec3 n{std::cos(theta) * r, y, std::sin(theta) * r};
            const Vec3 p = n * radius;
            AddVertex(m, p, n,
                      {static_cast<float>(slice) / static_cast<float>(slices),
                       static_cast<float>(stack) / static_cast<float>(stacks)});
            UpdateBounds(m, p);
        }
    }
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            const uint32_t a = static_cast<uint32_t>(stack * (slices + 1) + slice);
            const uint32_t b = a + static_cast<uint32_t>(slices + 1);
            m.indices.push_back(a);
            m.indices.push_back(b);
            m.indices.push_back(a + 1);
            m.indices.push_back(a + 1);
            m.indices.push_back(b);
            m.indices.push_back(b + 1);
        }
    }
    return m;
}

Mesh Plane(float width, float depth) {
    Mesh m;
    m.boundsMin = Vec3{1e9f, 1e9f, 1e9f};
    m.boundsMax = Vec3{-1e9f, -1e9f, -1e9f};
    const float hw = width * 0.5f;
    const float hd = depth * 0.5f;
    const Vec3 n{0.0f, 1.0f, 0.0f};
    const Vec3 corners[4] = {
        {-hw, 0.0f, -hd}, {hw, 0.0f, -hd}, {hw, 0.0f, hd}, {-hw, 0.0f, hd},
    };
    const Vec2 uvs[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    for (int i = 0; i < 4; ++i) {
        AddVertex(m, corners[i], n, uvs[i]);
        UpdateBounds(m, corners[i]);
    }
    m.indices = {0, 3, 2, 0, 2, 1};
    return m;
}

}  // namespace af::MeshGen
