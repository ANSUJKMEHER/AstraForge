#include <algorithm>
#include <cstddef>
#include <vector>

#include "af/core/Components.h"
#include "af/ecs/World.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

// Every entity gets a Transform; entities divisible by 3 also get Velocity.
void BuildMixed(World& w, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        const Entity e = w.CreateEntity();
        w.AddComponent<Transform>(e).position.x = static_cast<float>(i);
        if (i % 3 == 0) {
            w.AddComponent<Velocity>(e).linear.x = 1.0f;
        }
    }
}

}  // namespace

AF_TEST("single-component view iterates every entity in dense order") {
    World w;
    BuildMixed(w, 10);
    std::size_t count = 0;
    for (auto [entity, transform] : w.ViewComponents<Transform>()) {
        (void)entity;
        // No removals happened, so dense order is creation order.
        AF_CHECK_NEAR(transform.position.x, static_cast<float>(count), 0.0f);
        ++count;
    }
    AF_CHECK_EQ(count, 10u);
}

AF_TEST("multi-component view filters to entities with all components") {
    World w;
    BuildMixed(w, 12);
    std::vector<uint32_t> seen;
    for (auto [entity, transform, velocity] :
         w.ViewComponents<Transform, Velocity>()) {
        (void)transform;
        (void)velocity;
        AF_CHECK(w.HasComponent<Velocity>(entity));
        seen.push_back(entity.id);
    }
    std::vector<uint32_t> expected{0, 3, 6, 9};
    std::sort(seen.begin(), seen.end());
    AF_CHECK(seen == expected);
}

AF_TEST("view mutation writes through to the world") {
    World w;
    BuildMixed(w, 5);
    for (auto [entity, transform] : w.ViewComponents<Transform>()) {
        (void)entity;
        transform.position.y = 42.0f;  // must modify the stored component
    }
    std::size_t count = 0;
    for (auto [entity, transform] : w.ViewComponents<Transform>()) {
        (void)entity;
        AF_CHECK_NEAR(transform.position.y, 42.0f, 0.0f);
        ++count;
    }
    AF_CHECK_EQ(count, 5u);
}

AF_TEST("view reflects component removal") {
    World w;
    Entity entities[6];
    for (std::size_t i = 0; i < 6; ++i) {
        entities[i] = w.CreateEntity();
        w.AddComponent<Transform>(entities[i]);
        w.AddComponent<Velocity>(entities[i]);
    }
    w.RemoveComponent<Velocity>(entities[1]);
    w.RemoveComponent<Velocity>(entities[4]);
    std::size_t count = 0;
    for (auto [entity, transform, velocity] :
         w.ViewComponents<Transform, Velocity>()) {
        (void)entity;
        (void)transform;
        (void)velocity;
        ++count;
    }
    AF_CHECK_EQ(count, 4u);
}

AF_TEST("view reflects entity destruction") {
    World w;
    Entity entities[6];
    for (std::size_t i = 0; i < 6; ++i) {
        entities[i] = w.CreateEntity();
        w.AddComponent<Transform>(entities[i]);
        w.AddComponent<Velocity>(entities[i]);
    }
    w.DestroyEntity(entities[2]);
    w.DestroyEntity(entities[5]);
    std::size_t count = 0;
    for (auto [entity, transform, velocity] :
         w.ViewComponents<Transform, Velocity>()) {
        (void)entity;
        (void)transform;
        (void)velocity;
        ++count;
    }
    AF_CHECK_EQ(count, 4u);
}

AF_TEST("view over an empty component type is empty") {
    World w;
    const Entity e = w.CreateEntity();  // no components at all
    w.AddComponent<Velocity>(e);
    std::size_t count = 0;
    for (auto [entity, transform] : w.ViewComponents<Transform>()) {
        (void)entity;
        (void)transform;
        ++count;
    }
    AF_CHECK_EQ(count, 0u);
}
