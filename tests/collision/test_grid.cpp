#include <vector>

#include "af/collision/SpatialHashGrid.h"
#include "framework/AllocationProbe.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

Entity MakeEntity(uint32_t id) { return Entity{id, 1}; }

}  // namespace

AF_TEST("grid dimensions cover the arena") {
    SpatialHashGrid grid({0.0f, 0.0f}, {40.0f, 40.0f}, 2.0f);
    AF_CHECK_EQ(grid.Columns(), 20);
    AF_CHECK_EQ(grid.Rows(), 20);
    AF_CHECK_EQ(grid.CellCount(), 400u);
}

AF_TEST("grid finds neighbors in adjacent cells") {
    SpatialHashGrid grid({0.0f, 0.0f}, {40.0f, 40.0f}, 2.0f);
    std::vector<GridEntry> entries;
    // Two entities straddling a cell boundary — must become candidates.
    entries.push_back({MakeEntity(0), MakeAABB({0.99f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f})});
    entries.push_back({MakeEntity(1), MakeAABB({1.01f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f})});
    grid.Rebuild(entries);

    std::vector<uint32_t> found;
    grid.QueryNeighbors(entries[0].bounds, [&](int32_t j) { found.push_back(j); });
    // Entity 0 must find entity 1 (and itself, expected in a grid query).
    bool foundOne = false;
    bool foundSelf = false;
    for (const uint32_t j : found) {
        if (j == 1) foundOne = true;
        if (j == 0) foundSelf = true;
    }
    AF_CHECK(foundOne);
    AF_CHECK(foundSelf);
}

AF_TEST("grid does not report far-away entities") {
    SpatialHashGrid grid({0.0f, 0.0f}, {40.0f, 40.0f}, 2.0f);
    std::vector<GridEntry> entries;
    entries.push_back({MakeEntity(0), MakeAABB({0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f})});
    entries.push_back({MakeEntity(1), MakeAABB({30.0f, 0.0f, 30.0f}, {0.5f, 0.5f, 0.5f})});
    grid.Rebuild(entries);

    bool foundFar = false;
    grid.QueryNeighbors(entries[0].bounds, [&](int32_t j) {
        if (j == 1) foundFar = true;
    });
    AF_CHECK(!foundFar);
}

AF_TEST("grid query is a superset of true overlaps") {
    // Property test with a deterministic placement: every pair of overlapping
    // AABBs must appear in the candidate set (candidates may include extras —
    // the narrowphase filters those).
    SpatialHashGrid grid({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    std::vector<GridEntry> entries;
    uint32_t seed = 12345u;
    auto next = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>((seed >> 8) & 0xFFFF) / 65536.0f * 40.0f - 20.0f;
    };
    for (uint32_t i = 0; i < 300; ++i) {
        const Vec3 c{next(), 0.0f, next()};
        entries.push_back({MakeEntity(i), MakeAABB(c, {0.5f, 0.5f, 0.5f})});
    }
    grid.Rebuild(entries);

    // Collect the candidate set.
    std::vector<uint8_t> candidate(300 * 300 / 8 + 1, 0);
    auto mark = [&](uint32_t a, uint32_t b) {
        const uint32_t idx = a * 300u + b;
        candidate[idx / 8u] |= static_cast<uint8_t>(1u << (idx % 8u));
    };
    for (std::size_t i = 0; i < entries.size(); ++i) {
        grid.QueryNeighbors(entries[i].bounds, [&](int32_t j) {
            if (static_cast<std::size_t>(j) > i) mark(static_cast<uint32_t>(i), static_cast<uint32_t>(j));
        });
    }

    // Every actually-overlapping pair must be in the candidate set.
    int overlaps = 0;
    for (uint32_t i = 0; i < 300; ++i) {
        for (uint32_t j = i + 1; j < 300; ++j) {
            const AABB& a = entries[i].bounds;
            const AABB& b = entries[j].bounds;
            const bool hit = a.min.x < b.max.x && a.max.x > b.min.x &&
                             a.min.y < b.max.y && a.max.y > b.min.y &&
                             a.min.z < b.max.z && a.max.z > b.min.z;
            if (!hit) continue;
            ++overlaps;
            const uint32_t idx = i * 300u + j;
            AF_CHECK((candidate[idx / 8u] >> (idx % 8u)) & 1u);
        }
    }
    AF_CHECK(overlaps > 0);  // the placement actually overlaps
}

AF_TEST("grid rebuild allocates zero bytes in steady state") {
    SpatialHashGrid grid({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    std::vector<GridEntry> entries;
    uint32_t seed = 99u;
    auto next = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>((seed >> 8) & 0xFFFF) / 65536.0f * 40.0f - 20.0f;
    };
    for (uint32_t i = 0; i < 1000; ++i) {
        entries.push_back({MakeEntity(i), MakeAABB({next(), 0.0f, next()}, {0.5f, 0.5f, 0.5f})});
    }
    grid.Rebuild(entries);  // warm up: all buffers reach their peak size

    const long long before = af::test::AllocationCount();
    for (int frame = 0; frame < 200; ++frame) {
        grid.Rebuild(entries);
        std::size_t total = 0;
        for (const GridEntry& e : entries) {
            grid.QueryNeighbors(e.bounds, [&](int32_t) { ++total; });
        }
        AF_CHECK(total > 0);
    }
    const long long after = af::test::AllocationCount();
    AF_CHECK_EQ(after, before);
}
