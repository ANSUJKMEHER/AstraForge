#pragma once
// Sparse set — the component storage unit of the ECS.
//
// Layout: a dense array of components (contiguous, iteration is
// cache-friendly) + a sparse map from entity id to dense index (O(1) lookup).
// A parallel dense array stores the full entity handle for each component.
//
// Properties:
//   - add/get/has: O(1)
//   - remove: O(1) swap-remove — the last element moves into the hole, so
//     dense ORDER is not stable. Systems must not rely on iteration order
//     beyond determinism-for-identical-construction-history.
//   - capacity is retained after removals → steady-state zero allocations
//     (verified by tests/ecs/test_allocation.cpp).
//
// Component requirement: move-assignable (swap-remove).

#include <cstdint>
#include <utility>
#include <vector>

#include "af/ecs/Entity.h"

namespace af {

// Type-erased interface so World can hold heterogeneous sets and destroy
// entities without knowing component types.
class SparseSetBase {
public:
    virtual ~SparseSetBase() = default;
    virtual void RemoveEntity(EntityId id) = 0;
    virtual bool HasEntity(EntityId id) const = 0;
    virtual std::size_t Size() const = 0;
    virtual Entity EntityAt(std::size_t index) const = 0;
};

template <typename T>
class SparseSet : public SparseSetBase {
public:
    // Adds the component, or overwrites the existing one; returns a reference.
    T& Add(Entity entity, T component) {
        const EntityId id = entity.id;
        if (Has(id)) {
            dense_[sparse_[id] - 1] = std::move(component);
            return dense_[sparse_[id] - 1];
        }
        EnsureSparse(id);
        sparse_[id] = static_cast<uint32_t>(dense_.size()) + 1;
        entities_.push_back(entity);
        dense_.push_back(std::move(component));
        return dense_.back();
    }

    void Remove(EntityId id) {
        if (!Has(id)) return;
        const uint32_t index = sparse_[id] - 1;
        const uint32_t last = static_cast<uint32_t>(dense_.size()) - 1;
        if (index != last) {
            dense_[index] = std::move(dense_[last]);
            entities_[index] = entities_[last];
            sparse_[entities_[index].id] = index + 1;
        }
        dense_.pop_back();
        entities_.pop_back();
        sparse_[id] = 0;
    }

    bool Has(EntityId id) const { return id < sparse_.size() && sparse_[id] != 0; }

    T& Get(EntityId id) { return dense_[sparse_[id] - 1]; }
    const T& Get(EntityId id) const { return dense_[sparse_[id] - 1]; }

    const std::vector<T>& Dense() const { return dense_; }
    const std::vector<Entity>& Entities() const { return entities_; }

    std::size_t Size() const override { return dense_.size(); }
    void RemoveEntity(EntityId id) override { Remove(id); }
    bool HasEntity(EntityId id) const override { return Has(id); }
    Entity EntityAt(std::size_t index) const override { return entities_[index]; }

    void Clear() {
        dense_.clear();
        entities_.clear();
        std::fill(sparse_.begin(), sparse_.end(), 0);
    }

private:
    void EnsureSparse(EntityId id) {
        if (id >= sparse_.size()) sparse_.resize(id + 1, 0);
    }

    std::vector<T> dense_;
    std::vector<Entity> entities_;   // dense-order entity handles
    std::vector<uint32_t> sparse_;   // entity id -> dense index + 1 (0 = absent)
};

}  // namespace af
