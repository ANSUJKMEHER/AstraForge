#pragma once
// Core engine components — plain value types stored densely by the ECS.
// Requirement: move-assignable (swap-remove); these are trivially so.

#include <cstdint>

#include "af/math/Transform.h"
#include "af/math/Vec3.h"

namespace af {

// Note: Transform is the math type af::Transform (position, quaternion
// rotation, scale) reused directly as the ECS component — no duplication.

// Linear velocity in world units per second.
struct Velocity {
    Vec3 linear{0.0f, 0.0f, 0.0f};
};

enum class ColliderKind : uint8_t { Sphere, AABB };

// Axis-aligned collider attached to an entity. The world-space shape is
// computed from the entity position + offset (rotation/scale are ignored —
// documented limitation, adequate for the arena gameplay).
//
// isStatic marks non-moving geometry (arena walls). Static colliders are
// excluded from the broadphase grid and only push dynamic colliders (never
// move themselves) — see CollisionSystem.
struct Collider {
    ColliderKind kind = ColliderKind::Sphere;
    Vec3 offset{0.0f, 0.0f, 0.0f};
    float radius = 1.0f;                       // Sphere
    Vec3 halfExtents{0.5f, 0.5f, 0.5f};        // AABB
    bool isStatic = false;
};

}  // namespace af
