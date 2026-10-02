#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "framework/af_test.hpp"
#include "TestFixtures.h"

using namespace af;
using namespace af_test;

AF_TEST("twin worlds produce bit-identical state") {
    // Two worlds built identically and stepped in interleaved order must end
    // up bit-identical. This catches uninitialized reads and hidden global
    // state — the classic non-determinism sources.
    World a;
    World b;
    SystemManager sa;
    SystemManager sb;
    sa.AddSystem<SysMove>();
    sb.AddSystem<SysMove>();

    for (int i = 0; i < 500; ++i) {
        for (World* w : {&a, &b}) {
            const Entity e = w->CreateEntity();
            w->AddComponent<Transform>(e);
            w->AddComponent<Velocity>(
                e, Velocity{Vec3{static_cast<float>(i) * 0.01f, 0.5f, -0.25f}});
        }
    }

    for (int step = 0; step < 240; ++step) {
        sa.UpdateAll(a, 1.0f / 60.0f);
        sb.UpdateAll(b, 1.0f / 60.0f);
    }

    auto va = a.ViewComponents<Transform, Velocity>();
    auto vb = b.ViewComponents<Transform, Velocity>();
    auto ia = va.begin();
    auto ib = vb.begin();
    std::size_t count = 0;
    for (; ia != va.end(); ++ia, ++ib) {
        auto [ea, ta, va2] = *ia;
        auto [eb, tb, vb2] = *ib;
        (void)va2;
        (void)vb2;
        AF_CHECK_EQ(ea.id, eb.id);
        AF_CHECK_EQ(ea.generation, eb.generation);
        AF_CHECK_EQ(ta.position.x, tb.position.x);
        AF_CHECK_EQ(ta.position.y, tb.position.y);
        AF_CHECK_EQ(ta.position.z, tb.position.z);
        ++count;
    }
    AF_CHECK_EQ(count, 500u);
}
