#pragma once
// Camera — orbit-style view with perspective projection, built on our own
// Mat4 (docs/math-conventions.md). Pitch is clamped to ±89° so LookAt never
// degenerates (documented in Mat4.h).

#include "af/core/Frustum.h"
#include "af/math/Mat4.h"
#include "af/math/MathUtil.h"
#include "af/math/Vec3.h"

namespace af {

class Camera {
public:
    Camera() = default;

    void SetPerspective(float fovYDegrees, float aspect, float nearPlane,
                        float farPlane);

    // Live aspect update (window resize).
    void SetAspect(float aspect) { aspect_ = aspect; }

    // Current yaw/pitch (radians) — for input-driven orbit controls.
    void GetYawPitch(float* yaw, float* pitch) const {
        *yaw = yaw_;
        *pitch = pitch_;
    }

    // Direct placement: position + yaw/pitch (radians). Pitch clamped ±89°.
    void SetTransform(Vec3 position, float yawRad, float pitchRad);

    // Orbit placement: position = target − forward · distance.
    void Orbit(Vec3 target, float yawRad, float pitchRad, float distance);

    const Vec3& Position() const { return position_; }
    Vec3 Target() const { return target_; }
    float Distance() const { return distance_; }

    // Camera-local basis (yaw/pitch convention; see implementation).
    Vec3 Forward() const;
    Vec3 Right() const;
    Vec3 Up() const;

    Mat4 ViewMatrix() const;
    Mat4 ProjectionMatrix() const;
    Mat4 ViewProjection() const { return ProjectionMatrix() * ViewMatrix(); }
    Frustum GetFrustum() const { return Frustum::FromViewProjection(ViewProjection()); }

    void SetTarget(Vec3 target) { target_ = target; UpdatePosition(); }

private:
    void UpdatePosition();  // recompute position_ from target_, yaw_, pitch_, distance_

    Vec3 target_{0.0f, 0.0f, 0.0f};
    Vec3 position_{0.0f, 0.0f, 5.0f};
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float distance_ = 5.0f;
    float fovY_ = Radians(60.0f);
    float aspect_ = 16.0f / 9.0f;
    float nearPlane_ = 0.1f;
    float farPlane_ = 100.0f;
};

}  // namespace af
