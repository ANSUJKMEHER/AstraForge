#pragma once
// CollisionSystem — broadphase (uniform XZ grid) → narrowphase → positional
// separation. Runs at fixed dt like all systems.
//
// Two collider classes:
//   - Dynamic colliders go through the uniform-grid broadphase and are
//     separated symmetrically (each moves half the penetration).
//   - Static colliders (arena walls, added in the gameplay phase) are kept
//     out of the grid — their large extents would inflate query ranges and
//     degenerate the broadphase. Dynamic-vs-static pairs are tested directly
//     (few statics by design) and the FULL penetration is applied to the
//     dynamic collider only; statics never move. Static-vs-static pairs are
//     skipped (immovable geometry needs no response).
//
// Structural change rule: the system gathers state first, then resolves —
// it does not add/remove entities or components, so iteration stays valid.

#include <vector>

#include "af/collision/SpatialHashGrid.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"
#include "af/math/Vec2.h"

namespace af {

class CollisionSystem : public System {
public:
    struct Stats {
        std::size_t entries = 0;          // dynamic colliders submitted
        std::size_t candidatePairs = 0;   // broadphase candidates
        std::size_t contacts = 0;         // dynamic-dynamic intersections
        std::size_t staticContacts = 0;   // dynamic-vs-static intersections
    };

    CollisionSystem(Vec2 arenaMin, Vec2 arenaMax, float cellSize);

    void Update(World& world, float dt) override;

    const Stats& LastStats() const { return stats_; }

private:
    SpatialHashGrid grid_;
    Stats stats_;
    // Scratch buffers, reused across frames (zero steady-state allocations).
    std::vector<GridEntry> entries_;        // dynamic colliders
    struct StaticEntry {
        Entity entity;
        AABB bounds;
    };
    std::vector<StaticEntry> statics_;      // static colliders (walls)
    std::vector<Vec3> resolutions_;         // per dynamic entry: accumulated push
};

}  // namespace af
