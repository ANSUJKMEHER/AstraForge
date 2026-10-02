#pragma once
// World — the ECS container. Owns the entity manager, one sparse set per
// component type, and a per-entity component signature mask.
//
// Handle contract:
//   - GetComponent/HasComponent/AddComponent/RemoveComponent: assert the
//     entity is alive (stale handles are a programming error; AF_ASSERT is
//     always on — see af/core/Assert.h).
//   - TryGetComponent: defensive path, returns nullptr for dead entities or
//     missing components.
//   - DestroyEntity: idempotent (safe to call on dead handles).
//
// Not copyable/movable. Component references (e.g. from GetComponent) are
// invalidated by add/remove on the same component type (vector reallocation)
// and by entity destruction — do not stash them across those operations.

#include <memory>
#include <vector>

#include "af/core/Assert.h"
#include "af/ecs/Component.h"
#include "af/ecs/Entity.h"
#include "af/ecs/EntityManager.h"
#include "af/ecs/SparseSet.h"
#include "af/ecs/View.h"

namespace af {

class World {
public:
    World() = default;
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    Entity CreateEntity() { return entities_.Create(); }
    void DestroyEntity(Entity e);  // idempotent
    bool IsAlive(Entity e) const { return entities_.IsAlive(e); }
    std::size_t EntityCount() const { return entities_.AliveCount(); }

    template <typename T, typename... Args>
    T& AddComponent(Entity e, Args&&... args);

    template <typename T>
    void RemoveComponent(Entity e);

    template <typename T>
    bool HasComponent(Entity e) const;

    template <typename T>
    T& GetComponent(Entity e);

    template <typename T>
    const T& GetComponent(Entity e) const;

    template <typename T>
    T* TryGetComponent(Entity e);

    // Iterable view over entities having every listed component.
    template <typename... Ts>
    View<Ts...> ViewComponents();

    // Component signature of a live entity (0 for dead entities or entities
    // with no components yet).
    ComponentMask GetMask(Entity e) const;

private:
    template <typename T>
    SparseSet<T>& SetFor();  // creates the set if needed (add/view paths)

    template <typename T>
    SparseSet<T>* TrySetFor();  // no creation (lookup paths)

    template <typename T>
    const SparseSet<T>* ConstSetFor() const;

    void CheckAlive(Entity e) const;

    EntityManager entities_;
    std::vector<std::unique_ptr<SparseSetBase>> sets_;
    std::vector<ComponentMask> masks_;
};

template <typename T, typename... Args>
T& World::AddComponent(Entity e, Args&&... args) {
    CheckAlive(e);
    if (e.id >= masks_.size()) masks_.resize(e.id + 1, 0);
    T& ref = SetFor<T>().Add(e, T{std::forward<Args>(args)...});
    masks_[e.id] |= ComponentMaskOf<T>();
    return ref;
}

template <typename T>
void World::RemoveComponent(Entity e) {
    CheckAlive(e);
    if (SparseSet<T>* set = TrySetFor<T>()) set->Remove(e.id);
    if (e.id < masks_.size()) masks_[e.id] &= ~ComponentMaskOf<T>();
}

template <typename T>
bool World::HasComponent(Entity e) const {
    CheckAlive(e);
    const SparseSet<T>* set = ConstSetFor<T>();
    return set != nullptr && set->Has(e.id);
}

template <typename T>
T& World::GetComponent(Entity e) {
    CheckAlive(e);
    SparseSet<T>* set = TrySetFor<T>();
    AF_ASSERT(set != nullptr && set->Has(e.id),
              "entity does not have the requested component");
    return set->Get(e.id);
}

template <typename T>
const T& World::GetComponent(Entity e) const {
    CheckAlive(e);
    const SparseSet<T>* set = ConstSetFor<T>();
    AF_ASSERT(set != nullptr && set->Has(e.id),
              "entity does not have the requested component");
    return set->Get(e.id);
}

template <typename T>
T* World::TryGetComponent(Entity e) {
    if (!entities_.IsAlive(e)) return nullptr;  // defensive: no assert
    SparseSet<T>* set = TrySetFor<T>();
    if (set == nullptr || !set->Has(e.id)) return nullptr;
    return &set->Get(e.id);
}

template <typename... Ts>
View<Ts...> World::ViewComponents() {
    return View<Ts...>(&SetFor<Ts>()...);
}

template <typename T>
SparseSet<T>& World::SetFor() {
    const ComponentId id = ComponentIdOf<T>();
    if (id >= sets_.size()) sets_.resize(id + 1);
    if (!sets_[id]) sets_[id] = std::make_unique<SparseSet<T>>();
    return *static_cast<SparseSet<T>*>(sets_[id].get());
}

template <typename T>
SparseSet<T>* World::TrySetFor() {
    const ComponentId id = ComponentIdOf<T>();
    if (id >= sets_.size() || !sets_[id]) return nullptr;
    return static_cast<SparseSet<T>*>(sets_[id].get());
}

template <typename T>
const SparseSet<T>* World::ConstSetFor() const {
    const ComponentId id = ComponentIdOf<T>();
    if (id >= sets_.size() || !sets_[id]) return nullptr;
    return static_cast<const SparseSet<T>*>(sets_[id].get());
}

}  // namespace af
