#pragma once
// Position + rotation (unit quaternion) + scale.
// Local-to-world matrix composition is T * R * S: scale first, then rotate,
// then translate.

#include "Mat4.h"
#include "Quat.h"
#include "Vec3.h"

namespace af {

struct Transform {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Quat rotation = Quat::Identity();
    Vec3 scale{1.0f, 1.0f, 1.0f};

    Mat4 ToMat4() const {
        return Mat4::Translation(position) * ::af::ToMat4(rotation) * Mat4::Scaling(scale);
    }

    // Local basis vectors in world space (camera-style: forward is -Z).
    Vec3 Forward() const { return Rotate(rotation, Vec3{0.0f, 0.0f, -1.0f}); }
    Vec3 Right() const { return Rotate(rotation, Vec3{1.0f, 0.0f, 0.0f}); }
    Vec3 Up() const { return Rotate(rotation, Vec3{0.0f, 1.0f, 0.0f}); }
};

}  // namespace af
