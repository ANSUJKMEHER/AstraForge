#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/physics/PhysicsComponents.h"
#include "af/physics/PhysicsSystem.h"
#include "framework/AllocationProbe.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

// A falling entity: Transform + Velocity + Gravity. Colliders make ground
// contact use the collider extent.
Entity MakeBody(World& w, Vec3 pos) {
    const Entity e = w.CreateEntity();
    w.AddComponent<Transform>(e).position = pos;
    w.AddComponent<Velocity>(e);
    w.AddComponent<Gravity>(e);
    return e;
}

Entity MakeBodyWithCollider(World& w, Vec3 pos, float radius) {
    const Entity e = MakeBody(w, pos);
    w.AddComponent<Collider>(e, Collider{ColliderKind::Sphere, {}, radius, {}});
    return e;
}

}  // namespace

AF_TEST("free fall matches the semi-implicit reference exactly") {
    World w;
    const Entity e = MakeBody(w, Vec3{0.0f, 10.0f, 0.0f});
    PhysicsSystem physics(0.0f);

    const float dt = 1.0f / 60.0f;
    const float g = -9.81f;
    // Reference computed with the SAME operation order as the system
    // (v += g*dt; p += v*dt per step) — bit-exact expectation.
    float refY = 10.0f;
    float refV = 0.0f;
    for (int step = 0; step < 60; ++step) {
        refV += g * dt;
        refY += refV * dt;
        physics.Update(w, dt);
        AF_CHECK_NEAR(w.GetComponent<Transform>(e).position.y, refY, 0.0f);
        AF_CHECK_NEAR(w.GetComponent<Velocity>(e).linear.y, refV, 0.0f);
    }
    // Sanity: after 1s the fall is within tolerance of 0.5*g*t^2. Semi-
    // implicit Euler differs from the continuous solution by O(dt) —
    // g*t^2/(2N) — so the tolerance reflects that known discretization error.
    AF_CHECK_NEAR(w.GetComponent<Transform>(e).position.y, 10.0f + 0.5f * g, 0.1f);
}

AF_TEST("ground clamps falling entities and holds them stable") {
    World w;
    const Entity e = MakeBodyWithCollider(w, Vec3{0.0f, 5.0f, 0.0f}, 0.5f);
    PhysicsSystem physics(0.0f);
    const float dt = 1.0f / 60.0f;
    // Fall until grounded, then verify stability over many steps.
    for (int step = 0; step < 300; ++step) physics.Update(w, dt);
    const Transform& t = w.GetComponent<Transform>(e);
    AF_CHECK_NEAR(t.position.y, 0.5f, 1e-5f);  // ground + radius
    AF_CHECK_NEAR(w.GetComponent<Velocity>(e).linear.y, 0.0f, 1e-6f);
    // No drift over 600 more steps.
    for (int step = 0; step < 600; ++step) physics.Update(w, dt);
    AF_CHECK_NEAR(t.position.y, 0.5f, 1e-5f);
}

AF_TEST("ground response stops only at the right height") {
    World w;
    // Without a collider, extent is 0: entity rests exactly on the ground.
    const Entity e = MakeBody(w, Vec3{0.0f, 2.0f, 0.0f});
    PhysicsSystem physics(0.0f);
    const float dt = 1.0f / 60.0f;
    for (int step = 0; step < 300; ++step) physics.Update(w, dt);
    AF_CHECK_NEAR(w.GetComponent<Transform>(e).position.y, 0.0f, 1e-5f);
}

AF_TEST("jump applies impulse only when grounded") {
    World w;
    const Entity e = MakeBodyWithCollider(w, Vec3{0.0f, 5.0f, 0.0f}, 0.5f);
    PhysicsSystem physics(0.0f);
    const float dt = 1.0f / 60.0f;

    // Request a jump while airborne: must be ignored (and consumed).
    w.AddComponent<JumpRequest>(e, JumpRequest{8.0f});
    physics.Update(w, dt);
    AF_CHECK(!w.HasComponent<JumpRequest>(e));  // consumed either way
    AF_CHECK_NEAR(w.GetComponent<Velocity>(e).linear.y, -9.81f * dt, 0.0f);

    // Land, then jump from the ground: velocity becomes the impulse speed.
    for (int step = 0; step < 300; ++step) physics.Update(w, dt);
    AF_CHECK_NEAR(w.GetComponent<Velocity>(e).linear.y, 0.0f, 1e-6f);
    w.AddComponent<JumpRequest>(e, JumpRequest{8.0f});
    physics.Update(w, dt);
    AF_CHECK_NEAR(w.GetComponent<Velocity>(e).linear.y, 8.0f, 1e-6f);
}

