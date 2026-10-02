// GameSession integration tests: full loop behavior — spawning, score on
// kills, player death → restart, determinism (twin sessions), steady-state
// allocation freedom.

#include "af/core/Components.h"
#include "af/game/GameSession.h"
#include "af/game/GameplayComponents.h"
#include "af/physics/PhysicsComponents.h"
#include "framework/AllocationProbe.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

// Runs N fixed steps with an idle player (no input).
void RunIdle(GameSession& session, int steps) {
    for (int i = 0; i < steps; ++i) {
        session.SetPlayerInput(PlayerInput{});
        session.Update(1.0f / 60.0f);
    }
}

}  // namespace

AF_TEST("session starts with a live player, empty score, no enemies") {
    GameSession session;
    AF_CHECK(session.PlayerAlive());
    AF_CHECK_EQ(session.Score(), 0);
    AF_CHECK_EQ(session.Kills(), 0);
    AF_CHECK_EQ(session.EnemyCount(), 0u);
    AF_CHECK_EQ(session.ProjectileCount(), 0u);
}

AF_TEST("spawner produces enemies over time, capped at max_alive") {
    GameSession::Config config;
    config.spawnInterval = 0.1f;
    config.maxEnemies = 4;
    config.restartDelay = 100.0f;  // keep the player alive scenario simple
    GameSession session(config);

    RunIdle(session, 60);   // 1 s: ~10 spawn opportunities, capped at 4
    AF_CHECK_EQ(session.EnemyCount(), 4u);

    RunIdle(session, 600);  // cap must hold over time
    AF_CHECK(session.EnemyCount() <= 4u);
}

AF_TEST("player shoots a stationary enemy to death and scores") {
    GameSession session;

    // Park an enemy directly in front of the player.
    World& world = session.GetWorld();
    const Entity enemy = world.CreateEntity();
    world.AddComponent<Transform>(enemy).position = Vec3{0.0f, 1.0f, -3.0f};
    world.AddComponent<Velocity>(enemy);
    world.AddComponent<Gravity>(enemy);
    world.AddComponent<Collider>(enemy, Collider{ColliderKind::Sphere, {},
                                                 0.5f, {}});
    world.AddComponent<Enemy>(enemy);
    world.AddComponent<Health>(enemy, Health{3, 3});

    const float dt = 1.0f / 60.0f;
    PlayerInput input;
    input.shoot = true;
    input.aim = Vec3{0.0f, 0.0f, -1.0f};
    for (int i = 0; i < 300; ++i) {
        session.SetPlayerInput(input);
        session.Update(dt);
        if (session.Kills() > 0) break;
    }

    AF_CHECK_EQ(session.Kills(), 1);
    AF_CHECK_EQ(session.Score(), 100);  // Enemy::scoreValue
    AF_CHECK(!world.IsAlive(enemy));    // dead enemy destroyed
}

AF_TEST("enemies damage the player; death triggers restart") {
    GameSession::Config config;
    config.spawnInterval = 100.0f;  // spawner quiet
    config.restartDelay = 0.5f;
    config.playerHealth = 2;
    GameSession session(config);

    World& world = session.GetWorld();
    // Off the player's aim axis (-Z): shooting must not interrupt the enemy's
    // attack (constant hits would keep it permanently Hurt).
    const Entity enemy = world.CreateEntity();
    world.AddComponent<Transform>(enemy).position = Vec3{2.0f, 1.0f, 0.0f};
    world.AddComponent<Velocity>(enemy);
    world.AddComponent<Gravity>(enemy);
    world.AddComponent<Collider>(enemy, Collider{ColliderKind::Sphere, {},
                                                 0.5f, {}});
    Enemy enemyData;
    enemyData.state = EnemyState::Attack;
    enemyData.attackRange = 2.5f;
    enemyData.attackCooldown = 0.5f;
    enemyData.attackTimer = 0.0f;
    enemyData.damage = 1;
    world.AddComponent<Enemy>(enemy, enemyData);
    world.AddComponent<Health>(enemy, Health{10, 10});

    const float dt = 1.0f / 60.0f;
    PlayerInput input;
    input.shoot = true;
    input.aim = Vec3{0.0f, 0.0f, -1.0f};
    bool died = false;
    for (int i = 0; i < 600 && !died; ++i) {
        session.SetPlayerInput(input);
        session.Update(dt);
        died = !session.PlayerAlive();
    }
    AF_CHECK(died);  // melee hits landed and killed the player

    // Keep updating: restart countdown (0.5 s) elapses → world resets.
    for (int i = 0; i < 120; ++i) {
        session.SetPlayerInput(PlayerInput{});
        session.Update(dt);
    }
    AF_CHECK(session.PlayerAlive());
    AF_CHECK_EQ(session.Score(), 0);         // score reset
    AF_CHECK_EQ(session.Kills(), 0);
    AF_CHECK_EQ(session.ProjectileCount(), 0u);
    const Vec3 p = world.GetComponent<Transform>(session.Player()).position;
    AF_CHECK_NEAR(p.x, 0.0f, 1e-4f);         // respawned at center
    AF_CHECK_NEAR(p.z, 0.0f, 1e-4f);
    AF_CHECK_EQ(world.GetComponent<Health>(session.Player()).current, 2);
}

AF_TEST("twin sessions with identical input are bit-identical") {
    const int steps = 420;
    const float dt = 1.0f / 60.0f;

    GameSession a;
    GameSession b;
    for (int i = 0; i < steps; ++i) {
        const float t = static_cast<float>(i) * dt;
        PlayerInput input;
        input.move = Vec2{std::sin(t * 0.7f), std::cos(t * 0.5f)};
        input.jump = (i % 120) == 0;
        input.shoot = (i % 30) < 12;
        input.aim = Vec3{std::sin(t * 1.1f), 0.0f, -std::cos(t * 1.1f)};

        a.SetPlayerInput(input);
        b.SetPlayerInput(input);
        a.Update(dt);
        b.Update(dt);
    }

    AF_CHECK_EQ(a.Score(), b.Score());
    AF_CHECK_EQ(a.Kills(), b.Kills());
    AF_CHECK_EQ(a.EnemyCount(), b.EnemyCount());
    AF_CHECK_EQ(a.ProjectileCount(), b.ProjectileCount());
    AF_CHECK_EQ(a.PlayerAlive(), b.PlayerAlive());

    const Vec3 pa = a.GetWorld().GetComponent<Transform>(a.Player()).position;
    const Vec3 pb = b.GetWorld().GetComponent<Transform>(b.Player()).position;
    AF_CHECK_EQ(pa, pb);  // exact float equality: determinism claim
}

AF_TEST("session update allocates zero bytes in steady state") {
    GameSession::Config config;
    config.spawnInterval = 100.0f;  // spawner quiet during measurement
    GameSession session(config);

    // Warm up: shooting + spawning churn reaches steady state.
    PlayerInput input;
    input.shoot = true;
    input.aim = Vec3{1.0f, 0.0f, 0.0f};
    for (int i = 0; i < 300; ++i) {
        session.SetPlayerInput(input);
        session.Update(1.0f / 60.0f);
    }

    const long long before = af::test::AllocationCount();
    for (int i = 0; i < 600; ++i) {
        session.SetPlayerInput(input);
        session.Update(1.0f / 60.0f);
    }
    const long long after = af::test::AllocationCount();
    AF_CHECK_EQ(after, before);
}
