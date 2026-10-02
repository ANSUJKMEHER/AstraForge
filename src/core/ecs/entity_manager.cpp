#include "af/ecs/EntityManager.h"

namespace af {

Entity EntityManager::Create() {
    uint32_t id;
    if (freeList_.empty()) {
        id = static_cast<uint32_t>(generations_.size());
        generations_.push_back(1);  // generation 1 is the first issued one
    } else {
        id = freeList_.back();
        freeList_.pop_back();
    }
    ++aliveCount_;
    return Entity{id, generations_[id]};
}

void EntityManager::Destroy(Entity e) {
    if (!IsAlive(e)) return;
    ++generations_[e.id];
    freeList_.push_back(e.id);
    --aliveCount_;
}

bool EntityManager::IsAlive(Entity e) const {
    // generation 0 is never issued, so a slot that was never created cannot
    // match a forged handle. (Documented limit: after 2^32 destroys of one
    // slot the generation wraps to 0 and that slot can never be reused.)
    return e.id < generations_.size() && e.generation != 0 &&
           generations_[e.id] == e.generation;
}

}  // namespace af
