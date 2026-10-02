#pragma once
// Systems and the SystemManager.
//
// A System owns one slice of simulation logic. Systems run in registration
// order — explicit, visible dependencies instead of a scheduler. Single-
// threaded by design; Phase 10 re-examines that decision with measurements.

#include <memory>
#include <utility>
#include <vector>

#include "af/ecs/World.h"

namespace af {

class System {
public:
    virtual ~System() = default;
    virtual void Update(World& world, float dt) = 0;
};

class SystemManager {
public:
    template <typename S, typename... Args>
    S& AddSystem(Args&&... args) {
        auto sys = std::make_unique<S>(std::forward<Args>(args)...);
        S& ref = *sys;
        systems_.push_back(std::move(sys));
        return ref;
    }

    void UpdateAll(World& world, float dt) {
        for (auto& system : systems_) system->Update(world, dt);
    }

    void Clear() { systems_.clear(); }
    std::size_t Size() const { return systems_.size(); }

private:
    std::vector<std::unique_ptr<System>> systems_;
};

}  // namespace af
