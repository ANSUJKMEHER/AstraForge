#include "af/core/Components.h"
#include "af/ecs/World.h"
#include "af/game/GameplayComponents.h"
#include "af/game/ProjectilePool.h"
#include "framework/AllocationProbe.h"
#include "framework/af_test.hpp"

using namespace af;

AF_TEST("projectile pool spawns, tracks live count, and despawns") {
    World w;
    ProjectilePool pool(w, 4);

    AF_CHECK_EQ(pool.Capacity(), 4u);
    AF_CHECK_EQ(pool.LiveCount(), 0u);
    AF_CHECK_EQ(pool.FreeCount(), 4u);

    const Entity a = pool.Spawn({0, 0, 0}, {0, 0, -1}, 20.0f, 2.0f, 0.25f, 1,
                                Entity::Null());
    const Entity b = pool.Spawn({1, 0, 0}, {0, 0, -1}, 20.0f, 2.0f, 0.25f, 1,
                                Entity::Null());
    AF_CHECK(!a.IsNull());
    AF_CHECK(!b.IsNull());
    AF_CHECK(a != b);
    AF_CHECK_EQ(pool.LiveCount(), 2u);
    AF_CHECK_EQ(pool.FreeCount(), 2u);

    // Spawned entities carry Transform + Projectile with the requested data.
    AF_CHECK_NEAR(w.GetComponent<Transform>(a).position.x, 0.0f, 1e-6f);
    const Projectile& p = w.GetComponent<Projectile>(a);
    AF_CHECK_NEAR(p.speed, 20.0f, 1e-6f);
    AF_CHECK_NEAR(p.life, 2.0f, 1e-6f);
    AF_CHECK_EQ(p.damage, 1);

    pool.Despawn(a);
    AF_CHECK_EQ(pool.LiveCount(), 1u);
    AF_CHECK_EQ(pool.FreeCount(), 3u);
    AF_CHECK(!w.HasComponent<Projectile>(a));       // components removed
    AF_CHECK(!w.HasComponent<Transform>(a));

    pool.Despawn(b);
    AF_CHECK_EQ(pool.LiveCount(), 0u);
    AF_CHECK_EQ(pool.FreeCount(), 4u);
}

AF_TEST("projectile pool reuses parked entities (no new handles)") {
    World w;
    ProjectilePool pool(w, 2);

    const Entity first = pool.Spawn({0, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f,
                                    1, Entity::Null());
    const std::size_t aliveBefore = w.EntityCount();
    pool.Despawn(first);
    pool.Spawn({5, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1, Entity::Null());

    // Parked entity was reused — the world never grew.
    AF_CHECK_EQ(w.EntityCount(), aliveBefore);
}

AF_TEST("projectile pool returns null entity when exhausted") {
    World w;
    ProjectilePool pool(w, 2);
    pool.Spawn({0, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1, Entity::Null());
    pool.Spawn({0, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1, Entity::Null());
    const Entity overflow =
        pool.Spawn({0, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1, Entity::Null());
    AF_CHECK(overflow.IsNull());
    AF_CHECK_EQ(pool.LiveCount(), 2u);
}

AF_TEST("projectile pool despawn is idempotent and ignores non-projectiles") {
    World w;
    ProjectilePool pool(w, 2);

    // Despawning an arbitrary (non-projectile) entity is a no-op.
    const Entity plain = w.CreateEntity();
    pool.Despawn(plain);
    AF_CHECK_EQ(pool.LiveCount(), 0u);

    // Despawning twice doesn't corrupt the free list.
    const Entity p = pool.Spawn({0, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1,
                                Entity::Null());
    pool.Despawn(p);
    pool.Despawn(p);
    AF_CHECK_EQ(pool.FreeCount(), 2u);
    AF_CHECK_EQ(pool.LiveCount(), 0u);
}

AF_TEST("projectile pool despawn_all parks every live projectile") {
    World w;
    ProjectilePool pool(w, 4);
    pool.Spawn({0, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1, Entity::Null());
    pool.Spawn({1, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1, Entity::Null());
    pool.Spawn({2, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1, Entity::Null());

    pool.DespawnAll();
    AF_CHECK_EQ(pool.LiveCount(), 0u);
    AF_CHECK_EQ(pool.FreeCount(), 4u);
}

AF_TEST("projectile pool spawn/despawn cycles allocate zero bytes") {
    World w;
    ProjectilePool pool(w, 16);
    const Entity owner = w.CreateEntity();

    // Warm up: full cycles so all component storage reaches steady state.
    for (int i = 0; i < 32; ++i) {
        const Entity p =
            pool.Spawn({0, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1, owner);
        pool.Despawn(p);
    }

    const long long before = af::test::AllocationCount();
    for (int i = 0; i < 1000; ++i) {
        const Entity p =
            pool.Spawn({0, 0, 0}, {0, 0, -1}, 20.0f, 1.0f, 0.25f, 1, owner);
        pool.Despawn(p);
    }
    const long long after = af::test::AllocationCount();
    AF_CHECK_EQ(after, before);
}
