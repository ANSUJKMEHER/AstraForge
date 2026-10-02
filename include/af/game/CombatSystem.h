#pragma once
// CombatSystem — projectile-vs-enemy hits.
//
// For each live projectile, narrowphase Sphere↔Sphere against every enemy
// collider (arena scale is small — tens of enemies, a bounded projectile
// pool — so O(P·E) is fine; the grid broadphase handles the larger body
// population in CollisionSystem). A hit:
//   - damages the enemy (Health.current -= projectile.damage)
//   - kills it (state = Dead; ScoreSystem destroys + scores) or sends it to
//     Hurt with the hurt timer set
//   - consumes the projectile (returned to the pool)
//
// The first enemy hit per projectile wins (projectiles are single-hit).
// Hits are collected during iteration and applied after (despawning removes
// components — a structural change).

#include <vector>

#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/game/ProjectilePool.h"

namespace af {

class CombatSystem : public System {
public:
    explicit CombatSystem(ProjectilePool& pool);

    void Update(World& world, float dt) override;

    struct Stats {
        std::size_t hits = 0;
        std::size_t kills = 0;
    };
    const Stats& LastStats() const { return stats_; }

private:
    ProjectilePool& pool_;
    Stats stats_;
    struct PendingHit {
        Entity projectile;
        Entity enemy;
        int damage = 0;
    };
    std::vector<PendingHit> pendingHits_;  // scratch, reused across frames
};

}  // namespace af
