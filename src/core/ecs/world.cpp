#include "af/ecs/World.h"

#include <utility>

namespace af {

void World::DestroyEntity(Entity e) {
    if (!entities_.IsAlive(e)) return;  // idempotent
    const ComponentMask mask = GetMask(e);
    const ComponentId setCount = static_cast<ComponentId>(sets_.size());
    for (ComponentId cid = 0; cid < setCount; ++cid) {
        if ((mask & (ComponentMask{1} << cid)) && sets_[cid]) {
            sets_[cid]->RemoveEntity(e.id);
        }
    }
    if (e.id < masks_.size()) masks_[e.id] = 0;
    entities_.Destroy(e);
}

ComponentMask World::GetMask(Entity e) const {
    if (!entities_.IsAlive(e)) return 0;
    if (e.id >= masks_.size()) return 0;
    return masks_[e.id];
}

void World::CheckAlive(Entity e) const {
    AF_ASSERT(entities_.IsAlive(e), "stale entity handle used");
}

}  // namespace af
