#include "af/game/ProjectileSystem.h"

#include "af/core/Components.h"
#include "af/game/GameplayComponents.h"
#include "af/math/MathUtil.h"

namespace af {

ProjectileSystem::ProjectileSystem(ProjectilePool& pool, Vec2 arenaMin,
                                   Vec2 arenaMax)
    : pool_(pool), arenaMin_(arenaMin), arenaMax_(arenaMax) {}

void ProjectileSystem::Update(World& world, float dt) {
    expired_.clear();
    for (auto [entity, transform, projectile] :
         world.ViewComponents<Transform, Projectile>()) {
        transform.position += projectile.dir * (projectile.speed * dt);
        projectile.life -= dt;

        const Vec3& p = transform.position;
        const bool outOfArena = p.x < arenaMin_.x || p.x > arenaMax_.x ||
                                p.z < arenaMin_.y || p.z > arenaMax_.y;
        if (projectile.life <= 0.0f || outOfArena) {
            expired_.push_back(entity);
        }
    }
    for (const Entity e : expired_) pool_.Despawn(e);
}

}  // namespace af
