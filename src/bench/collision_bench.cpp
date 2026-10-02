// Collision benchmark: naive O(n^2) narrowphase vs uniform-grid broadphase.
// Reproducible: deterministic placement, fixed frame count, and a built-in
// correctness cross-check (contact counts must agree between the two paths).
//
// Usage: af_bench_collision [entity_count] [frames]

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "af/collision/CollisionTests.h"
#include "af/collision/Shapes.h"
#include "af/collision/SpatialHashGrid.h"
#include "af/ecs/Entity.h"

using namespace af;

namespace {

struct BenchEntry {
    AABB bounds;
    Sphere sphere;  // all bench entities are spheres (1 shape, fair compare)
};

float NextRand(uint32_t& seed) {
    seed = seed * 1664525u + 1013904223u;
    return static_cast<float>((seed >> 8) & 0xFFFFFF) / 16777216.0f - 0.5f;
}

std::vector<BenchEntry> MakePlacement(std::size_t count, uint32_t seed) {
    std::vector<BenchEntry> out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const Vec3 center{NextRand(seed) * 38.0f, 0.0f, NextRand(seed) * 38.0f};
        const float radius = 0.25f + NextRand(seed) * 0.0f + 0.5f;  // 0.75 fixed-ish
        const float r = 0.75f;
        out.push_back({MakeAABB(center, {r, r, r}), {center, r}});
    }
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    const std::size_t count = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 5000;
    const int frames = argc > 2 ? std::atoi(argv[2]) : 200;
    const auto entries = MakePlacement(count, 1234567u);

    std::size_t naiveContacts = 0;
    std::size_t naiveChecks = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int f = 0; f < frames; ++f) {
        std::size_t contacts = 0;
        for (std::size_t i = 0; i < count; ++i) {
            for (std::size_t j = i + 1; j < count; ++j) {
                ++naiveChecks;
                if (SphereSphere(entries[i].sphere, entries[j].sphere).hit) ++contacts;
            }
        }
        naiveContacts = contacts;  // final frame value
    }
    const auto t1 = std::chrono::steady_clock::now();

    SpatialHashGrid grid({-20.0f, -20.0f}, {20.0f, 20.0f}, 2.0f);
    std::vector<GridEntry> gridEntries;
    gridEntries.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        gridEntries.push_back({Entity{static_cast<uint32_t>(i), 1}, entries[i].bounds});
    }

    std::size_t gridContacts = 0;
    std::size_t gridCandidates = 0;
    const auto t2 = std::chrono::steady_clock::now();
    for (int f = 0; f < frames; ++f) {
        grid.Rebuild(gridEntries);
        std::size_t contacts = 0;
        std::size_t candidates = 0;
        for (std::size_t i = 0; i < count; ++i) {
            grid.QueryNeighbors(entries[i].bounds, [&](int32_t j) {
                if (static_cast<std::size_t>(j) <= i) return;
                ++candidates;
                if (SphereSphere(entries[i].sphere, entries[j].sphere).hit) ++contacts;
            });
        }
        gridContacts = contacts;
        gridCandidates = candidates;
    }
    const auto t3 = std::chrono::steady_clock::now();

    const double naiveMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double gridMs =
        std::chrono::duration<double, std::milli>(t3 - t2).count();

    std::printf("entities=%zu frames=%d\n", count, frames);
    std::printf("naive: %.3f ms total, %.6f ms/frame, checks=%zu, contacts=%zu\n",
                naiveMs, naiveMs / frames, naiveChecks, naiveContacts);
    std::printf("grid:  %.3f ms total, %.6f ms/frame, candidates=%zu, contacts=%zu\n",
                gridMs, gridMs / frames, gridCandidates, gridContacts);
    std::printf("speedup=%.2fx  candidate_reduction=%.3f%%\n",
                naiveMs / gridMs,
                100.0 * (1.0 - static_cast<double>(gridCandidates) /
                                  static_cast<double>(naiveChecks)));
    std::printf("cross_check=%s\n",
                naiveContacts == gridContacts ? "OK (contact counts match)"
                                              : "MISMATCH");
    return 0;
}
