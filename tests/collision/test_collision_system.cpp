#include "af/collision/CollisionSystem.h"
#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "framework/af_test.hpp"

using namespace af;

AF_TEST("collision system detects and separates overlapping spheres") {
    World w;
    const Entity a = w.CreateEntity();
    w.AddComponent<Transform>(a).position = Vec3{0.0f, 0.0f, 0.0f};
    w.AddComponent<Collider>(a, Collider{ColliderKind::Sphere, {}, 1.0f, {}});
    const Entity b = w.CreateEntity();
    w.AddComponent<Transform>(b).position = Vec3{1.5f, 0.0f, 0.0f};
    w.AddComponent<Collider>(b, Collider{ColliderKind::Sphere, {}, 1.0f, {}});

    CollisionSystem sys({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    sys.Update(w, 1.0f / 60.0f);

    AF_CHECK_EQ(sys.LastStats().contacts, 1u);
    // Each moved half the 0.5 penetration: distance becomes 2.0 (touching).
    const Vec3 pa = w.GetComponent<Transform>(a).position;
    const Vec3 pb = w.GetComponent<Transform>(b).position;
    AF_CHECK_NEAR(Distance(pa, pb), 2.0f, 1e-4f);
}

AF_TEST("collision system leaves separated objects alone") {
    World w;
    const Entity a = w.CreateEntity();
    w.AddComponent<Transform>(a).position = Vec3{0.0f, 0.0f, 0.0f};
    w.AddComponent<Collider>(a, Collider{ColliderKind::Sphere, {}, 1.0f, {}});
    const Entity b = w.CreateEntity();
    w.AddComponent<Transform>(b).position = Vec3{5.0f, 0.0f, 0.0f};
    w.AddComponent<Collider>(b, Collider{ColliderKind::Sphere, {}, 1.0f, {}});

    CollisionSystem sys({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    sys.Update(w, 1.0f / 60.0f);
    AF_CHECK_EQ(sys.LastStats().contacts, 0u);
    AF_CHECK_NEAR(w.GetComponent<Transform>(a).position.x, 0.0f, 1e-6f);
}

AF_TEST("collision system reports candidate counts") {
    World w;
    // 100 spheres packed densely in a corner: many candidates, some contacts.
    for (int i = 0; i < 100; ++i) {
        const Entity e = w.CreateEntity();
        w.AddComponent<Transform>(e).position =
            Vec3{static_cast<float>(i % 10) * 0.4f, 0.0f, static_cast<float>(i / 10) * 0.4f};
        w.AddComponent<Collider>(e, Collider{ColliderKind::Sphere, {}, 0.5f, {}});
    }
    CollisionSystem sys({-20.0f, -20.0f}, {20.0f, 20.0f}, 1.0f);
    sys.Update(w, 1.0f / 60.0f);
    const auto& stats = sys.LastStats();
    AF_CHECK_EQ(stats.entries, 100u);
    AF_CHECK(stats.candidatePairs > 0u);
    AF_CHECK(stats.contacts > 0u);
    // Candidates never exceed the naive pair count.
    AF_CHECK(stats.candidatePairs <= 100u * 99u / 2u);
}

AF_TEST("collision system supports mixed sphere and AABB colliders") {
    World w;
    const Entity box = w.CreateEntity();
    w.AddComponent<Transform>(box).position = Vec3{0.0f, 0.0f, 0.0f};
    w.AddComponent<Collider>(box, Collider{ColliderKind::AABB, {},
                                           0.0f, {0.5f, 0.5f, 0.5f}});
    const Entity ball = w.CreateEntity();
    w.AddComponent<Transform>(ball).position = Vec3{0.8f, 0.0f, 0.0f};
    w.AddComponent<Collider>(ball, Collider{ColliderKind::Sphere, {},
                                            0.5f, {}});

    CollisionSystem sys({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    sys.Update(w, 1.0f / 60.0f);
    AF_CHECK_EQ(sys.LastStats().contacts, 1u);
    // Separation resolves to touching: box face moves to 0.4, sphere to 0.9.
    const Vec3 p = w.GetComponent<Transform>(ball).position;
    AF_CHECK_NEAR(p.x, 0.9f, 1e-4f);
}

AF_TEST("collision system ignores entities without colliders") {
    World w;
    const Entity a = w.CreateEntity();
    w.AddComponent<Transform>(a).position = Vec3{0.0f, 0.0f, 0.0f};
    w.AddComponent<Collider>(a, Collider{ColliderKind::Sphere, {}, 1.0f, {}});
    const Entity b = w.CreateEntity();
    w.AddComponent<Transform>(b).position = Vec3{0.5f, 0.0f, 0.0f};  // overlaps, no collider

    CollisionSystem sys({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    sys.Update(w, 1.0f / 60.0f);
    AF_CHECK_EQ(sys.LastStats().contacts, 0u);
}

AF_TEST("static collider pushes a dynamic sphere out but never moves") {
    World w;
    // Static wall at x = 2 (AABB, thin slab).
    const Entity wall = w.CreateEntity();
    w.AddComponent<Transform>(wall).position = Vec3{2.0f, 0.0f, 0.0f};
    w.AddComponent<Collider>(
        wall, Collider{ColliderKind::AABB, {}, 0.0f,
                       {0.5f, 5.0f, 5.0f}, true});
    // Dynamic sphere overlapping the wall face.
    const Entity ball = w.CreateEntity();
    w.AddComponent<Transform>(ball).position = Vec3{2.3f, 0.0f, 0.0f};
    w.AddComponent<Collider>(ball, Collider{ColliderKind::Sphere, {},
                                            1.0f, {}});

    CollisionSystem sys({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    sys.Update(w, 1.0f / 60.0f);

    const auto& stats = sys.LastStats();
    AF_CHECK_EQ(stats.entries, 1u);         // statics excluded from the grid
    AF_CHECK_EQ(stats.staticContacts, 1u);  // resolved against the wall

    // Sphere pushed out by the FULL penetration: face at x = 2.5 (2 + 0.5),
    // sphere center lands at 3.5 (face + radius).
    const Vec3 ballPos = w.GetComponent<Transform>(ball).position;
    AF_CHECK_NEAR(ballPos.x, 3.5f, 1e-3f);
    // The wall did not move.
    AF_CHECK_NEAR(w.GetComponent<Transform>(wall).position.x, 2.0f, 1e-6f);
}

AF_TEST("static colliders neither respond to each other nor generate contacts") {
    World w;
    const Entity w1 = w.CreateEntity();
    w.AddComponent<Transform>(w1).position = Vec3{0.0f, 0.0f, 0.0f};
    w.AddComponent<Collider>(
        w1, Collider{ColliderKind::AABB, {}, 0.0f, {2.0f, 2.0f, 2.0f}, true});
    const Entity w2 = w.CreateEntity();
    w.AddComponent<Transform>(w2).position = Vec3{1.0f, 0.0f, 0.0f};
    w.AddComponent<Collider>(
        w2, Collider{ColliderKind::AABB, {}, 0.0f, {2.0f, 2.0f, 2.0f}, true});

    CollisionSystem sys({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    sys.Update(w, 1.0f / 60.0f);
    const auto& stats = sys.LastStats();
    AF_CHECK_EQ(stats.entries, 0u);
    AF_CHECK_EQ(stats.staticContacts, 0u);
    AF_CHECK_NEAR(w.GetComponent<Transform>(w1).position.x, 0.0f, 1e-6f);
    AF_CHECK_NEAR(w.GetComponent<Transform>(w2).position.x, 1.0f, 1e-6f);
}

AF_TEST("dynamic-dynamic and dynamic-static both resolve in one frame") {
    World w;
    const Entity wall = w.CreateEntity();
    w.AddComponent<Transform>(wall).position = Vec3{0.0f, 0.0f, 4.0f};
    w.AddComponent<Collider>(
        wall, Collider{ColliderKind::AABB, {}, 0.0f, {4.0f, 2.0f, 0.5f}, true});
    const Entity a = w.CreateEntity();
    w.AddComponent<Transform>(a).position = Vec3{0.0f, 0.0f, 3.0f};
    w.AddComponent<Collider>(a, Collider{ColliderKind::Sphere, {}, 1.0f, {}});
    const Entity b = w.CreateEntity();
    w.AddComponent<Transform>(b).position = Vec3{0.0f, 0.0f, 1.5f};
    w.AddComponent<Collider>(b, Collider{ColliderKind::Sphere, {}, 1.0f, {}});

    CollisionSystem sys({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    sys.Update(w, 1.0f / 60.0f);
    const auto& stats = sys.LastStats();
    AF_CHECK_EQ(stats.contacts, 1u);        // a-b overlap resolved
    AF_CHECK_EQ(stats.staticContacts, 1u);  // a-wall overlap resolved
}
