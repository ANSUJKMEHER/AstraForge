// Headless simulation runner — runs the engine without a window or GPU.
//
// Purpose: CI smoke test, deterministic simulation verification, and the seed
// of the benchmark harness (extended in Phases 3/9). This is also how the
// gameplay logic gets verified in environments without a display.
//
// Scene: N bodies with gravity above a ground plane at y = 0. They fall and
// come to rest — the final state is deterministic (verified by the twin-world
// tests) and entity0's y prints as a run-to-run stability probe.
//
// Usage: af_headless [entity_count] [step_count]

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/physics/PhysicsComponents.h"
#include "af/physics/PhysicsSystem.h"

using namespace af;

int main(int argc, char** argv) {
    const std::size_t entityCount =
        argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 1000;
    const int stepCount = argc > 2 ? std::atoi(argv[2]) : 600;

    World world;
    SystemManager systems;
    systems.AddSystem<PhysicsSystem>(0.0f);

    Entity first = Entity::Null();
    for (std::size_t i = 0; i < entityCount; ++i) {
        const Entity e = world.CreateEntity();
        if (i == 0) first = e;
        world.AddComponent<Transform>(e).position =
            Vec3{static_cast<float>(i % 20) * 0.5f - 5.0f, 1.0f,
                 static_cast<float>(i / 20) * 0.5f - 5.0f};
        world.AddComponent<Velocity>(e);
        world.AddComponent<Gravity>(e);
    }

    const float dt = 1.0f / 60.0f;
    const auto t0 = std::chrono::steady_clock::now();
    for (int step = 0; step < stepCount; ++step) {
        systems.UpdateAll(world, dt);
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double elapsedMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::printf("entities=%zu steps=%d elapsed=%.2f ms (%.4f ms/step)\n",
                entityCount, stepCount, elapsedMs,
                stepCount > 0 ? elapsedMs / static_cast<double>(stepCount) : 0.0);

    if (!first.IsNull() && world.IsAlive(first)) {
        const auto& t = world.GetComponent<Transform>(first);
        std::printf("entity0.position.y=%.6f\n", t.position.y);
    }

    return 0;
}
