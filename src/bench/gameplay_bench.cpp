// Gameplay benchmark — the full arena session under stress (Phase 9).
//
// Spawns N enemies directly into the session world (bypassing the spawner
// cap), then drives the session with scripted input (constant fire + strafe)
// for `frames` fixed steps, timing each system group with the profiler.
//
// Deterministic: fixed seed placement, scripted input, fixed step count.
//
// Usage: af_bench_gameplay [enemy_count] [frames]

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "af/core/Components.h"
#include "af/core/Profiling.h"
#include "af/game/GameSession.h"
#include "af/game/GameplayComponents.h"
#include "af/physics/PhysicsComponents.h"

using namespace af;

namespace {

float NextRand(uint32_t& seed) {
    seed = seed * 1664525u + 1013904223u;
    return static_cast<float>((seed >> 8) & 0xFFFFFF) / 16777216.0f - 0.5f;
}

// Drops N enemies at deterministic pseudo-random positions. The arena scales
// with the population (constant density ~0.11 enemies/m²) so the stress test
// measures per-system scaling, not a degenerate overlap soup — a fixed 40×40
// arena saturates narrowphase contacts at ~5k entities (measured; see
// docs/performance.md).
void InjectEnemies(GameSession& session, std::size_t count, uint32_t seed) {
    World& world = session.GetWorld();
    const float half = std::sqrt(static_cast<float>(count)) * 1.5f;
    for (std::size_t i = 0; i < count; ++i) {
        const Entity e = world.CreateEntity();
        world.AddComponent<Transform>(e).position =
            Vec3{NextRand(seed) * half * 2.0f, 1.0f,
                 NextRand(seed) * half * 2.0f};
        world.AddComponent<Velocity>(e);
        world.AddComponent<Gravity>(e);
        world.AddComponent<Collider>(e, Collider{ColliderKind::Sphere, {},
                                                 0.5f, {}});
        world.AddComponent<Enemy>(e);
        world.AddComponent<Health>(e, Health{3, 3});
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::size_t enemyCount =
        argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 1000;
    const int frames = argc > 2 ? std::atoi(argv[2]) : 600;

    // Spawner must stay quiet: measure the injected population, not spawn
    // churn. Enemies spawn at the edge; injections are uniform.
    // Arena scales with the population (constant density).
    const float half = std::sqrt(static_cast<float>(enemyCount)) * 1.5f;
    GameSession::Config config;
    config.arenaMin = Vec2{-half, -half};
    config.arenaMax = Vec2{half, half};
    config.spawnInterval = 1.0e9f;
    config.maxEnemies = static_cast<int>(enemyCount) + 8;
    // Huge health: the benchmark measures a stable stress population, not the
    // death/restart flow (covered by tests). Melee damage saturates int range
    // only after ~2e9 hits — far beyond any measured window.
    config.playerHealth = 100000000;
    GameSession session(config);
    InjectEnemies(session, enemyCount, 12345u);

    // Warm up: systems + pools + profiler slots reach steady state.
    const float dt = 1.0f / 60.0f;
    PlayerInput input;
    input.shoot = true;
    for (int i = 0; i < 60; ++i) {
        input.move = Vec2{static_cast<float>(i % 2), 0.0f};
        input.aim = Vec3{1.0f, 0.0f, 0.0f};
        session.SetPlayerInput(input);
        session.Update(dt);
    }

    Profiler& prof = Profiler::Instance();
    prof.Reset();
    for (int i = 0; i < frames; ++i) {
        const float t = static_cast<float>(i) * dt;
        input.move = Vec2{std::sin(t * 0.7f), std::cos(t * 0.5f)};
        input.aim = Vec3{std::sin(t * 1.1f), 0.0f, -std::cos(t * 1.1f)};
        prof.BeginFrame();
        session.SetPlayerInput(input);
        session.UpdateInstrumented(dt);
        prof.EndFrame();
    }

    std::printf("enemies=%zu frames=%d\n", enemyCount, frames);
    std::printf("avg_frame=%.4f ms (%.1f fps equivalent)\n",
                prof.AverageFrameMs(),
                1000.0 / (prof.AverageFrameMs() > 0.0 ? prof.AverageFrameMs()
                                                      : 1.0));
    // Per-system breakdown (average ms per frame over the run).
    std::printf("system,ms_per_frame,calls\n");
    for (const auto& s : prof.LastFrameScopes()) {
        if (s.calls == 0) continue;
        std::printf("%s,%.4f,%zu\n", s.name, s.ms / static_cast<double>(s.calls),
                    s.calls);
    }
    std::printf("score=%d kills=%d enemies_alive=%zu projectiles=%zu\n",
                session.Score(), session.Kills(), session.EnemyCount(),
                session.ProjectileCount());
    const auto& combatStats = session.CombatStats();
    std::printf("combat_hits=%zu combat_kills=%zu\n", combatStats.hits,
                combatStats.kills);
    return 0;
}
