// Player controller tests: movement, jump edge-triggering, shooting with
// cooldown + pool, arena clamping, dead-player behavior.

#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/game/GameplayComponents.h"
#include "af/game/PlayerControllerSystem.h"
#include "af/game/ProjectilePool.h"
#include "af/physics/PhysicsComponents.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

struct Fixture {
    World world;
    ProjectilePool pool{world, 16};
    Entity player;

    explicit Fixture(Vec3 pos = {0.0f, 1.0f, 0.0f}) {
        player = world.CreateEntity();
        world.AddComponent<Transform>(player).position = pos;
        world.AddComponent<Velocity>(player);
        world.AddComponent<Gravity>(player);
        world.AddComponent<Collider>(player,
                                     Collider{ColliderKind::Sphere, {},
                                              0.5f, {}});
        world.AddComponent<Player>(player);
        world.AddComponent<Health>(player, Health{5, 5});
    }
};

}  // namespace

AF_TEST("WASD input sets horizontal velocity at player speed") {
    Fixture f;
    PlayerInput input;
    input.move = Vec2{0.0f, 1.0f};  // forward
    PlayerControllerSystem sys(f.player, f.pool, &input, {-20, -20}, {20, 20});

    sys.Update(f.world, 1.0f / 60.0f);
    const Velocity& v = f.world.GetComponent<Velocity>(f.player);
    AF_CHECK_NEAR(v.linear.z, 6.0f, 1e-5f);  // forward speed
    AF_CHECK_NEAR(v.linear.x, 0.0f, 1e-5f);
    AF_CHECK_NEAR(v.linear.y, 0.0f, 1e-6f);  // controller never touches Y
}

AF_TEST("diagonal input is normalized (no faster diagonal movement)") {
    Fixture f;
    PlayerInput input;
    input.move = Vec2{1.0f, 1.0f};
    PlayerControllerSystem sys(f.player, f.pool, &input, {-20, -20}, {20, 20});

    sys.Update(f.world, 1.0f / 60.0f);
    const Velocity& v = f.world.GetComponent<Velocity>(f.player);
    const float speed = Length(Vec2{v.linear.x, v.linear.z});
    AF_CHECK_NEAR(speed, 6.0f, 1e-4f);  // capped at moveSpeed, not 8.49
}

AF_TEST("jump fires only on the rising edge") {
    Fixture f;
    PlayerInput input;
    input.jump = true;
    PlayerControllerSystem sys(f.player, f.pool, &input, {-20, -20}, {20, 20});

    sys.Update(f.world, 1.0f / 60.0f);
    AF_CHECK(f.world.HasComponent<JumpRequest>(f.player));

    // Still held: no new request (physics would consume the old one).
    f.world.RemoveComponent<JumpRequest>(f.player);
    sys.Update(f.world, 1.0f / 60.0f);
    AF_CHECK(!f.world.HasComponent<JumpRequest>(f.player));

    // Released and pressed again: a fresh request.
    input.jump = false;
    sys.Update(f.world, 1.0f / 60.0f);
    input.jump = true;
    sys.Update(f.world, 1.0f / 60.0f);
    AF_CHECK(f.world.HasComponent<JumpRequest>(f.player));
    const JumpRequest& req = f.world.GetComponent<JumpRequest>(f.player);
    AF_CHECK_NEAR(req.speed, 8.0f, 1e-5f);
}

AF_TEST("shooting spawns pooled projectiles, gated by cooldown") {
    Fixture f;
    PlayerInput input;
    input.shoot = true;
    input.aim = Vec3{0.0f, 0.0f, -1.0f};
    PlayerControllerSystem sys(f.player, f.pool, &input, {-20, -20}, {20, 20});
    const float dt = 0.125f;  // binary-exact: 2 ticks == 0.25 s cooldown

    sys.Update(f.world, dt);
    AF_CHECK_EQ(f.pool.LiveCount(), 1u);

    // Cooldown 0.25 s: still only one projectile after one more tick.
    sys.Update(f.world, dt);
    AF_CHECK_EQ(f.pool.LiveCount(), 1u);
    sys.Update(f.world, dt);  // 2nd tick: cooldown elapsed → second shot
    AF_CHECK_EQ(f.pool.LiveCount(), 2u);
}

AF_TEST("projectile spawns in the aim direction ahead of the player") {
    Fixture f;
    PlayerInput input;
    input.shoot = true;
    input.aim = Vec3{0.0f, 0.0f, -1.0f};
    PlayerControllerSystem sys(f.player, f.pool, &input, {-20, -20}, {20, 20});

    sys.Update(f.world, 1.0f / 60.0f);
    AF_CHECK_EQ(f.pool.LiveCount(), 1u);

    // Find the spawned projectile via the view.
    bool found = false;
    for (auto [e, t, p] : f.world.ViewComponents<Transform, Projectile>()) {
        (void)e;
        found = true;
        AF_CHECK_NEAR(p.dir.z, -1.0f, 1e-6f);
        // Player at (0,1,0) + aim*0.8 → spawns at z = -0.8.
        AF_CHECK_NEAR(t.position.z, -0.8f, 1e-5f);
    }
    AF_CHECK(found);
}

AF_TEST("dead player stops moving and cannot shoot") {
    Fixture f;
    f.world.GetComponent<Health>(f.player).current = 0;
    PlayerInput input;
    input.move = Vec2{1.0f, 0.0f};
    input.shoot = true;
    PlayerControllerSystem sys(f.player, f.pool, &input, {-20, -20}, {20, 20});

    sys.Update(f.world, 1.0f / 60.0f);
    const Velocity& v = f.world.GetComponent<Velocity>(f.player);
    AF_CHECK_NEAR(v.linear.x, 0.0f, 1e-6f);
    AF_CHECK_NEAR(v.linear.z, 0.0f, 1e-6f);
    AF_CHECK_EQ(f.pool.LiveCount(), 0u);
}

AF_TEST("player is clamped to the arena rectangle") {
    Fixture f({25.0f, 1.0f, -30.0f});  // beyond the walls (clamp is a backstop)
    PlayerInput input;
    PlayerControllerSystem sys(f.player, f.pool, &input, {-20, -20}, {20, 20});

    sys.Update(f.world, 1.0f / 60.0f);
    const Transform& t = f.world.GetComponent<Transform>(f.player);
    AF_CHECK_NEAR(t.position.x, 20.0f, 1e-4f);  // clamped to +X wall
    AF_CHECK_NEAR(t.position.z, -20.0f, 1e-4f);
}
