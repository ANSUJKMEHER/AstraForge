#pragma once
// ProjectilePool — fixed-capacity pool of projectile entities.
//
// Why pool (context.md Phase 8): projectiles are created and destroyed at
// high frequency (shots, hits, expiry). A pool pre-creates `capacity` bare
// entities once; Spawn activates one (Transform + Projectile components),
// Despawn parks it again (components removed). No CreateEntity/DestroyEntity
// churn, no steady-state allocations (sparse-set capacity is retained), and
// entity handles never go stale mid-flight because parked entities are never
// destroyed.
//
// Contract:
//   - Spawn returns Entity::Null() when the pool is exhausted — callers drop
//     the shot (documented behavior, keeps the frame cost bounded).
//   - Despawn is idempotent and ignores entities that aren't live
//     projectiles.
//   - DespawnAll parks every live projectile (used by restart).

#include <cstddef>
#include <vector>

#include "af/ecs/Entity.h"
#include "af/ecs/World.h"
#include "af/math/Vec3.h"

namespace af {

class ProjectilePool {
public:
    ProjectilePool(World& world, std::size_t capacity);

    Entity Spawn(const Vec3& position, const Vec3& dir, float speed,
                 float life, float radius, int damage, Entity owner);
    void Despawn(Entity e);
    void DespawnAll();

    std::size_t LiveCount() const { return liveCount_; }
    std::size_t FreeCount() const { return free_.size(); }
    std::size_t Capacity() const { return free_.size() + liveCount_; }

private:
    World& world_;
    std::vector<Entity> free_;  // parked entities ready for reuse
    std::size_t liveCount_ = 0;
};

}  // namespace af
