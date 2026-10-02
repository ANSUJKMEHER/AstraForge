#pragma once
// PlayerControllerSystem — translates PlayerInput into player motion.
//
// Per fixed step: WASD strafe (Velocity on the XZ plane), edge-triggered jump
// (JumpRequest, consumed by PhysicsSystem when grounded), and shooting
// (projectiles spawned from the pool, rate-limited by the Player component's
// shootCooldown). A dead player ignores input (velocity zeroed, no shots).
//
// Structural changes (jump requests, pool spawns) happen outside view
// iteration — the system reads input first, then applies.

#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/game/GameplayComponents.h"
#include "af/game/ProjectilePool.h"
#include "af/math/Vec2.h"

namespace af {

class PlayerControllerSystem : public System {
public:
    PlayerControllerSystem(Entity player, ProjectilePool& pool,
                           const PlayerInput* input, Vec2 arenaMin, Vec2 arenaMax);

    void Update(World& world, float dt) override;

    // Restart support: clears edge-trigger and cooldown state.
    void Reset();

    // Re-points the system at a player entity (the session builds the world
    // after systems are constructed, so the target is set in two steps).
    void Retarget(Entity player) { player_ = player; }

private:
    Entity player_;
    ProjectilePool& pool_;
    const PlayerInput* input_;
    Vec2 arenaMin_;
    Vec2 arenaMax_;
    float shootTimer_ = 0.0f;
    bool prevJump_ = false;
};

}  // namespace af
