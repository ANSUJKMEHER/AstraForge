#pragma once
// PhysicsSystem — semi-implicit Euler kinematics at fixed dt, ground response,
// jump handling. Runs inside the fixed-timestep loop (see context.md
// Decision 4), so every integration step is deterministic and testable.
//
// Update order (deliberate, documented):
//   1. gravity: v.y += g*dt (per-entity Gravity strength, clamped terminal)
//   2. integrate: p += v*dt
//   3. ground response: falling entities with ground contact clamp + zero vy
//   4. jump: grounded entities with a JumpRequest get vy = speed; request
//      consumed (component removed)
//
// Ground contact uses the collider's vertical extent when present (sphere
// radius / AABB half-height), else 0.

#include <vector>

#include "af/ecs/System.h"
#include "af/ecs/World.h"

namespace af {

class PhysicsSystem : public System {
public:
    explicit PhysicsSystem(float groundY = 0.0f, float terminalSpeed = 40.0f);

    void Update(World& world, float dt) override;

private:
    float groundY_;
    float terminalSpeed_;
    // Scratch buffer reused across frames (zero steady-state allocations):
    // jump requests are collected, then applied, because consuming a request
    // removes the component mid-frame (structural change outside views).
    struct PendingJump {
        Entity entity;
        float speed = 0.0f;
    };
    std::vector<PendingJump> pendingJumps_;
};

}  // namespace af
