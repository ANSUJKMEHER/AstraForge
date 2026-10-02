#pragma once
// View — iterable over all entities having every listed component.
//
// Usage:
//   for (auto [entity, transform, velocity] :
//        world.ViewComponents<Transform, Velocity>()) { ... }
//
// Iteration walks the SMALLEST of the requested sparse sets and filters by
// the rest, so cost is proportional to the rarest component's population,
// plus one O(1) sparse lookup per extra component per entity.
//
// The structured bindings for components are references (tuple elements are
// T&), so mutation through the view writes through to the world.
//
// Views are transient: making structural changes (add/remove components,
// create/destroy entities) DURING iteration invalidates them — component
// vectors may reallocate/swap-remove. Systems must defer spawn/despawn
// (standard pattern, used by the gameplay systems).

#include <algorithm>
#include <array>
#include <cstddef>
#include <tuple>

#include "af/ecs/Entity.h"
#include "af/ecs/SparseSet.h"

namespace af {

template <typename... Ts>
class View {
    static_assert(sizeof...(Ts) >= 1, "View needs at least one component type");

public:
    using SetsTuple = std::tuple<SparseSet<Ts>*...>;

    explicit View(SparseSet<Ts>*... sets) : sets_(sets...) {
        lead_ = SmallestOf(sets...);
    }

    class Iterator {
    public:
        Iterator() = default;
        Iterator(const View* view, std::size_t index) : view_(view), index_(index) {
            SkipMissing();
        }

        auto operator*() const {
            const Entity e = view_->EntityAt(index_);
            return std::tuple_cat(std::make_tuple(e), view_->RefsFor(e));
        }

        Iterator& operator++() {
            ++index_;
            SkipMissing();
            return *this;
        }

        bool operator==(const Iterator& o) const { return index_ == o.index_; }
        bool operator!=(const Iterator& o) const { return index_ != o.index_; }

    private:
        void SkipMissing() {
            while (index_ < view_->LeadSize() && !view_->AllPresent(index_)) {
                ++index_;
            }
        }

        const View* view_ = nullptr;
        std::size_t index_ = 0;
    };

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, LeadSize()); }

private:
    static SparseSetBase* SmallestOf(SparseSet<Ts>*... sets) {
        std::array<SparseSetBase*, sizeof...(Ts)> arr{
            static_cast<SparseSetBase*>(sets)...};
        return *std::min_element(arr.begin(), arr.end(),
                                 [](const SparseSetBase* a, const SparseSetBase* b) {
                                     return a->Size() < b->Size();
                                 });
    }

    std::size_t LeadSize() const { return lead_->Size(); }
    Entity EntityAt(std::size_t index) const { return lead_->EntityAt(index); }

    bool AllPresent(std::size_t index) const {
        const Entity e = EntityAt(index);
        bool all = true;
        std::apply([&](SparseSet<Ts>*... sets) {
            ((all = all && sets->Has(e.id)), ...);
        },
                   sets_);
        return all;
    }

    auto RefsFor(Entity e) const {
        return std::apply([&](SparseSet<Ts>*... sets) {
            return std::tie(sets->Get(e.id)...);
        },
                          sets_);
    }

    SetsTuple sets_;
    SparseSetBase* lead_ = nullptr;
};

}  // namespace af
