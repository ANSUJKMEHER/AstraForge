#include "af/game/PlayerControllerSystem.h"

#include "af/core/Components.h"
#include "af/math/MathUtil.h"
#include "af/physics/PhysicsComponents.h"

namespace af {

PlayerControllerSystem::PlayerControllerSystem(Entity player,
                                               ProjectilePool& pool,
                                               const PlayerInput* input,
                                               Vec2 arenaMin, Vec2 arenaMax)
    : player_(player),
      pool_(pool),
      input_(input),
      arenaMin_(arenaMin),
      arenaMax_(arenaMax) {}

void PlayerControllerSystem::Reset() {
    shootTimer_ = 0.0f;
    prevJump_ = false;
}

void PlayerControllerSystem::Update(World& world, float dt) {
    if (!world.IsAlive(player_)) return;

    Player& player = world.GetComponent<Player>(player_);
    Velocity& velocity = world.GetComponent<Velocity>(player_);
    Health& health = world.GetComponent<Health>(player_);

    shootTimer_ = Max(0.0f, shootTimer_ - dt);

    // Dead players ignore input and come to a stop (restart is handled by
    // the session after the systems run).
    if (health.current <= 0 || input_ == nullptr) {
        velocity.linear.x = 0.0f;
        velocity.linear.z = 0.0f;
        return;
    }

    // --- Movement: WASD strafe on the XZ plane. ---
    Vec2 move = input_->move;
    if (LengthSq(move) > 1.0f) move = Normalize(move);  // no diagonal speedup
    velocity.linear.x = move.x * player.moveSpeed;
    velocity.linear.z = move.y * player.moveSpeed;

    // --- Jump: edge-triggered; PhysicsSystem applies it only when grounded. ---
    if (input_->jump && !prevJump_) {
        world.AddComponent<JumpRequest>(player_, JumpRequest{player.jumpSpeed});
    }
    prevJump_ = input_->jump;

    // --- Shoot: rate-limited, pooled projectile in the aim direction. ---
    if (input_->shoot && shootTimer_ <= 0.0f) {
        const Vec3 aim = Normalize(input_->aim);
        const Transform& t = world.GetComponent<Transform>(player_);
        // Spawn in front of the player so the projectile doesn't start
        // inside the player's own collider.
        const Vec3 origin = t.position + aim * 0.8f;
        pool_.Spawn(origin, aim, 20.0f, 2.0f, 0.25f, player.projectileDamage,
                    player_);
        shootTimer_ = player.shootCooldown;
    }

    // --- Arena clamp (belt and braces; walls also handle this). ---
    Transform& t = world.GetComponent<Transform>(player_);
    t.position.x = Clamp(t.position.x, arenaMin_.x, arenaMax_.x);
    t.position.z = Clamp(t.position.z, arenaMin_.y, arenaMax_.y);
}

}  // namespace af
