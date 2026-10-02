#pragma once
// Collision primitives. Axis-aligned, world-space units. Colliders ignore
// entity rotation/scale (documented limitation — the gameplay objects use
// them as axis-aligned bounds).

#include "af/math/Vec3.h"

namespace af {

struct Sphere {
    Vec3 center{0.0f, 0.0f, 0.0f};
    float radius = 1.0f;
};

struct AABB {
    Vec3 min{0.0f, 0.0f, 0.0f};
    Vec3 max{0.0f, 0.0f, 0.0f};
};

inline AABB MakeAABB(const Vec3& center, const Vec3& halfExtents) {
    return {center - halfExtents, center + halfExtents};
}

}  // namespace af
