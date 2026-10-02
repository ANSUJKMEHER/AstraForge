#include "af/game/SpawnerSystem.h"

#include "af/core/Components.h"
#include "af/game/GameplayComponents.h"
#include "af/math/MathUtil.h"
#include "af/physics/PhysicsComponents.h"

namespace af {

namespace {

// LCG in [0,1) — same generator family as the collision benchmark.
float NextRand(uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>((state >> 8) & 0xFFFFFF) / 16777216.0f;
}

}  // namespace

SpawnerSystem::SpawnerSystem(Entity spawner, Vec2 arenaMin, Vec2 arenaMax)
    : spawner_(spawner), arenaMin_(arenaMin), arenaMax_(arenaMax) {}

void SpawnerSystem::Reset(World& world, Entity spawner, uint32_t seed) {
    if (!world.IsAlive(spawner)) return;
    EnemySpawner& s = world.GetComponent<EnemySpawner>(spawner);
    s.timer = 0.0f;
    s.rngState = seed;
}

void SpawnerSystem::Update(World& world, float dt) {
    if (!world.IsAlive(spawner_)) return;
    EnemySpawner& s = world.GetComponent<EnemySpawner>(spawner_);

    // Count live enemies (view iteration — no structural changes inside).
    std::size_t alive = 0;
    for (auto [entity, enemy] : world.ViewComponents<Enemy>()) {
        (void)entity;
        if (enemy.state != EnemyState::Dead) ++alive;
    }

    s.timer -= dt;
    if (s.timer > 0.0f || alive >= static_cast<std::size_t>(s.maxAlive)) return;

    s.timer = s.interval;

    // Deterministic placement: pick a random point on the arena edge ring.
    const float angle = NextRand(s.rngState) * 2.0f * Pi;
    const float inset = NextRand(s.rngState) * 0.4f + 0.6f;  // 0.6..1.0 of half-extent
    const float halfX = (arenaMax_.x - arenaMin_.x) * 0.5f;
    const float halfZ = (arenaMax_.y - arenaMin_.y) * 0.5f;
    const Vec3 pos{std::cos(angle) * halfX * inset, 1.0f,
                   std::sin(angle) * halfZ * inset};

    const Entity e = world.CreateEntity();
    world.AddComponent<Transform>(e).position = pos;
    world.AddComponent<Velocity>(e);
    world.AddComponent<Gravity>(e);
    world.AddComponent<Collider>(e, Collider{ColliderKind::Sphere, {},
                                             0.5f, {}});
    world.AddComponent<Enemy>(e);
    world.AddComponent<Health>(e, Health{3, 3});
}

}  // namespace af
