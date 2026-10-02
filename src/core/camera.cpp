#include "af/core/Camera.h"

#include <cmath>

#include "af/math/Vec3.h"

namespace af {

namespace {

// Forward from yaw/pitch: yaw=0, pitch=0 looks down −Z.
// +yaw turns toward +X (right); +pitch turns toward +Y (up).
Vec3 DirectionFromYawPitch(float yaw, float pitch) {
    return {std::cos(pitch) * std::sin(yaw), std::sin(pitch),
            -std::cos(pitch) * std::cos(yaw)};
}

constexpr float MaxPitch = 1.55334f;  // 89 degrees, avoids LookAt degeneracy

}  // namespace

void Camera::SetPerspective(float fovYDegrees, float aspect, float nearPlane,
                            float farPlane) {
    fovY_ = Radians(fovYDegrees);
    aspect_ = aspect;
    nearPlane_ = nearPlane;
    farPlane_ = farPlane;
}

void Camera::SetTransform(Vec3 position, float yawRad, float pitchRad) {
    position_ = position;
    yaw_ = yawRad;
    pitch_ = Clamp(pitchRad, -MaxPitch, MaxPitch);
    // Keep target/distance coherent so Orbit-style controls still work.
    distance_ = ::af::Distance(position_, target_);
}

void Camera::Orbit(Vec3 target, float yawRad, float pitchRad, float distance) {
    target_ = target;
    yaw_ = yawRad;
    pitch_ = Clamp(pitchRad, -MaxPitch, MaxPitch);
    distance_ = distance;
    UpdatePosition();
}

void Camera::UpdatePosition() {
    position_ = target_ - DirectionFromYawPitch(yaw_, pitch_) * distance_;
}

Vec3 Camera::Forward() const { return DirectionFromYawPitch(yaw_, pitch_); }

Vec3 Camera::Right() const {
    return Normalize(Cross(Forward(), Vec3{0.0f, 1.0f, 0.0f}));
}

Vec3 Camera::Up() const { return Normalize(Cross(Right(), Forward())); }

Mat4 Camera::ViewMatrix() const {
    return LookAt(position_, position_ + Forward(), Vec3{0.0f, 1.0f, 0.0f});
}

Mat4 Camera::ProjectionMatrix() const {
    return Perspective(fovY_, aspect_, nearPlane_, farPlane_);
}

}  // namespace af
