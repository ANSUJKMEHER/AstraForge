#pragma once
// Gameplay components for the arena game (Phase 8).
//
// These are plain value types stored densely by the ECS, like the core
// components in af/core/Components.h. The game-state entity carries Score;
// the spawner entity carries EnemySpawner. Enemy holds its own FSM state —
// the AI system (EnemyAISystem) advances it at fixed dt.

#include <cstdint>

#include "af/ecs/Entity.h"
#include "af/math/Vec2.h"
#include "af/math/Vec3.h"

namespace af {

// Marks the player entity (single-player arena).
struct Player {
    float moveSpeed = 6.0f;
    float jumpSpeed = 8.0f;
    float shootCooldown = 0.25f;  // seconds between shots
    int projectileDamage = 1;
};

// Enemy FSM: Idle → Chase → Attack → (Hurt → Chase | Dead).
enum class EnemyState : uint8_t { Idle, Chase, Attack, Hurt, Dead };

struct Enemy {
    EnemyState state = EnemyState::Idle;
    float speed = 3.0f;
    float detectRange = 12.0f;     // Idle → Chase when player within range
    float attackRange = 1.8f;      // Chase → Attack when within range
    float attackCooldown = 1.0f;   // seconds between melee hits
    float attackTimer = 0.0f;      // counts down in Attack state
    float hurtTime = 0.25f;        // seconds spent in Hurt after being hit
    float hurtTimer = 0.0f;
    int damage = 1;                // melee damage per hit
    int scoreValue = 100;          // awarded when this enemy dies
};

struct Health {
    int current = 1;
    int max = 1;
};

// Pooled projectile (see ProjectilePool). Moves at constant velocity; no
// gravity. CombatSystem resolves hits; ProjectileSystem moves and expires.
struct Projectile {
    Entity owner = Entity::Null();
    Vec3 dir{0.0f, 0.0f, -1.0f};
    float speed = 20.0f;
    float life = 2.0f;     // remaining seconds
    float radius = 0.25f;  // narrowphase size
    int damage = 1;
};

// On the game-state entity: session-level score bookkeeping.
struct Score {
    int points = 0;
    int kills = 0;
};

// On the spawner entity: spawns enemies near the arena edge on a timer.
// rngState is a deterministic LCG so headless runs are bit-reproducible.
struct EnemySpawner {
    float interval = 3.0f;
    float timer = 0.0f;
    int maxAlive = 6;
    uint32_t rngState = 12345u;
};

// Stationary defense turret (Tower / Plant). Auto-targets closest enemy and shoots.
struct Turret {
    float range = 14.0f;
    float fireInterval = 0.5f;
    float fireTimer = 0.0f;
    int damage = 1;
};

// Health pickup in arena. Restores player health on contact.
struct HealthPickup {
    int healAmount = 2;
    float radius = 0.8f;
};

// Headless-friendly player input. The SDL2 app maps its InputState into this
// struct every fixed step; headless runs and tests script it directly.
struct PlayerInput {
    Vec2 move{0.0f, 0.0f};  // XZ plane: x = strafe, y = forward (WASD)
    bool jump = false;
    bool shoot = false;
    Vec3 aim{0.0f, 0.0f, -1.0f};  // world-space shoot direction
};

}  // namespace af

