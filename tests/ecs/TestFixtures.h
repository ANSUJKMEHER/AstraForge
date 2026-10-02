#pragma once
// Shared fixtures for ECS tests: a scratch component and tiny systems.

#include "af/core/Components.h"
#include "af/ecs/System.h"
#include "af/ecs/World.h"

namespace af_test {

// Scratch component used to verify signature/mask behavior.
struct Tag {
    int value = 0;
};

class SysSetTag : public af::System {
public:
    void Update(af::World& world, float) override {
        for (auto [entity, tag] : world.ViewComponents<Tag>()) {
            (void)entity;
            tag.value = 1;
        }
    }
};

class SysDoubleTag : public af::System {
public:
    void Update(af::World& world, float) override {
        for (auto [entity, tag] : world.ViewComponents<Tag>()) {
            (void)entity;
            tag.value *= 2;
        }
    }
};

// p += v * dt — the same integration order as the headless runner.
class SysMove : public af::System {
public:
    void Update(af::World& world, float dt) override {
        for (auto [entity, transform, velocity] :
             world.ViewComponents<af::Transform, af::Velocity>()) {
            (void)entity;
            transform.position += velocity.linear * dt;
        }
    }
};

}  // namespace af_test
