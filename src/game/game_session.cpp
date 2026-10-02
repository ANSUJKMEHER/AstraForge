#include "af/game/GameSession.h"

#include "af/collision/CollisionSystem.h"
#include "af/core/Components.h"
#include "af/core/Profiling.h"
#include "af/game/EnemyAISystem.h"
#include "af/game/ProjectileSystem.h"
#include "af/game/ScoreSystem.h"
#include "af/game/SpawnerSystem.h"
#include "af/physics/PhysicsComponents.h"
#include "af/physics/PhysicsSystem.h"

namespace af {

GameSession::GameSession(const Config& config)
    : config_(config), pool_(world_, config.projectilePoolSize) {
    BuildWorld();
}

GameSession::GameSession() : GameSession(Config{}) {}

void GameSession::BuildWorld() {
    // --- Arena walls: static AABB colliders (CollisionSystem pushes dynamic
    // bodies out of them; statics never move). ---
    const float halfX = (config_.arenaMax.x - config_.arenaMin.x) * 0.5f;
    const float halfZ = (config_.arenaMax.y - config_.arenaMin.y) * 0.5f;
    const float wallT = 1.0f;       // wall thickness
    const float wallH = 3.0f;       // wall half-height
    const auto AddWall = [&](Vec3 center, Vec3 halfExtents) {
        const Entity wall = world_.CreateEntity();
        world_.AddComponent<Transform>(wall).position = center;
        world_.AddComponent<Collider>(
            wall, Collider{ColliderKind::AABB, {}, 0.0f, halfExtents, true});
    };
    AddWall({0.0f, wallH, config_.arenaMin.y - wallT},
            {halfX + wallT, wallH, wallT});  // -Z
    AddWall({0.0f, wallH, config_.arenaMax.y + wallT},
            {halfX + wallT, wallH, wallT});  // +Z
    AddWall({config_.arenaMin.x - wallT, wallH, 0.0f},
            {wallT, wallH, halfZ + wallT});  // -X
    AddWall({config_.arenaMax.x + wallT, wallH, 0.0f},
            {wallT, wallH, halfZ + wallT});  // +X

    // --- Player ---
    player_ = world_.CreateEntity();
    world_.AddComponent<Transform>(player_).position = Vec3{0.0f, 1.0f, 0.0f};
    world_.AddComponent<Velocity>(player_);
    world_.AddComponent<Gravity>(player_);
    world_.AddComponent<Collider>(player_, Collider{ColliderKind::Sphere, {},
                                                    0.5f, {}});
    world_.AddComponent<af::Player>(player_);
    world_.AddComponent<Health>(player_, Health{config_.playerHealth,
                                                config_.playerHealth});

    // --- Game state (score) + spawner ---
    gameState_ = world_.CreateEntity();
    world_.AddComponent<af::Score>(gameState_);

    spawner_ = world_.CreateEntity();
    world_.AddComponent<EnemySpawner>(
        spawner_, EnemySpawner{config_.spawnInterval, 0.0f, config_.maxEnemies,
                               config_.spawnSeed});

    // --- Systems in execution order. ---
    controller_ = &systems_.AddSystem<PlayerControllerSystem>(
        player_, pool_, &input_, config_.arenaMin, config_.arenaMax);
    enemyAI_ = &systems_.AddSystem<EnemyAISystem>(player_);
    spawnerSys_ = &systems_.AddSystem<SpawnerSystem>(spawner_, config_.arenaMin,
                                                  config_.arenaMax);
    physics_ = &systems_.AddSystem<PhysicsSystem>(0.0f);
    collision_ = &systems_.AddSystem<CollisionSystem>(
        config_.arenaMin, config_.arenaMax, 2.0f);
    projectileSys_ = &systems_.AddSystem<ProjectileSystem>(
        pool_, config_.arenaMin, config_.arenaMax);
    combat_ = &systems_.AddSystem<CombatSystem>(pool_);
    scoreSys_ = &systems_.AddSystem<ScoreSystem>(gameState_);

    dead_ = false;
    restartTimer_ = 0.0f;
}

void GameSession::Update(float dt) {
    systems_.UpdateAll(world_, dt);

    // Turret auto-targeting and firing (Defense Towers / Plants)
    for (auto [tEnt, turret, tTrans] : world_.ViewComponents<Turret, Transform>()) {
        turret.fireTimer -= dt;
        if (turret.fireTimer <= 0.0f) {
            float closestDistSq = turret.range * turret.range;
            Vec3 targetPos{0.0f, 0.0f, 0.0f};
            bool found = false;
            for (auto [eEnt, enemy, eTrans] : world_.ViewComponents<Enemy, Transform>()) {
                (void)eEnt;
                if (enemy.state != EnemyState::Dead) {
                    const Vec3 diff = eTrans.position - tTrans.position;
                    const float d2 = diff.x * diff.x + diff.z * diff.z;
                    if (d2 < closestDistSq) {
                        closestDistSq = d2;
                        targetPos = eTrans.position;
                        found = true;
                    }
                }
            }
            if (found) {
                turret.fireTimer = turret.fireInterval;
                Vec3 dir = targetPos - tTrans.position;
                dir.y = 0.0f;
                const float len = std::sqrt(dir.x * dir.x + dir.z * dir.z);
                if (len > 0.001f) {
                    dir = dir * (1.0f / len);
                    pool_.Spawn(tTrans.position + Vec3{0.0f, 0.5f, 0.0f} + dir * 0.8f,
                                dir, 22.0f, 2.0f, 0.25f, turret.damage, tEnt);
                }
            }
        }
    }

    // Health pickup collection
    if (PlayerAlive()) {
        const Vec3 pPos = world_.GetComponent<Transform>(player_).position;
        std::vector<Entity> consumedPickups;
        for (auto [pEnt, pickup, pTrans] : world_.ViewComponents<HealthPickup, Transform>()) {
            const Vec3 diff = pPos - pTrans.position;
            const float d2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            const float r = pickup.radius + 0.5f;
            if (d2 <= r * r) {
                auto* hp = world_.TryGetComponent<Health>(player_);
                if (hp) {
                    hp->current = std::min(hp->max, hp->current + pickup.healAmount);
                }
                consumedPickups.push_back(pEnt);
            }
        }
        for (Entity e : consumedPickups) {
            world_.DestroyEntity(e);
        }
    }

    // Death → restart countdown.
    if (!dead_ && !PlayerAlive()) {
        dead_ = true;
        restartTimer_ = config_.restartDelay;
    } else if (dead_) {
        restartTimer_ -= dt;
        if (restartTimer_ <= 0.0f) Restart();
    }
}

void GameSession::UpdateInstrumented(float dt) {
    { ScopeTimer t("player_controller"); controller_->Update(world_, dt); }
    { ScopeTimer t("enemy_ai"); enemyAI_->Update(world_, dt); }
    { ScopeTimer t("spawner"); spawnerSys_->Update(world_, dt); }
    { ScopeTimer t("physics"); physics_->Update(world_, dt); }
    { ScopeTimer t("collision"); collision_->Update(world_, dt); }
    { ScopeTimer t("projectiles"); projectileSys_->Update(world_, dt); }
    { ScopeTimer t("combat"); combat_->Update(world_, dt); }
    { ScopeTimer t("score"); scoreSys_->Update(world_, dt); }

    if (!dead_ && !PlayerAlive()) {
        dead_ = true;
        restartTimer_ = config_.restartDelay;
    } else if (dead_) {
        restartTimer_ -= dt;
        if (restartTimer_ <= 0.0f) Restart();
    }
}

void GameSession::Restart() { ResetWorld(); }

void GameSession::ResetWorld() {
    // 1. Destroy all enemies.
    std::vector<Entity> enemies;
    for (auto [entity, enemy] : world_.ViewComponents<Enemy>()) {
        (void)enemy;
        enemies.push_back(entity);
    }
    for (const Entity e : enemies) world_.DestroyEntity(e);

    // 2. Park all projectiles.
    pool_.DespawnAll();

    // 3. Reset the player.
    world_.GetComponent<Transform>(player_).position = Vec3{0.0f, 1.0f, 0.0f};
    world_.GetComponent<Velocity>(player_).linear = Vec3{0.0f, 0.0f, 0.0f};
    world_.GetComponent<Health>(player_).current =
        world_.GetComponent<Health>(player_).max;
    world_.RemoveComponent<JumpRequest>(player_);

    // 4. Reset score, spawner, controller state.
    world_.GetComponent<af::Score>(gameState_) = af::Score{};
    SpawnerSystem::Reset(world_, spawner_, config_.spawnSeed);
    controller_->Reset();

    dead_ = false;
    restartTimer_ = 0.0f;
}

bool GameSession::PlayerAlive() {
    const Health* h = world_.TryGetComponent<Health>(player_);
    return h != nullptr && h->current > 0;
}

int GameSession::Score() {
    return world_.GetComponent<af::Score>(gameState_).points;
}

int GameSession::Kills() {
    return world_.GetComponent<af::Score>(gameState_).kills;
}

std::size_t GameSession::EnemyCount() {
    std::size_t count = 0;
    for (auto [entity, enemy] : world_.ViewComponents<Enemy>()) {
        (void)entity;
        (void)enemy;
        ++count;
    }
    return count;
}

}  // namespace af
