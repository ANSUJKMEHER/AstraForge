// Enemy FSM tests: transitions, chase movement, melee damage cadence,
// hurt recovery. Runs EnemyAISystem directly against scripted worlds.

#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/game/EnemyAISystem.h"
#include "af/game/GameplayComponents.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

Entity MakePlayer(World& w, Vec3 pos) {
    const Entity p = w.CreateEntity();
    w.AddComponent<Transform>(p).position = pos;
    w.AddComponent<Health>(p, Health{5, 5});
    return p;
}

Entity MakeEnemy(World& w, Vec3 pos, Enemy e = {}) {
    const Entity en = w.CreateEntity();
    w.AddComponent<Transform>(en).position = pos;
    w.AddComponent<Velocity>(en);
    w.AddComponent<Enemy>(en, e);
    return en;
}

}  // namespace

AF_TEST("idle enemy starts chasing when player enters detect range") {
    World w;
    const Entity player = MakePlayer(w, {0.0f, 0.0f, 0.0f});
    EnemyAISystem ai(player);

    Enemy far = {};
    far.detectRange = 10.0f;
    const Entity eFar = MakeEnemy(w, {20.0f, 0.0f, 0.0f}, far);
    Enemy near = {};
    near.detectRange = 10.0f;
    const Entity eNear = MakeEnemy(w, {5.0f, 0.0f, 0.0f}, near);

    ai.Update(w, 1.0f / 60.0f);
    AF_CHECK(w.GetComponent<Enemy>(eFar).state == EnemyState::Idle);
    AF_CHECK(w.GetComponent<Enemy>(eNear).state == EnemyState::Chase);
}

AF_TEST("chasing enemy moves toward the player at its speed") {
    World w;
    const Entity player = MakePlayer(w, {10.0f, 0.0f, 0.0f});
    EnemyAISystem ai(player);

    Enemy e = {};
    e.state = EnemyState::Chase;
    e.speed = 4.0f;
    const Entity en = MakeEnemy(w, {0.0f, 0.0f, 0.0f}, e);

    ai.Update(w, 1.0f / 60.0f);
    const Velocity& v = w.GetComponent<Velocity>(en);
    AF_CHECK_NEAR(v.linear.x, 4.0f, 1e-4f);   // toward +X
    AF_CHECK_NEAR(v.linear.z, 0.0f, 1e-4f);
    AF_CHECK_NEAR(v.linear.y, 0.0f, 1e-6f);   // AI never touches Y
}

AF_TEST("enemy attacks when in range, with cooldown between hits") {
    World w;
    const Entity player = MakePlayer(w, {0.0f, 0.0f, 0.0f});
    EnemyAISystem ai(player);

    Enemy e = {};
    e.state = EnemyState::Chase;
    e.attackRange = 2.0f;
    e.attackCooldown = 1.0f;
    e.damage = 2;
    const Entity en = MakeEnemy(w, {1.0f, 0.0f, 0.0f}, e);
    const float dt = 0.25f;  // binary-exact: 4 ticks == 1.0 s cooldown exactly

    // First update: enters Attack with a windup (no immediate damage).
    ai.Update(w, dt);
    AF_CHECK(w.GetComponent<Enemy>(en).state == EnemyState::Attack);
    AF_CHECK_EQ(w.GetComponent<Health>(player).current, 5);

    // 3 more ticks: 0.75 s of windup elapsed — still no damage.
    for (int i = 0; i < 3; ++i) ai.Update(w, dt);
    AF_CHECK_EQ(w.GetComponent<Health>(player).current, 5);
    ai.Update(w, dt);  // 4th tick: timer hits exactly 0.0 → damage lands
    AF_CHECK_EQ(w.GetComponent<Health>(player).current, 3);  // 5 - 2

    // Cooldown restarts: another 4 ticks before the next hit.
    for (int i = 0; i < 3; ++i) ai.Update(w, dt);
    AF_CHECK_EQ(w.GetComponent<Health>(player).current, 3);
    ai.Update(w, dt);
    AF_CHECK_EQ(w.GetComponent<Health>(player).current, 1);
}

AF_TEST("attacking enemy returns to chase when player leaves range") {
    World w;
    const Entity player = MakePlayer(w, {0.0f, 0.0f, 0.0f});
    EnemyAISystem ai(player);

    Enemy e = {};
    e.state = EnemyState::Attack;
    e.attackRange = 2.0f;
    const Entity en = MakeEnemy(w, {5.0f, 0.0f, 0.0f}, e);

    ai.Update(w, 1.0f / 60.0f);
    AF_CHECK(w.GetComponent<Enemy>(en).state == EnemyState::Chase);
}

AF_TEST("hurt enemy is frozen and recovers to chase after the timer") {
    World w;
    const Entity player = MakePlayer(w, {0.0f, 0.0f, 0.0f});
    EnemyAISystem ai(player);

    Enemy e = {};
    e.state = EnemyState::Hurt;
    e.hurtTime = 0.25f;
    e.hurtTimer = 0.25f;
    const Entity en = MakeEnemy(w, {0.0f, 0.0f, 0.0f}, e);
    const float dt = 0.125f;  // binary-exact: 2 ticks == 0.25 s exactly

    ai.Update(w, dt);
    AF_CHECK(w.GetComponent<Enemy>(en).state == EnemyState::Hurt);
    AF_CHECK_NEAR(w.GetComponent<Velocity>(en).linear.x, 0.0f, 1e-6f);

    ai.Update(w, dt);  // timer hits exactly 0.0 → Chase
    AF_CHECK(w.GetComponent<Enemy>(en).state == EnemyState::Chase);
}

AF_TEST("dead enemy is inert (no velocity, no state changes)") {
    World w;
    const Entity player = MakePlayer(w, {0.0f, 0.0f, 0.0f});
    EnemyAISystem ai(player);

    Enemy e = {};
    e.state = EnemyState::Dead;
    const Entity en = MakeEnemy(w, {0.5f, 0.0f, 0.0f}, e);

    w.GetComponent<Velocity>(en).linear = Vec3{3.0f, 0.0f, 0.0f};
    ai.Update(w, 1.0f / 60.0f);
    AF_CHECK(w.GetComponent<Enemy>(en).state == EnemyState::Dead);
    AF_CHECK_NEAR(w.GetComponent<Velocity>(en).linear.x, 0.0f, 1e-6f);
}

AF_TEST("enemies ignore a dead player") {
    World w;
    const Entity player = MakePlayer(w, {0.0f, 0.0f, 0.0f});
    w.GetComponent<Health>(player).current = 0;
    EnemyAISystem ai(player);

    Enemy e = {};
    e.state = EnemyState::Chase;
    e.speed = 5.0f;
    const Entity en = MakeEnemy(w, {1.0f, 0.0f, 0.0f}, e);

    ai.Update(w, 1.0f / 60.0f);
    AF_CHECK(w.GetComponent<Enemy>(en).state == EnemyState::Chase);
    AF_CHECK_NEAR(w.GetComponent<Velocity>(en).linear.x, 0.0f, 1e-6f);
}
