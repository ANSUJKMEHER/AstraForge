// AstraForge Studio & Arena — engine entry point.
//
// Interactive Game Development Environment & Playable Arena.
// Features:
//   - C++20 Sparse-Set ECS, Spatial Hash Collision, Fixed-Timestep Physics
//   - Dear ImGui Studio: Object Spawner, Inspector, Tuner, Audio, Scene Manager
//   - Real-time Procedural Audio Synthesizer (Zero asset dependencies)
//   - 3D Particle Effects System (Sparks, Debris, Explosions)
//   - Turret / Defense Plants (Plants vs Zombies mechanics) & Health Pickups
//   - Dynamic Fresnel Lighting & Antialiased Cyber Grid
//
// Controls:
//   TAB      — Toggle Dev Studio Editor (Cursor Free) / Play Mode (Mouse Locked)
//   mouse    — Look around / Aim
//   W A S D  — Move the player (camera-relative)
//   Space    — Jump
//   LMB      — Shoot (aim = camera forward)
//   R        — Restart arena session
//   Esc      — Quit

#include <cstddef>
#include <cstdio>
#include <vector>
#include <cmath>

#include <SDL.h>

#include "af/app/Application.h"
#include "af/app/AudioSystem.h"
#include "af/app/DevUI.h"
#include "af/app/ParticleSystem.h"
#include "af/app/Screenshot.h"
#include "af/core/Camera.h"
#include "af/core/Components.h"
#include "af/game/GameSession.h"
#include "af/game/GameplayComponents.h"
#include "af/physics/PhysicsComponents.h"
#include "af/render/RenderSystem.h"
#include "af/render/Renderer.h"

using namespace af;

