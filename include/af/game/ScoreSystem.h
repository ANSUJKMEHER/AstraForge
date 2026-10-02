#pragma once
// ScoreSystem — resolves dead enemies: awards points, counts kills, and
// destroys the entities (deferred — destruction during iteration is a
// structural change). Score is a component on the game-state entity.

#include <vector>

#include "af/ecs/System.h"
#include "af/ecs/World.h"

namespace af {

class ScoreSystem : public System {
public:
    explicit ScoreSystem(Entity gameState);

    void Update(World& world, float dt) override;

private:
    Entity gameState_;
    std::vector<Entity> dead_;  // scratch, reused across frames
};

}  // namespace af
