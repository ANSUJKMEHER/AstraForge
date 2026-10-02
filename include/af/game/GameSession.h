#pragma once
// GameSession — the complete arena game, headless-testable.
//
// Owns the World, the ordered SystemManager, the projectile pool, and the
// scripted player input. One Update(dt) = one fixed 60 Hz simulation step:
//
//   PlayerController → EnemyAI → Spawner → Physics → Collision →
//   Projectiles → Combat → Score
//
// Death/restart: when the player's health hits zero, the session counts down
// restartDelay seconds, then Restart()s — score reset, enemies destroyed,
// projectiles returned to the pool, player respawned at the arena center.
// Restart() can also be called directly.
//
// Determinism: no hidden randomness — the only RNG is the spawner's LCG,
// seeded at construction and re-seeded on restart.

#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/game/CombatSystem.h"
#include "af/game/EnemyAISystem.h"
#include "af/game/GameplayComponents.h"
#include "af/game/PlayerControllerSystem.h"
#include "af/game/ProjectilePool.h"
#include "af/game/ProjectileSystem.h"
#include "af/game/ScoreSystem.h"
#include "af/game/SpawnerSystem.h"

namespace af {

class PhysicsSystem;
class CollisionSystem;
class ProjectileSystem;
class ScoreSystem;

class GameSession {
public:
    struct Config {
        Vec2 arenaMin{-20.0f, -20.0f};
        Vec2 arenaMax{20.0f, 20.0f};
        std::size_t projectilePoolSize = 64;
        float spawnInterval = 3.0f;
        int maxEnemies = 6;
        int playerHealth = 5;
        float restartDelay = 1.5f;
        uint32_t spawnSeed = 12345u;
    };

    explicit GameSession(const Config& config);
    GameSession();  // default config (delegates, defined out-of-line)

    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;

    // Scripted input for the next Update (replaced by the app each frame).
    void SetPlayerInput(const PlayerInput& input) { input_ = input; }
    const PlayerInput& PlayerInputState() const { return input_; }

    void Update(float dt);
    // Same as Update, but wraps each system stage in a profiler scope
    // (benchmark/profiling only — see src/bench/gameplay_bench.cpp).
    void UpdateInstrumented(float dt);
    void Restart();

    World& GetWorld() { return world_; }
    Entity Player() const { return player_; }
    Entity GameState() const { return gameState_; }
    Entity Spawner() const { return spawner_; }

    bool PlayerAlive();
    int Score();
    int Kills();
    std::size_t EnemyCount();
    std::size_t ProjectileCount() const { return pool_.LiveCount(); }
    ProjectilePool& GetPool() { return pool_; }
    float SecondsToRestart() const { return dead_ ? restartTimer_ : 0.0f; }

    const CombatSystem::Stats& CombatStats() const { return combat_->LastStats(); }

private:
    void BuildWorld();
    void ResetWorld();

    Config config_;
    World world_;
    SystemManager systems_;
    ProjectilePool pool_;
    PlayerInput input_{};
    Entity player_ = Entity::Null();
    Entity gameState_ = Entity::Null();
    Entity spawner_ = Entity::Null();
    PlayerControllerSystem* controller_ = nullptr;
    EnemyAISystem* enemyAI_ = nullptr;
    SpawnerSystem* spawnerSys_ = nullptr;
    PhysicsSystem* physics_ = nullptr;
    CollisionSystem* collision_ = nullptr;
    ProjectileSystem* projectileSys_ = nullptr;
    CombatSystem* combat_ = nullptr;
    ScoreSystem* scoreSys_ = nullptr;
    bool dead_ = false;
    float restartTimer_ = 0.0f;
};

}  // namespace af