namespace {

struct CameraControl {
    float yaw = 0.0f;
    float pitch = 0.48f;      // Elevated stylish angle showing the full arena
    float distance = 13.5f;
};

void UpdatePlayerAndCamera(const InputState& input, CameraControl& cam,
                           PlayerInput& playerInput, bool editorMode) {
    constexpr float LookSensitivity = 0.003f;

    // In play mode, mouse rotates camera.
    // In editor mode, hold Right Mouse Button to rotate camera, or interact with UI.
    if (!editorMode || input.mouseButtons[SDL_BUTTON_RIGHT]) {
        cam.yaw -= static_cast<float>(input.mouseDx) * LookSensitivity;
        cam.pitch -= static_cast<float>(input.mouseDy) * LookSensitivity;
    }
    if (!editorMode) {
        cam.distance -= static_cast<float>(input.wheelY) * 0.8f;
        if (cam.distance < 4.0f) cam.distance = 4.0f;
    }

    Camera probe;
    probe.Orbit(Vec3{0.0f, 0.0f, 0.0f}, cam.yaw, cam.pitch, cam.distance);
    const Vec3 forwardXZ = Normalize(Vec3{probe.Forward().x, 0.0f, probe.Forward().z});
    const Vec3 rightXZ = Normalize(Vec3{probe.Right().x, 0.0f, probe.Right().z});

    Vec2 move{0.0f, 0.0f};
    if (input.keys[SDL_SCANCODE_W]) { move.x += forwardXZ.x; move.y += forwardXZ.z; }
    if (input.keys[SDL_SCANCODE_S]) { move.x -= forwardXZ.x; move.y -= forwardXZ.z; }
    if (input.keys[SDL_SCANCODE_D]) { move.x += rightXZ.x; move.y += rightXZ.z; }
    if (input.keys[SDL_SCANCODE_A]) { move.x -= rightXZ.x; move.y -= rightXZ.z; }

    playerInput.move = move;
    playerInput.jump = input.keys[SDL_SCANCODE_SPACE];
    playerInput.shoot = (!editorMode) && input.mouseButtons[SDL_BUTTON_LEFT];
    playerInput.aim = Normalize(probe.Forward());
}

void SyncGameplayRenderables(GameSession& session) {
    World& world = session.GetWorld();
    struct Pending { Entity entity; Renderable renderable; };
    static std::vector<Pending> pending;

    pending.clear();
    if (session.PlayerAlive()) {
        const Entity p = session.Player();
        if (!world.HasComponent<Renderable>(p)) {
            pending.push_back({p, Renderable{MeshId::Cube,
                                             {0.08f, 0.88f, 1.0f, 1.0f}}});  // Electric Cyan
        }
    }

    for (auto [e, enemy] : world.ViewComponents<Enemy>()) {
        if (!world.HasComponent<Renderable>(e)) {
            pending.push_back({e, Renderable{MeshId::Sphere,
                                             {0.92f, 0.20f, 0.25f, 1.0f}}});  // Vibrant ruby
        } else {
            auto& ren = world.GetComponent<Renderable>(e);
            if (enemy.state == EnemyState::Hurt) {
                ren.color = Vec4{1.0f, 1.0f, 0.35f, 1.0f};   // Electric yellow flash
            } else if (enemy.state == EnemyState::Attack) {
                ren.color = Vec4{1.0f, 0.12f, 0.12f, 1.0f};  // Blazing scarlet
            } else if (enemy.state == EnemyState::Chase) {
                ren.color = Vec4{1.0f, 0.38f, 0.10f, 1.0f};  // Fiery aggressive vermilion
            } else {
                ren.color = Vec4{0.92f, 0.20f, 0.25f, 1.0f};  // Rich crimson ruby
            }
        }
    }

    for (auto [e, projectile] : world.ViewComponents<Projectile>()) {
        (void)projectile;
        if (!world.HasComponent<Renderable>(e)) {
            pending.push_back({e, Renderable{MeshId::Sphere,
                                             {1.0f, 0.90f, 0.25f, 1.0f}}});
        }
    }

    for (auto [e, turret] : world.ViewComponents<Turret>()) {
        (void)turret;
        if (!world.HasComponent<Renderable>(e)) {
            pending.push_back({e, Renderable{MeshId::Cube,
                                             {0.15f, 0.85f, 0.40f, 1.0f}}});
        }
    }

    for (auto [e, pickup] : world.ViewComponents<HealthPickup>()) {
        (void)pickup;
        if (!world.HasComponent<Renderable>(e)) {
            pending.push_back({e, Renderable{MeshId::Sphere,
                                             {0.20f, 0.95f, 0.40f, 1.0f}}});
        }
    }

    for (auto [e, collider] : world.ViewComponents<Collider>()) {
        if (collider.isStatic && !world.HasComponent<Renderable>(e)) {
            pending.push_back({e, Renderable{MeshId::Cube,
                                             {0.50f, 0.54f, 0.60f, 1.0f}}});
        }
    }

    for (const Pending& p : pending) {
        world.AddComponent<Renderable>(p.entity, p.renderable);
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    Application app("AstraForge Studio & Arena", 1280, 720);
    if (!app.IsValid()) {
        std::fprintf(stderr, "AstraForge: failed to create window/context\n");
        return 1;
    }

    Renderer renderer;
    if (!renderer.Init()) {
        std::fprintf(stderr, "AstraForge: renderer init failed (OpenGL 3.3 required)\n");
        return 1;
    }
    renderer.SetViewportSize(1280, 720);

    // Audio & Particle subsystems
    AudioSystem audio;
    if (!audio.Init()) {
        std::fprintf(stderr, "AstraForge: Audio init failed, continuing silent\n");
    }
    ParticleSystem particles;

    // Headless-testable game session
    GameSession session;
    World& world = session.GetWorld();

    // Floor skin
    {
        const Entity floor = world.CreateEntity();
        world.AddComponent<Transform>(floor).scale = Vec3{42.0f, 1.0f, 42.0f};
        world.AddComponent<Renderable>(floor, Renderable{MeshId::Plane,
                                                         {0.28f, 0.32f, 0.38f, 1.0f}});
    }

    // Static arena boundary wall skins
    {
        std::vector<Entity> walls;
        for (auto [e, collider] : world.ViewComponents<Collider>()) {
            if (collider.isStatic) walls.push_back(e);
        }
        for (const Entity w : walls) {
            const auto& collider = world.GetComponent<Collider>(w);
            world.GetComponent<Transform>(w).scale = collider.halfExtents * 2.0f;
            world.AddComponent<Renderable>(w, Renderable{MeshId::Cube,
                                                         {0.50f, 0.52f, 0.58f, 1.0f}});
        }
    }

    Camera camera;
    camera.SetPerspective(60.0f, 1280.0f / 720.0f, 0.1f, 200.0f);

    DevUI devUI;
    if (!devUI.Init(app.windowHandle(), app.glContext())) {
        std::fprintf(stderr, "AstraForge: DevUI init failed\n");
    }

    const double fixedDt = 1.0 / 60.0;
    int renderedFrames = 0;
    uint64_t fpsWindowStart = SDL_GetTicks64();
    uint64_t lastFrameTicks = SDL_GetTicks64();
    PlayerInput playerInput;
    CameraControl camControl;

    // Audio & Visual trigger state trackers
    std::size_t lastHits = 0;
    std::size_t lastKills = 0;
    bool wasShooting = false;
    bool wasJumping = false;
    int lastPlayerHp = 5;

    while (!app.ShouldQuit()) {
        app.PumpEvents([&](const SDL_Event& event) {
            return devUI.ProcessEvent(event);
        });

        if (app.GetInput().quitRequested) break;

        // Esc toggles Studio & releases cursor safely without quitting
        static bool wasEsc = false;
        if (app.GetInput().keys[SDL_SCANCODE_ESCAPE]) {
            if (!wasEsc) devUI.ToggleEditorMode();
            wasEsc = true;
        } else {
            wasEsc = false;
        }

        // F12 captures high-resolution screenshot
        static bool wasF12 = false;
        if (app.GetInput().keys[SDL_SCANCODE_F12]) {
            if (!wasF12) {
                int w = 1280, h = 720;
                SDL_GetWindowSize(app.windowHandle(), &w, &h);
                std::string filename;
                if (CaptureScreenshot(w, h, filename)) {
                    std::printf("AstraForge: screenshot captured -> %s\n", filename.c_str());
                    audio.Play(SoundEffect::Pickup);  // camera shutter sound
                }
            }
            wasF12 = true;
        } else {
            wasF12 = false;
        }

        if (app.GetInput().keys[SDL_SCANCODE_R]) session.Restart();

        int width = 1280;
        int height = 720;
        SDL_GetWindowSize(app.windowHandle(), &width, &height);
        renderer.SetViewportSize(width, height);
        camera.SetAspect(static_cast<float>(width) /
                         static_cast<float>(height > 0 ? height : 1));

        UpdatePlayerAndCamera(app.GetInput(), camControl, playerInput, devUI.IsEditorMode());

        // Audio trigger: Shoot
        if (playerInput.shoot && !wasShooting) {
            audio.Play(SoundEffect::Shoot);
        }
        wasShooting = playerInput.shoot;

        // Audio trigger: Jump
        if (playerInput.jump && !wasJumping) {
            audio.Play(SoundEffect::Jump);
        }
        wasJumping = playerInput.jump;

        // Camera orbits the player with screen shake offset
        Vec3 playerPos{0.0f, 1.0f, 0.0f};
        if (session.PlayerAlive()) {
            playerPos = world.GetComponent<Transform>(session.Player()).position;
            const auto* hp = world.TryGetComponent<Health>(session.Player());
            if (hp && hp->current > lastPlayerHp) {
                audio.Play(SoundEffect::Pickup);  // healed
            } else if (hp && hp->current < lastPlayerHp) {
                audio.Play(SoundEffect::Hit);     // player hurt
                devUI.TriggerScreenShake(1.2f);
            }
            if (hp) lastPlayerHp = hp->current;
        }

        const float shake = devUI.ScreenShake();
        const Vec3 shakeOffset{
            shake > 0.0f ? std::sin(static_cast<float>(SDL_GetTicks64()) * 0.05f) * shake * 0.3f : 0.0f,
            shake > 0.0f ? std::cos(static_cast<float>(SDL_GetTicks64()) * 0.07f) * shake * 0.3f : 0.0f,
            0.0f
        };

        camera.Orbit(playerPos + Vec3{0.0f, 1.2f, 0.0f} + shakeOffset,
                     camControl.yaw, camControl.pitch, camControl.distance);

        // Simulation update: fixed-step driver
        if (!devUI.IsPaused() || devUI.ShouldStepSingleFrame()) {
            const float scaledDt = static_cast<float>(fixedDt) * devUI.TimeScale();
            app.RunFixedSteps([&](float dt) {
                session.SetPlayerInput(playerInput);
                session.Update(dt);
            }, scaledDt);
        }

        // Combat feedback checks (Sound + Particle bursts)
        const auto& cStats = session.CombatStats();
        if (cStats.hits > lastHits) {
            audio.Play(SoundEffect::Hit);
            // Spawn sparks near player aim
            particles.EmitSparks(playerPos + Normalize(playerInput.aim) * 3.0f,
                                 Vec3{0.0f, 1.0f, 0.0f}, Vec4{1.0f, 0.8f, 0.2f, 1.0f}, 8);
            lastHits = cStats.hits;
        }
        if (cStats.kills > lastKills) {
            audio.Play(SoundEffect::Explosion);
            devUI.TriggerScreenShake(0.8f);
            particles.EmitExplosion(playerPos + Normalize(playerInput.aim) * 4.0f,
                                  Vec4{1.0f, 0.35f, 0.15f, 1.0f}, 24);
            lastKills = cStats.kills;
        }

        SyncGameplayRenderables(session);

        const uint64_t currentTicks = SDL_GetTicks64();
        const float frameDelta = static_cast<float>(currentTicks - lastFrameTicks) / 1000.0f;
        lastFrameTicks = currentTicks;

        particles.Update(frameDelta);

        // 3D Scene Rendering
        renderer.BeginFrame(camera, devUI.LightDirection(), devUI.LightColor(), camera.Position());
        RenderScene(world, renderer, camera);
        particles.Render(renderer);
        renderer.EndFrame();

        // DevUI & HUD Rendering
        devUI.BeginFrame();
        devUI.Render(session, renderer, camera, audio, frameDelta);
        devUI.EndFrame();

        app.Present();
        ++renderedFrames;

        const uint64_t now = SDL_GetTicks64();
        if (now - fpsWindowStart >= 1000) {
            const double seconds = static_cast<double>(now - fpsWindowStart) / 1000.0;
            const auto& stats = renderer.LastStats();
            std::printf(
                "fps=%.1f drawCalls=%zu culled=%zu score=%d kills=%d "
                "enemies=%zu hp=%d particles=%zu\n",
                renderedFrames / seconds, stats.drawCalls, stats.culled,
                session.Score(), session.Kills(), session.EnemyCount(),
                session.PlayerAlive() ? world.GetComponent<Health>(session.Player()).current : 0,
                particles.ActiveCount());
            renderedFrames = 0;
            fpsWindowStart = now;
        }
    }

    std::printf("AstraForge: shutdown cleanly\n");
    return 0;
}
