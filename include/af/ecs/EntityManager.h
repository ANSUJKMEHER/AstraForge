#pragma once
// EntityManager owns entity slots: a generation counter per id plus a LIFO
// free list of recycled slots.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "af/ecs/Entity.h"

namespace af {

class EntityManager {
public:
    // Returns a live handle. New ids are allocated sequentially; destroyed ids
    // are recycled first (LIFO free list — recently destroyed slots are reused
    // while still cache-warm).
    Entity Create();

    // Idempotent: no-op on dead/stale handles.
    void Destroy(Entity e);

    bool IsAlive(Entity e) const;

    std::size_t AliveCount() const { return aliveCount_; }
    std::size_t Capacity() const { return generations_.size(); }

private:
    std::vector<uint32_t> generations_;  // per slot; 0 = never issued
    std::vector<uint32_t> freeList_;
    std::size_t aliveCount_ = 0;
};

}  // namespace af
