#pragma once
// Uniform grid on the XZ plane — the broadphase.
//
// Why a uniform grid (per context.md Decision 3): arena gameplay is mostly
// planar, the world is bounded, and object sizes are similar, so a grid gives
// O(1) neighbor queries with a tiny constant. A tree would only be justified
// by wildly varying sizes or unbounded space.
//
// Membership model (this matters): each entry is anchored in exactly ONE cell
// — the cell containing its bounds center. Queries enumerate the cells of the
// query bounds INFLATED by the frame's largest object extent. This guarantees
// the superset property without multi-cell list memberships:
//   if A and B overlap, B's center lies inside A's bounds expanded by B's
//   half-extent, so B's anchor cell is inside A's inflated query range.
// (An earlier single-next multi-cell insertion corrupted cell lists; this
// model is correct by construction AND does less work.)
//
// Y is ignored (planar hash). Zero steady-state allocations (verified in
// tests/collision/test_grid.cpp).

#include <cstddef>
#include <cstdint>
#include <vector>

#include "af/collision/Shapes.h"
#include "af/ecs/Entity.h"
#include "af/math/Vec2.h"

namespace af {

struct GridEntry {
    Entity entity;
    AABB bounds;  // world-space XZ-relevant bounds (Y ignored in the hash)
};

class SpatialHashGrid {
public:
    // Arena is [min, max] in XZ; cellSize in world units (choose ~2x the
    // largest collider diameter).
    SpatialHashGrid(Vec2 min, Vec2 max, float cellSize);

    // Rebuilds cell lists from entries. `entries` must stay alive and
    // unmodified until the next Rebuild (the grid stores a pointer).
    void Rebuild(const std::vector<GridEntry>& entries);

    // Invokes fn(entryIndex) for every entry whose anchor cell lies in the
    // inflated query range. Callers deduplicate pairs (j > i pattern).
    template <typename Fn>
    void QueryNeighbors(const AABB& bounds, Fn&& fn) const {
        const int cx0 = ClampX(CellX(bounds.min.x - inflateX_));
        const int cx1 = ClampX(CellX(bounds.max.x + inflateX_));
        const int cz0 = ClampZ(CellZ(bounds.min.z - inflateZ_));
        const int cz1 = ClampZ(CellZ(bounds.max.z + inflateZ_));
        for (int cz = cz0; cz <= cz1; ++cz) {
            for (int cx = cx0; cx <= cx1; ++cx) {
                int32_t head = heads_[static_cast<std::size_t>(cz * cols_ + cx)];
                while (head >= 0) {
                    fn(head);
                    head = next_[static_cast<std::size_t>(head)];
                }
            }
        }
    }

    int Columns() const { return cols_; }
    int Rows() const { return rows_; }
    std::size_t CellCount() const { return static_cast<std::size_t>(cols_) * rows_; }

private:
    int CellX(float x) const { return static_cast<int>((x - min_.x) / cellSize_); }
    int CellZ(float z) const { return static_cast<int>((z - min_.y) / cellSize_); }
    int ClampX(int c) const { return c < 0 ? 0 : (c >= cols_ ? cols_ - 1 : c); }
    int ClampZ(int c) const { return c < 0 ? 0 : (c >= rows_ ? rows_ - 1 : c); }

    Vec2 min_;
    float cellSize_;
    int cols_ = 0;
    int rows_ = 0;
    float inflateX_ = 0.0f;  // largest entry half-extent this frame
    float inflateZ_ = 0.0f;
    std::vector<int32_t> heads_;  // per cell: entry index or -1
    std::vector<int32_t> next_;   // per entry: next entry in its anchor cell
};

}  // namespace af
