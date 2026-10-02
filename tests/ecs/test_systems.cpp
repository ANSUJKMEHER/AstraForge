#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "framework/af_test.hpp"
#include "TestFixtures.h"

using namespace af;
using namespace af_test;

AF_TEST("systems run in registration order") {
    World w;
    for (int i = 0; i < 4; ++i) {
        w.AddComponent<Tag>(w.CreateEntity());
    }
    SystemManager mgr;
    mgr.AddSystem<SysSetTag>();    // tag.value = 1
    mgr.AddSystem<SysDoubleTag>(); // tag.value *= 2
    AF_CHECK_EQ(mgr.Size(), 2u);
    mgr.UpdateAll(w, 0.016f);
    // Order matters: if reversed, the value would be 0 -> 0, not 2.
    for (auto [entity, tag] : w.ViewComponents<Tag>()) {
        (void)entity;
        AF_CHECK_EQ(tag.value, 2);
    }
    mgr.Clear();
    AF_CHECK_EQ(mgr.Size(), 0u);
}

AF_TEST("fixed-step integration accumulates deterministically") {
    World w;
    for (int i = 0; i < 100; ++i) {
        const Entity e = w.CreateEntity();
        w.AddComponent<Transform>(e);
        w.AddComponent<Velocity>(e, Velocity{Vec3{0.1f, 0.0f, 0.0f}});
    }
    SystemManager mgr;
    mgr.AddSystem<SysMove>();
    for (int step = 0; step < 60; ++step) {
        mgr.UpdateAll(w, 1.0f / 60.0f);
    }
    // Reference computed with the exact same operation order as the system:
    // position += velocity * dt, 60 times. NEAR with eps 0 == bitwise equality.
    float ref = 0.0f;
    const float dt = 1.0f / 60.0f;
    for (int step = 0; step < 60; ++step) {
        ref += 0.1f * dt;
    }
    for (auto [entity, transform, velocity] :
         w.ViewComponents<Transform, Velocity>()) {
        (void)entity;
        (void)velocity;
        AF_CHECK_NEAR(transform.position.x, ref, 0.0f);
    }
}

AF_TEST("system chaining: later systems see earlier writes in the same tick") {
    World w;
    for (int i = 0; i < 8; ++i) {
        w.AddComponent<Tag>(w.CreateEntity());
    }
    SystemManager mgr;
    mgr.AddSystem<SysSetTag>();
    mgr.AddSystem<SysDoubleTag>();
    mgr.UpdateAll(w, 0.016f);
    // Single tick already shows the chained result: 1 * 2 = 2.
    for (auto [entity, tag] : w.ViewComponents<Tag>()) {
        (void)entity;
        AF_CHECK_EQ(tag.value, 2);
    }
}
