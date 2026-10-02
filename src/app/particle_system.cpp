#include "af/app/ParticleSystem.h"

#include <cmath>
#include <algorithm>
#include "af/render/Renderer.h"
#include "af/math/Mat4.h"

namespace af {

ParticleSystem::ParticleSystem() = default;

float ParticleSystem::RandomFloat(float min, float max) {
    rngState_ = rngState_ * 1664525u + 1013904223u;
    const float unit = static_cast<float>(rngState_ >> 16) / 65535.0f;
    return min + unit * (max - min);
}

Vec3 ParticleSystem::RandomSphereDir() {
    const float u = RandomFloat(0.0f, 1.0f);
    const float v = RandomFloat(0.0f, 1.0f);
    const float theta = u * 6.2831853f;
    const float phi = std::acos(2.0f * v - 1.0f);
    return Vec3{
        std::sin(phi) * std::cos(theta),
        std::abs(std::cos(phi)),  // bias upward
        std::sin(phi) * std::sin(theta)
    };
}

void ParticleSystem::EmitExplosion(const Vec3& pos, const Vec4& color, int count) {
    int emitted = 0;
    for (size_t i = 0; i < MaxParticles && emitted < count; ++i) {
        if (!particles_[i].active) {
            Particle& p = particles_[i];
            p.active = true;
            p.position = pos;
            const float speed = RandomFloat(3.0f, 8.5f);
            p.velocity = RandomSphereDir() * speed + Vec3{0.0f, RandomFloat(1.5f, 4.0f), 0.0f};
            p.life = RandomFloat(0.35f, 0.75f);
            p.maxLife = p.life;
            p.scale = RandomFloat(0.10f, 0.22f);
            p.color = color;
            ++emitted;
        }
    }
}

void ParticleSystem::EmitSparks(const Vec3& pos, const Vec3& normal, const Vec4& color, int count) {
    int emitted = 0;
    for (size_t i = 0; i < MaxParticles && emitted < count; ++i) {
        if (!particles_[i].active) {
            Particle& p = particles_[i];
            p.active = true;
            p.position = pos + normal * 0.1f;
            const Vec3 randDir = RandomSphereDir();
            p.velocity = (randDir + normal * 1.5f) * RandomFloat(2.5f, 6.0f);
            p.life = RandomFloat(0.2f, 0.45f);
            p.maxLife = p.life;
            p.scale = RandomFloat(0.07f, 0.14f);
            p.color = color;
            ++emitted;
        }
    }
}

void ParticleSystem::Update(float dt) {
    for (size_t i = 0; i < MaxParticles; ++i) {
        Particle& p = particles_[i];
        if (!p.active) continue;

        p.position += p.velocity * dt;
        p.velocity.y -= 14.0f * dt;  // gravity

        // Bounce on floor
        if (p.position.y < 0.06f) {
            p.position.y = 0.06f;
            p.velocity.y = -p.velocity.y * 0.4f;
            p.velocity.x *= 0.7f;
            p.velocity.z *= 0.7f;
        }

        p.life -= dt;
        if (p.life <= 0.0f) {
            p.active = false;
        }
    }
}

void ParticleSystem::Render(Renderer& renderer) {
    for (size_t i = 0; i < MaxParticles; ++i) {
        const Particle& p = particles_[i];
        if (!p.active) continue;

        const float alpha = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        Vec4 col = p.color;
        col.w = alpha;

        const Mat4 model = Mat4::Translation(p.position) *
                           Mat4::Scaling(Vec3{p.scale, p.scale, p.scale});
        const Vec3 halfExtents{p.scale * 0.5f, p.scale * 0.5f, p.scale * 0.5f};
        renderer.Submit(MeshId::Cube, model, col,
                        p.position - halfExtents, p.position + halfExtents,
                        p.position, p.scale);
    }
}

void ParticleSystem::Clear() {
    for (size_t i = 0; i < MaxParticles; ++i) {
        particles_[i].active = false;
    }
}

size_t ParticleSystem::ActiveCount() const {
    size_t count = 0;
    for (size_t i = 0; i < MaxParticles; ++i) {
        if (particles_[i].active) ++count;
    }
    return count;
}

}  // namespace af
