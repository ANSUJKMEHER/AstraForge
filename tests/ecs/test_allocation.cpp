// Verifies the ECS steady-state no-allocation guarantee. The global operator
// new counter lives in tests/framework/allocation_probe.cpp (shared by the
// collision tests); AllocationCount() deltas are measured around regions.

#include <cstddef>
#include <vector>

#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "framework/AllocationProbe.h"
#include "framework/af_test.hpp"
#include "TestFixtures.h"

using namespace af;
using namespace af_test;

AF_TEST("steady-state simulation allocates zero bytes") {
    World w;
    SystemManager mgr;
    mgr.AddSystem<SysMove>();
    for (int i = 0; i < 1000; ++i) {
        const Entity e = w.CreateEntity();
        w.AddComponent<Transform>(e);
        w.AddComponent<Velocity>(e, Velocity{Vec3{0.1f, 0.0f, 0.0f}});
    }
    // Warm up: any lazy growth (component vectors, masks) happens here.
    for (int i = 0; i < 5; ++i) mgr.UpdateAll(w, 1.0f / 60.0f);

    const long long before = af::test::AllocationCount();
    for (int i = 0; i < 600; ++i) mgr.UpdateAll(w, 1.0f / 60.0f);
    const long long after = af::test::AllocationCount();
    AF_CHECK_EQ(after, before);
}

AF_TEST("entity recycling allocates zero bytes in steady state") {
    World w;
    std::vector<Entity> pool;
    for (int i = 0; i < 1000; ++i) {
        const Entity e = w.CreateEntity();
        w.AddComponent<Transform>(e);
        w.AddComponent<Velocity>(e, Velocity{});
        pool.push_back(e);
    }

    // Warm up the churn once: the free list grows to its peak size here.
    for (const Entity e : pool) w.DestroyEntity(e);
    for (Entity& e : pool) {
        e = w.CreateEntity();
        w.AddComponent<Transform>(e);
        w.AddComponent<Velocity>(e, Velocity{});
    }

    const long long before = af::test::AllocationCount();
    for (int frame = 0; frame < 100; ++frame) {
        // Full churn each frame: destroy all, recreate all.
        for (const Entity e : pool) w.DestroyEntity(e);
        for (Entity& e : pool) {
            e = w.CreateEntity();
            w.AddComponent<Transform>(e);
            w.AddComponent<Velocity>(e, Velocity{});
        }
    }
    const long long after = af::test::AllocationCount();
    AF_CHECK_EQ(after, before);
    AF_CHECK_EQ(w.EntityCount(), 1000u);
}
