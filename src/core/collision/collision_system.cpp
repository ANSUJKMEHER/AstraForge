#include "af/collision/CollisionSystem.h"

#include "af/collision/CollisionTests.h"
#include "af/collision/Shapes.h"
#include "af/core/Components.h"

namespace af {

namespace {

// World-space bounds of a collider (rotation/scale ignored by design).
AABB WorldBounds(const Transform& t, const Collider& c) {
    const Vec3 center = t.position + c.offset;
    if (c.kind == ColliderKind::Sphere) {
        return MakeAABB(center, Vec3{c.radius, c.radius, c.radius});
    }
    return MakeAABB(center, c.halfExtents);
}

// Reconstructs the narrowphase shape for a collider at a given position.
// (Colliders store one shape; dispatch picks the right test overload.)
Contact TestAgainstStatic(const Collider& dynamic, const Vec3& center,
                          const AABB& staticBox) {
    if (dynamic.kind == ColliderKind::Sphere) {
        return SphereAABB(Sphere{center, dynamic.radius}, staticBox);
    }
    return AABBAABB(MakeAABB(center, dynamic.halfExtents), staticBox);
}

// Push vector for the dynamic collider against a static AABB, honoring the
// narrowphase contact convention (CollisionTests.h): the normal points from
// the dynamic shape toward the box, so the dynamic shape moves
// -normal*depth — EXCEPT when a sphere's center is inside the box, where the
// normal already points OUT through the nearest face and the sphere must
// move +normal*depth. depth >= radius distinguishes the inside case (the
// outside case saturates at depth < radius).
Vec3 StaticPush(const Collider& dynamic, const Vec3& center,
                const AABB& staticBox) {
    const Contact contact = TestAgainstStatic(dynamic, center, staticBox);
    if (!contact.hit) return Vec3{0.0f, 0.0f, 0.0f};
    if (dynamic.kind == ColliderKind::Sphere &&
        contact.depth >= dynamic.radius) {
        return contact.normal * contact.depth;  // inside: normal is the exit
    }
    return -contact.normal * contact.depth;
}

// Symmetric-resolution variant for dynamic pairs. Same inside-box exception:
// when a's sphere center is inside b's box, only a moves (full depth, along
// the exit normal).
void PairPushes(const Collider& a, const Contact& c, Vec3& pushA, Vec3& pushB) {
    if (a.kind == ColliderKind::Sphere && c.depth >= a.radius) {
        pushA = c.normal * c.depth;
        pushB = Vec3{0.0f, 0.0f, 0.0f};
    } else {
        const Vec3 push = c.normal * (c.depth * 0.5f);
        pushA = -push;
        pushB = push;
    }
}

}  // namespace

CollisionSystem::CollisionSystem(Vec2 arenaMin, Vec2 arenaMax, float cellSize)
    : grid_(arenaMin, arenaMax, cellSize) {}

void CollisionSystem::Update(World& world, float dt) {
    (void)dt;  // positional separation only; velocities are the physics system's job

    // 1. Gather colliders, splitting static (walls) from dynamic.
    entries_.clear();
    statics_.clear();
    for (auto [entity, transform, collider] :
         world.ViewComponents<Transform, Collider>()) {
        const AABB bounds = WorldBounds(transform, collider);
        if (collider.isStatic) {
            statics_.push_back(StaticEntry{entity, bounds});
        } else {
            entries_.push_back(GridEntry{entity, bounds});
        }
    }
    const std::size_t n = entries_.size();
    stats_ = {};
    stats_.entries = n;

    // 2. Broadphase rebuild + candidate generation (dynamic colliders only).
    grid_.Rebuild(entries_);

    // 3. Dynamic-dynamic narrowphase + symmetric response. Accumulate pushes,
    //    then apply (half-and-half) so results don't depend on pair order.
    resolutions_.assign(n, Vec3{0.0f, 0.0f, 0.0f});
    for (std::size_t i = 0; i < n; ++i) {
        grid_.QueryNeighbors(entries_[i].bounds, [&](int32_t j) {
            if (static_cast<std::size_t>(j) <= i) return;  // dedup unordered pairs
            ++stats_.candidatePairs;

            const Entity ea = entries_[i].entity;
            const Entity eb = entries_[j].entity;
            const Transform& ta = world.GetComponent<Transform>(ea);
            const Transform& tb = world.GetComponent<Transform>(eb);
            const Collider& ca = world.GetComponent<Collider>(ea);
            const Collider& cb = world.GetComponent<Collider>(eb);

            const Vec3 centerA = ta.position + ca.offset;
            const Vec3 centerB = tb.position + cb.offset;
            Contact contact{};
            if (ca.kind == ColliderKind::Sphere && cb.kind == ColliderKind::Sphere) {
                contact = SphereSphere({centerA, ca.radius}, {centerB, cb.radius});
            } else if (ca.kind == ColliderKind::Sphere) {
                contact = SphereAABB({centerA, ca.radius},
                                     MakeAABB(centerB, cb.halfExtents));
            } else if (cb.kind == ColliderKind::Sphere) {
                contact = TestColliders(MakeAABB(centerA, ca.halfExtents),
                                        Sphere{centerB, cb.radius});
            } else {
                contact = AABBAABB(MakeAABB(centerA, ca.halfExtents),
                                   MakeAABB(centerB, cb.halfExtents));
            }

            if (contact.hit) {
                ++stats_.contacts;
                PairPushes(ca, contact, resolutions_[i], resolutions_[j]);
            }
        });
    }

    // 4. Dynamic-vs-static: push the dynamic collider out by the FULL depth.
    //    Statics are few (arena walls), so a direct loop is cheap and keeps
    //    the grid inflation tight (see header).
    for (std::size_t i = 0; i < n; ++i) {
        const Entity e = entries_[i].entity;
        const Collider& cd = world.GetComponent<Collider>(e);
        const Vec3 center = world.GetComponent<Transform>(e).position + cd.offset;
        for (const StaticEntry& s : statics_) {
            const Contact contact = TestAgainstStatic(cd, center, s.bounds);
            if (contact.hit) {
                ++stats_.staticContacts;
                resolutions_[i] += StaticPush(cd, center, s.bounds);
            }
        }
    }

    // 5. Apply accumulated separations.
    for (std::size_t i = 0; i < n; ++i) {
        world.GetComponent<Transform>(entries_[i].entity).position += resolutions_[i];
    }
}

}  // namespace af
