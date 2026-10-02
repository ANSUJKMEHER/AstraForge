#include "af/physics/PhysicsSystem.h"

#include "af/core/Components.h"
#include "af/math/MathUtil.h"
#include "af/physics/PhysicsComponents.h"

namespace af {

PhysicsSystem::PhysicsSystem(float groundY, float terminalSpeed)
    : groundY_(groundY), terminalSpeed_(terminalSpeed) {}

namespace {

// Vertical extent of the entity for ground contact: sphere radius or AABB
// half-height; 0 when there is no collider.
float GroundExtent(World& world, Entity e) {
    const Collider* c = world.TryGetComponent<Collider>(e);
    if (c == nullptr) return 0.0f;
    return c->kind == ColliderKind::Sphere ? c->radius : c->halfExtents.y;
}

}  // namespace

void PhysicsSystem::Update(World& world, float dt) {
    // 1. Gravity.
    for (auto [entity, velocity, gravity] :
         world.ViewComponents<Velocity, Gravity>()) {
        (void)entity;
        velocity.linear.y += gravity.strength * dt;
        if (velocity.linear.y < -terminalSpeed_) {
            velocity.linear.y = -terminalSpeed_;  // clamp terminal velocity
        }
    }

    // 2. Integrate.
    for (auto [entity, transform, velocity] :
         world.ViewComponents<Transform, Velocity>()) {
        (void)entity;
        transform.position += velocity.linear * dt;
    }

    // 3. Ground response.
    for (auto [entity, transform, velocity] :
         world.ViewComponents<Transform, Velocity>()) {
        const float extent = GroundExtent(world, entity);
        if (velocity.linear.y <= 0.0f &&
            transform.position.y - extent < groundY_) {
            transform.position.y = groundY_ + extent;
            velocity.linear.y = 0.0f;
        }
    }

    // 4. Jump: collect first (consuming requests mutates component storage),
    //    then apply to grounded entities.
    pendingJumps_.clear();
    for (auto [entity, velocity, request] :
         world.ViewComponents<Velocity, JumpRequest>()) {
        (void)velocity;
        pendingJumps_.push_back({entity, request.speed});
    }
    for (const PendingJump& jump : pendingJumps_) {
        world.RemoveComponent<JumpRequest>(jump.entity);
        if (world.IsAlive(jump.entity)) {
            Velocity& v = world.GetComponent<Velocity>(jump.entity);
            const float extent = GroundExtent(world, jump.entity);
            const Transform& t = world.GetComponent<Transform>(jump.entity);
            const bool grounded =
                t.position.y - extent <= groundY_ + 1e-4f && v.linear.y <= 0.0f;
            if (grounded) v.linear.y = jump.speed;
        }
    }
}

}  // namespace af