AF_TEST("jump arc rises and falls back deterministically") {
    World w;
    const Entity e = MakeBodyWithCollider(w, Vec3{0.0f, 0.5f, 0.0f}, 0.5f);
    PhysicsSystem physics(0.0f);
    const float dt = 1.0f / 60.0f;
    w.AddComponent<JumpRequest>(e, JumpRequest{6.0f});

    // Bit-exact reference mirroring the system's exact per-tick order:
    // gravity → integrate → ground → jump (the jump lands AFTER integrate,
    // so its impulse is first integrated in the NEXT tick).
    float refY = 0.5f;
    float refV = 0.0f;
    float maxY = 0.0f;
    bool jumped = false;
    for (int step = 0; step < 120; ++step) {
        refV += -9.81f * dt;      // gravity
        refY += refV * dt;        // integrate
        if (refV <= 0.0f && refY < 0.5f) { refY = 0.5f; refV = 0.0f; }  // ground
        if (step == 0) refV = 6.0f;  // jump impulse, after ground handling
        physics.Update(w, dt);
        AF_CHECK_NEAR(w.GetComponent<Transform>(e).position.y, refY, 0.0f);
        if (refY > maxY) { maxY = refY; jumped = true; }
    }
    AF_CHECK(jumped);
    // Analytic apex v^2/(2g) — the discrete sim undershoots it by O(dt).
    AF_CHECK_NEAR(maxY - 0.5f, (6.0f * 6.0f) / (2.0f * 9.81f), 6e-2f);
    // Landed and resting at the end.
    AF_CHECK_NEAR(w.GetComponent<Transform>(e).position.y, 0.5f, 1e-5f);
    AF_CHECK_NEAR(w.GetComponent<Velocity>(e).linear.y, 0.0f, 1e-6f);
}

AF_TEST("terminal velocity is clamped") {
    World w;
    const Entity e = MakeBody(w, Vec3{0.0f, 100.0f, 0.0f});
    PhysicsSystem physics(0.0f, 10.0f);
    const float dt = 1.0f / 60.0f;
    for (int step = 0; step < 300; ++step) physics.Update(w, dt);
    AF_CHECK(w.GetComponent<Velocity>(e).linear.y >= -10.0f - 1e-4f);
}

AF_TEST("gravity strength is per-entity") {
    World w;
    const Entity light = MakeBody(w, Vec3{0.0f, 5.0f, 0.0f});
    w.GetComponent<Gravity>(light).strength = -2.0f;
    const Entity heavy = MakeBody(w, Vec3{0.0f, 5.0f, 0.0f});
    PhysicsSystem physics(0.0f);
    const float dt = 1.0f / 60.0f;
    physics.Update(w, dt);
    AF_CHECK_NEAR(w.GetComponent<Velocity>(light).linear.y, -2.0f * dt, 0.0f);
    AF_CHECK_NEAR(w.GetComponent<Velocity>(heavy).linear.y, -9.81f * dt, 0.0f);
}

AF_TEST("physics update allocates zero bytes in steady state") {
    World w;
    for (int i = 0; i < 500; ++i) {
        const Entity e = w.CreateEntity();
        w.AddComponent<Transform>(e).position = Vec3{0.0f, 10.0f, 0.0f};
        w.AddComponent<Velocity>(e);
        w.AddComponent<Gravity>(e);
        w.AddComponent<Collider>(e, Collider{ColliderKind::Sphere, {}, 0.5f, {}});
    }
    PhysicsSystem physics(0.0f);
    const float dt = 1.0f / 60.0f;
    for (int i = 0; i < 10; ++i) physics.Update(w, dt);  // warm up

    const long long before = af::test::AllocationCount();
    for (int i = 0; i < 600; ++i) physics.Update(w, dt);
    const long long after = af::test::AllocationCount();
    AF_CHECK_EQ(after, before);
}
