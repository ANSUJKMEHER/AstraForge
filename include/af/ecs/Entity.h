#pragma once
// Entity — a handle to an entity: (id, generation).
//
// The generation makes stale handles detectable after a slot is reused:
// destroying an entity bumps its generation, so any handle kept from before
// the destroy fails validation even if the id is later handed out again.
// This is what makes object pooling safe (see Phase 8).
//
// Handles are 8 bytes and copyable — use them by value everywhere.

#include <cstdint>
#include <limits>
#include <tuple>

namespace af {

using EntityId = uint32_t;

struct Entity {
    static constexpr EntityId InvalidId = std::numeric_limits<EntityId>::max();

    EntityId id = InvalidId;
    uint32_t generation = 0;

    constexpr bool IsNull() const { return id == InvalidId; }
    static constexpr Entity Null() { return {}; }

    bool operator==(const Entity&) const = default;
    bool operator!=(const Entity&) const = default;

    // Total order (for sorted containers, not used by the hot path).
    bool operator<(const Entity& o) const {
        return std::tie(id, generation) < std::tie(o.id, o.generation);
    }
};

}  // namespace af
