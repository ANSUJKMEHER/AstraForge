#include "af/collision/SpatialHashGrid.h"

#include <algorithm>
#include <cmath>

namespace af {

SpatialHashGrid::SpatialHashGrid(Vec2 min, Vec2 max, float cellSize)
    : min_(min), cellSize_(cellSize) {
    const float spanX = max.x - min.x;
    const float spanY = max.y - min.y;
    cols_ = std::max(1, static_cast<int>(std::ceil(spanX / cellSize)));
    rows_ = std::max(1, static_cast<int>(std::ceil(spanY / cellSize)));
    heads_.resize(static_cast<std::size_t>(cols_) * rows_);
}

void SpatialHashGrid::Rebuild(const std::vector<GridEntry>& entries) {
    std::fill(heads_.begin(), heads_.end(), -1);
    next_.assign(entries.size(), -1);  // capacity retained across frames

    // Largest half-extent of this frame → query inflation radius.
    inflateX_ = 0.0f;
    inflateZ_ = 0.0f;
    for (const GridEntry& e : entries) {
        inflateX_ = std::max(inflateX_, (e.bounds.max.x - e.bounds.min.x) * 0.5f);
        inflateZ_ = std::max(inflateZ_, (e.bounds.max.z - e.bounds.min.z) * 0.5f);
    }

    // Anchor each entry in the single cell containing its bounds center.
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const AABB& b = entries[i].bounds;
        const float cx = (b.min.x + b.max.x) * 0.5f;
        const float cz = (b.min.z + b.max.z) * 0.5f;
        const std::size_t cell =
            static_cast<std::size_t>(ClampZ(CellZ(cz)) * cols_ + ClampX(CellX(cx)));
        next_[i] = heads_[cell];
        heads_[cell] = static_cast<int32_t>(i);
    }
}

}  // namespace af
