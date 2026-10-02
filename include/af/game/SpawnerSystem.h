#pragma once
// SpawnerSystem — spawns enemies near the arena edge on a fixed interval,
// up to maxAlive. Spawn positions come from a deterministic LCG stored in
// the EnemySpawner component, so headless runs are bit-reproducible.
//
// Spawning is structural (CreateEntity + components) and happens AFTER the
// counting loop, never during view iteration.

#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/math/Vec2.h"

namespace af {

class SpawnerSystem : public System {
public:
    SpawnerSystem(Entity spawner, Vec2 arenaMin, Vec2 arenaMax);

    void Update(World& world, float dt) override;

    // Restart support: resets timer and RNG seed in the spawner component.
    static void Reset(World& world, Entity spawner, uint32_t seed);

private:
    Entity spawner_;
    Vec2 arenaMin_;
    Vec2 arenaMax_;
};

}  // namespace af
