#pragma once
// Physics components.
//
// Gravity: presence = entity is affected by gravity (per-entity strength, so
// different entities can have different gravity — default -9.81).
// JumpRequest: presence = "jump wanted this tick"; the physics system
// consumes it (removes the component) after applying the impulse.

namespace af {

struct Gravity {
    float strength = -9.81f;
};

struct JumpRequest {
    float speed = 0.0f;  // upward impulse speed (m/s)
};

}  // namespace af
