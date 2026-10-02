// Headless gameplay runner — the full arena game without a window.
//
// Runs GameSession for N fixed steps with a scripted input (strafe, periodic
// jump + shoot, rotating aim). Deterministic: same args → same output (the
// only RNG is the spawner's seeded LCG). Used as the CI gameplay smoke test
// and as the seed of the Phase 9 benchmark harness.
//
// Usage: af_headless_game [step_count]

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "af/core/Components.h"
#include "af/game/GameSession.h"

using namespace af;

int main(int argc, char** argv) {
    const int stepCount = argc > 1 ? std::atoi(argv[1]) : 600;

    GameSession session;
    const float dt = 1.0f / 60.0f;

    const auto t0 = std::chrono::steady_clock::now();
    for (int step = 0; step < stepCount; ++step) {
        const float t = static_cast<float>(step) * dt;

        PlayerInput input;
        // Strafe in a slow circle.
        input.move = Vec2{std::sin(t * 0.7f), std::cos(t * 0.5f)};
        // Hop every ~2 s, hold fire in bursts.
        input.jump = (step % 120) == 0;
        input.shoot = (step % 30) < 12;
        // Rotating aim: sweeps the arena.
        input.aim = Vec3{std::sin(t * 1.1f), 0.0f, -std::cos(t * 1.1f)};

        session.SetPlayerInput(input);
        session.Update(dt);
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double elapsedMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::printf("steps=%d elapsed=%.2f ms (%.4f ms/step)\n", stepCount,
                elapsedMs, stepCount > 0 ? elapsedMs / stepCount : 0.0);
    std::printf("score=%d kills=%d enemies=%zu projectiles=%zu player_alive=%d\n",
                session.Score(), session.Kills(), session.EnemyCount(),
                session.ProjectileCount(), session.PlayerAlive() ? 1 : 0);
    std::printf("player.position=%.6f,%.6f,%.6f\n",
                session.GetWorld().GetComponent<Transform>(session.Player())
                    .position.x,
                session.GetWorld().GetComponent<Transform>(session.Player())
                    .position.y,
                session.GetWorld().GetComponent<Transform>(session.Player())
                    .position.z);
    return 0;
}
