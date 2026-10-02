#pragma once
// EnemyAISystem — the enemy finite state machine.
//
//   Idle   → Chase   (player within detectRange)
//   Chase  → Attack  (player within attackRange; velocity zeroed)
//   Attack → Chase   (player left attackRange)
//   Hurt   → Chase   (hurtTimer elapsed)
//   *      → Dead    (health ≤ 0; set by CombatSystem)
//
// Chase sets Velocity toward the player on the XZ plane; Attack deals melee
// damage on the attackTimer cadence; Hurt freezes the enemy briefly. Dead
// enemies are inert (ScoreSystem destroys them and awards points).
//
// Structural changes: none (melee damage mutates components only; dead
// entities are destroyed by ScoreSystem, not here).

#include <vector>

#include "af/ecs/System.h"
#include "af/ecs/World.h"

namespace af {

class EnemyAISystem : public System {
public:
    explicit EnemyAISystem(Entity player);

    void Update(World& world, float dt) override;

private:
    Entity player_;
    // Scratch: enemies whose attack timer elapsed this frame (melee damage
    // applied after iteration — the player may be in the iterated view if the
    // player also carries Enemy, which it doesn't, but deferring keeps the
    // invariant local).
    struct PendingHit {
        Entity enemy;
        int damage = 0;
    };
    std::vector<PendingHit> pendingHits_;
};

}  // namespace af
