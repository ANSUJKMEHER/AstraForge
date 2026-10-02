#include "af/game/ProjectilePool.h"

#include "af/core/Components.h"
#include "af/game/GameplayComponents.h"

namespace af {

ProjectilePool::ProjectilePool(World& world, std::size_t capacity)
    : world_(world) {
    free_.reserve(capacity);
    for (std::size_t i = 0; i < capacity; ++i) {
        free_.push_back(world_.CreateEntity());
    }
}

Entity ProjectilePool::Spawn(const Vec3& position, const Vec3& dir, float speed,
                             float life, float radius, int damage, Entity owner) {
    if (free_.empty()) return Entity::Null();  // pool exhausted: drop the shot

    const Entity e = free_.back();
    free_.pop_back();
    world_.AddComponent<Transform>(e).position = position;
    world_.AddComponent<Projectile>(e, Projectile{owner, dir, speed, life,
                                                  radius, damage});
    ++liveCount_;
    return e;
}

void ProjectilePool::Despawn(Entity e) {
    if (e.IsNull() || !world_.IsAlive(e)) return;
    if (!world_.HasComponent<Projectile>(e)) return;  // not a live projectile
    world_.RemoveComponent<Projectile>(e);
    world_.RemoveComponent<Transform>(e);
    free_.push_back(e);
    --liveCount_;
}

void ProjectilePool::DespawnAll() {
    // Collect first: removing components during view iteration is illegal.
    std::vector<Entity> live;
    for (auto [entity, projectile] : world_.ViewComponents<Projectile>()) {
        (void)projectile;
        live.push_back(entity);
    }
    for (const Entity e : live) Despawn(e);
}

}  // namespace af
