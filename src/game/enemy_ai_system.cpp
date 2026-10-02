#include "af/game/EnemyAISystem.h"

#include "af/core/Components.h"
#include "af/game/GameplayComponents.h"
#include "af/math/MathUtil.h"

namespace af {

EnemyAISystem::EnemyAISystem(Entity player) : player_(player) {}

void EnemyAISystem::Update(World& world, float dt) {
    if (!world.IsAlive(player_)) return;

    const Vec3 playerPos = world.GetComponent<Transform>(player_).position;
    Health* playerHealth = world.TryGetComponent<Health>(player_);
    const bool playerDead = playerHealth != nullptr && playerHealth->current <= 0;

    pendingHits_.clear();
    for (auto [entity, enemy, transform, velocity] :
         world.ViewComponents<Enemy, Transform, Velocity>()) {
        if (enemy.state == EnemyState::Dead) {
            velocity.linear.x = 0.0f;
            velocity.linear.z = 0.0f;
            continue;
        }

        const Vec3 delta = playerPos - transform.position;
        const float distXZ = Length(Vec3{delta.x, 0.0f, delta.z});

        switch (enemy.state) {
            case EnemyState::Idle:
                velocity.linear.x = 0.0f;
                velocity.linear.z = 0.0f;
                if (!playerDead && distXZ < enemy.detectRange) {
                    enemy.state = EnemyState::Chase;
                }
                break;

            case EnemyState::Chase: {
                if (playerDead) {
                    velocity.linear.x = 0.0f;
                    velocity.linear.z = 0.0f;
                    break;
                }
                const Vec3 dirXZ = Normalize(Vec3{delta.x, 0.0f, delta.z});
                velocity.linear.x = dirXZ.x * enemy.speed;
                velocity.linear.z = dirXZ.z * enemy.speed;
                if (distXZ < enemy.attackRange) {
                    enemy.state = EnemyState::Attack;
                    enemy.attackTimer = enemy.attackCooldown;  // windup
                }
                break;
            }

            case EnemyState::Attack:
                velocity.linear.x = 0.0f;
                velocity.linear.z = 0.0f;
                if (distXZ > enemy.attackRange) {
                    enemy.state = EnemyState::Chase;
                    break;
                }
                if (playerDead) break;
                enemy.attackTimer -= dt;
                if (enemy.attackTimer <= 0.0f) {
                    enemy.attackTimer = enemy.attackCooldown;
                    pendingHits_.push_back({entity, enemy.damage});
                }
                break;

            case EnemyState::Hurt:
                velocity.linear.x = 0.0f;
                velocity.linear.z = 0.0f;
                enemy.hurtTimer -= dt;
                if (enemy.hurtTimer <= 0.0f) {
                    enemy.state = EnemyState::Chase;
                }
                break;

            case EnemyState::Dead:
                break;
        }
    }

    // Apply melee damage after iteration (deferred pattern).
    if (playerHealth != nullptr) {
        for (const PendingHit& hit : pendingHits_) {
            if (world.IsAlive(hit.enemy) && playerHealth->current > 0) {
                playerHealth->current -= hit.damage;
            }
        }
    }
}

}  // namespace af
