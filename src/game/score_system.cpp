#include "af/game/ScoreSystem.h"

#include "af/game/GameplayComponents.h"

namespace af {

ScoreSystem::ScoreSystem(Entity gameState) : gameState_(gameState) {}

void ScoreSystem::Update(World& world, float dt) {
    (void)dt;
    if (!world.IsAlive(gameState_)) return;

    dead_.clear();
    for (auto [entity, enemy] : world.ViewComponents<Enemy>()) {
        if (enemy.state == EnemyState::Dead) dead_.push_back(entity);
    }

    Score& score = world.GetComponent<Score>(gameState_);
    for (const Entity e : dead_) {
        if (!world.IsAlive(e)) continue;
        score.points += world.GetComponent<Enemy>(e).scoreValue;
        ++score.kills;
        world.DestroyEntity(e);
    }
}

}  // namespace af
