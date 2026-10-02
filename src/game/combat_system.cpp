#include "af/game/CombatSystem.h"

#include "af/collision/CollisionTests.h"
#include "af/core/Components.h"
#include "af/game/GameplayComponents.h"
#include "af/math/MathUtil.h"

namespace af {

CombatSystem::CombatSystem(ProjectilePool& pool) : pool_(pool) {}

void CombatSystem::Update(World& world, float dt) {
    (void)dt;

    stats_ = {};
    pendingHits_.clear();

    for (auto [pe, pTransform, projectile] :
         world.ViewComponents<Transform, Projectile>()) {
        for (auto [ee, eTransform, collider, enemy, health] :
             world.ViewComponents<Transform, Collider, Enemy, Health>()) {
            if (enemy.state == EnemyState::Dead) continue;

            const Vec3 enemyCenter = eTransform.position + collider.offset;
            const float enemyRadius =
                collider.kind == ColliderKind::Sphere ? collider.radius : 1.0f;
            const Contact contact =
                SphereSphere({pTransform.position, projectile.radius},
                             {enemyCenter, enemyRadius});
            if (contact.hit) {
                ++stats_.hits;
                pendingHits_.push_back({pe, ee, projectile.damage});
                break;  // one enemy per projectile
            }
        }
    }

    // Apply after iteration: despawning removes components (structural).
    for (const PendingHit& hit : pendingHits_) {
        pool_.Despawn(hit.projectile);
        if (!world.IsAlive(hit.enemy)) continue;
        Health& health = world.GetComponent<Health>(hit.enemy);
        Enemy& enemy = world.GetComponent<Enemy>(hit.enemy);
        health.current -= hit.damage;
        if (health.current <= 0) {
            health.current = 0;
            enemy.state = EnemyState::Dead;
            ++stats_.kills;
        } else {
            enemy.state = EnemyState::Hurt;
            enemy.hurtTimer = enemy.hurtTime;
        }
    }
}

}  // namespace af
