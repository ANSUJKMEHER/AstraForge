// Combat system tests: projectile-vs-enemy hits, damage, hurt/dead
// transitions, projectile consumption.

#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/game/CombatSystem.h"
#include "af/game/GameplayComponents.h"
#include "af/game/ProjectilePool.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

Entity MakeEnemy(World& w, Vec3 pos) {
    const Entity e = w.CreateEntity();
    w.AddComponent<Transform>(e).position = pos;
    w.AddComponent<Collider>(e, Collider{ColliderKind::Sphere, {}, 0.5f, {}});
    w.AddComponent<Enemy>(e);
    w.AddComponent<Health>(e, Health{3, 3});
    return e;
}

}  // namespace

AF_TEST("projectile overlapping an enemy damages it and is consumed") {
    World w;
    ProjectilePool pool(w, 8);
    CombatSystem combat(pool);

    const Entity enemy = MakeEnemy(w, {0.0f, 0.0f, 0.0f});
    const Entity projectile =
        pool.Spawn({0.5f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 20.0f, 2.0f,
                   0.25f, 1, Entity::Null());

    combat.Update(w, 1.0f / 60.0f);

    AF_CHECK_EQ(combat.LastStats().hits, 1u);
    AF_CHECK_EQ(w.GetComponent<Health>(enemy).current, 2);        // 3 - 1
    AF_CHECK(w.GetComponent<Enemy>(enemy).state == EnemyState::Hurt);
    AF_CHECK_EQ(pool.LiveCount(), 0u);  // projectile returned to the pool
}

AF_TEST("projectile that misses leaves the enemy untouched") {
    World w;
    ProjectilePool pool(w, 8);
    CombatSystem combat(pool);

    const Entity enemy = MakeEnemy(w, {0.0f, 0.0f, 0.0f});
    pool.Spawn({5.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 20.0f, 2.0f, 0.25f, 1,
               Entity::Null());

    combat.Update(w, 1.0f / 60.0f);
    AF_CHECK_EQ(combat.LastStats().hits, 0u);
    AF_CHECK_EQ(w.GetComponent<Health>(enemy).current, 3);
    AF_CHECK_EQ(pool.LiveCount(), 1u);
}

AF_TEST("lethal hit kills the enemy and reports a kill") {
    World w;
    ProjectilePool pool(w, 8);
    CombatSystem combat(pool);

    const Entity enemy = MakeEnemy(w, {0.0f, 0.0f, 0.0f});
    w.GetComponent<Health>(enemy).current = 1;

    pool.Spawn({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 20.0f, 2.0f, 0.25f, 1,
               Entity::Null());

    combat.Update(w, 1.0f / 60.0f);
    AF_CHECK_EQ(combat.LastStats().kills, 1u);
    AF_CHECK_EQ(w.GetComponent<Health>(enemy).current, 0);
    AF_CHECK(w.GetComponent<Enemy>(enemy).state == EnemyState::Dead);
}

AF_TEST("one projectile hits only one enemy") {
    World w;
    ProjectilePool pool(w, 8);
    CombatSystem combat(pool);

    const Entity a = MakeEnemy(w, {0.0f, 0.0f, 0.0f});
    const Entity b = MakeEnemy(w, {0.4f, 0.0f, 0.0f});  // overlaps both radii
    pool.Spawn({0.5f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 20.0f, 2.0f, 0.25f, 1,
               Entity::Null());

    combat.Update(w, 1.0f / 60.0f);
    AF_CHECK_EQ(combat.LastStats().hits, 1u);
    const int damaged = (w.GetComponent<Health>(a).current == 2 ? 1 : 0) +
                        (w.GetComponent<Health>(b).current == 2 ? 1 : 0);
    AF_CHECK_EQ(damaged, 1);
}

AF_TEST("dead enemies are ignored by combat") {
    World w;
    ProjectilePool pool(w, 8);
    CombatSystem combat(pool);

    const Entity enemy = MakeEnemy(w, {0.0f, 0.0f, 0.0f});
    w.GetComponent<Enemy>(enemy).state = EnemyState::Dead;
    pool.Spawn({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 20.0f, 2.0f, 0.25f, 1,
               Entity::Null());

    combat.Update(w, 1.0f / 60.0f);
    AF_CHECK_EQ(combat.LastStats().hits, 0u);
    AF_CHECK_EQ(pool.LiveCount(), 1u);  // projectile flies on
}
