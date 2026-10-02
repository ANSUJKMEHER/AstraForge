#pragma once
// ProjectileSystem — advances pooled projectiles and expires them.
//
// Position += dir·speed·dt, life -= dt. Projectiles leaving the arena or
// running out of life are returned to the pool (deferred — collected during
// iteration, despawned after).

#include <vector>

#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/game/ProjectilePool.h"
#include "af/math/Vec2.h"

namespace af {

class ProjectileSystem : public System {
public:
    ProjectileSystem(ProjectilePool& pool, Vec2 arenaMin, Vec2 arenaMax);

    void Update(World& world, float dt) override;

private:
    ProjectilePool& pool_;
    Vec2 arenaMin_;
    Vec2 arenaMax_;
    std::vector<Entity> expired_;  // scratch, reused across frames
};

}  // namespace af
