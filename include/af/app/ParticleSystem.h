#pragma once

#include <cstdint>
#include "af/math/Vec3.h"
#include "af/math/Vec4.h"

namespace af {

class Renderer;

struct Particle {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    float life = 0.0f;
    float maxLife = 1.0f;
    float scale = 0.15f;
    bool active = false;
};

class ParticleSystem {
public:
    ParticleSystem();

    void Update(float dt);
    void Render(Renderer& renderer);

    void EmitExplosion(const Vec3& pos, const Vec4& color, int count = 24);
    void EmitSparks(const Vec3& pos, const Vec3& normal, const Vec4& color, int count = 12);
    void Clear();

    size_t ActiveCount() const;

private:
    static constexpr size_t MaxParticles = 256;
    Particle particles_[MaxParticles] = {};
    uint32_t rngState_ = 54321u;

    float RandomFloat(float min, float max);
    Vec3 RandomSphereDir();
};

}  // namespace af
