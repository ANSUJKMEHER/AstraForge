#include "af/ecs/Entity.h"
#include "af/ecs/EntityManager.h"
#include "framework/af_test.hpp"

using namespace af;

AF_TEST("entity creation assigns sequential ids and generations") {
    EntityManager em;
    const Entity a = em.Create();
    const Entity b = em.Create();
    AF_CHECK_EQ(a.id, 0u);
    AF_CHECK_EQ(b.id, 1u);
    AF_CHECK_EQ(a.generation, 1u);  // first issued generation is 1 (0 = never issued)
    AF_CHECK(em.IsAlive(a));
    AF_CHECK(em.IsAlive(b));
    AF_CHECK_EQ(em.AliveCount(), 2u);
    AF_CHECK(!a.IsNull());
    AF_CHECK(Entity::Null().IsNull());
}

AF_TEST("destroy invalidates handles and is idempotent") {
    EntityManager em;
    const Entity a = em.Create();
    em.Destroy(a);
    AF_CHECK(!em.IsAlive(a));  // stale handle detected
    AF_CHECK_EQ(em.AliveCount(), 0u);
    em.Destroy(a);  // idempotent
    AF_CHECK_EQ(em.AliveCount(), 0u);
}

AF_TEST("slot reuse bumps generation") {
    EntityManager em;
    const Entity a = em.Create();
    const EntityId id = a.id;
    em.Destroy(a);
    const Entity b = em.Create();  // reuses the freed slot (LIFO free list)
    AF_CHECK_EQ(b.id, id);
    AF_CHECK(b.generation > a.generation);
    AF_CHECK(em.IsAlive(b));
    AF_CHECK(!em.IsAlive(a));  // the OLD handle must be rejected after reuse
}

AF_TEST("foreign or forged handles never validate") {
    EntityManager em;
    const Entity a = em.Create();
    AF_CHECK(em.IsAlive(a));
    AF_CHECK(!em.IsAlive(Entity{9999u, 1u}));  // id beyond capacity
    AF_CHECK(!em.IsAlive(Entity{a.id, 0u}));   // generation 0 is never issued
    // Destroy a and verify a forged handle with the right id but wrong
    // generation is rejected.
    em.Destroy(a);
    AF_CHECK(!em.IsAlive(Entity{a.id, a.generation}));
}

AF_TEST("free list reuses most recently destroyed slots") {
    EntityManager em;
    const Entity a = em.Create();
    const Entity b = em.Create();
    const Entity c = em.Create();
    em.Destroy(c);
    em.Destroy(b);
    const Entity d = em.Create();
    AF_CHECK_EQ(d.id, b.id);  // LIFO: b's slot comes back first
    const Entity e = em.Create();
    AF_CHECK_EQ(e.id, c.id);
    AF_CHECK(em.IsAlive(a));
    AF_CHECK_EQ(em.AliveCount(), 3u);
}
