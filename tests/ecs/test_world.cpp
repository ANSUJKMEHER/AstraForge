#include "af/core/Components.h"
#include "af/ecs/World.h"
#include "framework/af_test.hpp"

using namespace af;

AF_TEST("world entity lifecycle") {
    World w;
    AF_CHECK_EQ(w.EntityCount(), 0u);
    const Entity e = w.CreateEntity();
    AF_CHECK_EQ(w.EntityCount(), 1u);
    AF_CHECK(w.IsAlive(e));
    w.DestroyEntity(e);
    AF_CHECK(!w.IsAlive(e));
    AF_CHECK_EQ(w.EntityCount(), 0u);
    w.DestroyEntity(e);  // idempotent
}

AF_TEST("component add, get, remove round trip") {
    World w;
    const Entity e = w.CreateEntity();
    AF_CHECK(!w.HasComponent<Transform>(e));
    AF_CHECK(w.TryGetComponent<Transform>(e) == nullptr);

    auto& t = w.AddComponent<Transform>(e);
    t.position = Vec3{1.0f, 2.0f, 3.0f};
    AF_CHECK(w.HasComponent<Transform>(e));
    AF_CHECK_NEAR(w.GetComponent<Transform>(e).position.y, 2.0f, 0.0f);
    AF_CHECK(w.TryGetComponent<Transform>(e) != nullptr);

    w.RemoveComponent<Transform>(e);
    AF_CHECK(!w.HasComponent<Transform>(e));
    AF_CHECK(w.TryGetComponent<Transform>(e) == nullptr);
}

AF_TEST("component re-add overwrites without duplicating") {
    World w;
    const Entity e = w.CreateEntity();
    w.AddComponent<Velocity>(e, Velocity{Vec3{1.0f, 0.0f, 0.0f}});
    w.AddComponent<Velocity>(e, Velocity{Vec3{5.0f, 0.0f, 0.0f}});
    AF_CHECK_NEAR(w.GetComponent<Velocity>(e).linear.x, 5.0f, 0.0f);
    std::size_t count = 0;
    for (auto [entity, v] : w.ViewComponents<Velocity>()) {
        (void)entity;
        (void)v;
        ++count;
    }
    AF_CHECK_EQ(count, 1u);
}

AF_TEST("destroy clears all components for the reused slot") {
    World w;
    const Entity a = w.CreateEntity();
    w.AddComponent<Transform>(a);
    w.AddComponent<Velocity>(a);
    w.DestroyEntity(a);
    const Entity b = w.CreateEntity();  // reuses slot 0
    AF_CHECK_EQ(b.id, a.id);
    AF_CHECK(!w.HasComponent<Transform>(b));
    AF_CHECK(!w.HasComponent<Velocity>(b));
    AF_CHECK_EQ(w.GetMask(b), 0u);
}

AF_TEST("signature mask tracks component membership") {
    World w;
    const Entity e = w.CreateEntity();
    AF_CHECK_EQ(w.GetMask(e), 0u);
    w.AddComponent<Transform>(e);
    const ComponentMask afterTransform = w.GetMask(e);
    AF_CHECK(afterTransform != 0u);
    AF_CHECK_EQ(afterTransform, ComponentMaskOf<Transform>());
    w.AddComponent<Velocity>(e);
    AF_CHECK_EQ(w.GetMask(e),
                ComponentMaskOf<Transform>() | ComponentMaskOf<Velocity>());
    w.RemoveComponent<Velocity>(e);
    AF_CHECK_EQ(w.GetMask(e), ComponentMaskOf<Transform>());
    w.RemoveComponent<Transform>(e);
    AF_CHECK_EQ(w.GetMask(e), 0u);
}

AF_TEST("dead handles are rejected defensively") {
    World w;
    const Entity e = w.CreateEntity();
    w.AddComponent<Transform>(e);
    w.DestroyEntity(e);
    // Defensive path: nullptr, no crash.
    AF_CHECK(w.TryGetComponent<Transform>(e) == nullptr);
    // Get/Has/Add on dead handles abort via AF_ASSERT — the documented
    // contract (always-on assertion, see af/core/Assert.h).
}
