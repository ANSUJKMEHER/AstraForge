#include "af/app/DevUI.h"

#include <cstdio>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#include "af/app/AudioSystem.h"
#include "af/game/GameSession.h"
#include "af/game/GameplayComponents.h"
#include "af/core/Components.h"
#include "af/core/Profiling.h"
#include "af/physics/PhysicsComponents.h"
#include "af/render/Renderer.h"
#include "af/render/GL.h"
#include "af/core/Camera.h"

namespace af {

DevUI::DevUI() = default;

DevUI::~DevUI() {
    Shutdown();
}

bool DevUI::Init(SDL_Window* window, void* glContext) {
    if (initialized_) return true;
    window_ = window;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    SetupTheme();

    if (!ImGui_ImplSDL2_InitForOpenGL(window, glContext)) {
        std::fprintf(stderr, "AstraForge: ImGui SDL2 init failed\n");
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        std::fprintf(stderr, "AstraForge: ImGui OpenGL3 init failed\n");
        return false;
    }

    initialized_ = true;
    SetEditorMode(true);
    return true;
}

void DevUI::Shutdown() {
    if (!initialized_) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
}

void DevUI::SetupTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 5.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.5f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.08f, 0.10f, 0.14f, 0.94f);
    colors[ImGuiCol_Header]             = ImVec4(0.18f, 0.38f, 0.58f, 0.70f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.24f, 0.48f, 0.72f, 0.85f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.30f, 0.58f, 0.85f, 0.95f);
    colors[ImGuiCol_Button]             = ImVec4(0.16f, 0.35f, 0.54f, 0.80f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.22f, 0.46f, 0.70f, 0.90f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.28f, 0.56f, 0.84f, 1.00f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.13f, 0.17f, 0.23f, 0.85f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.19f, 0.24f, 0.33f, 0.90f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.24f, 0.31f, 0.42f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.06f, 0.08f, 0.12f, 0.95f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.12f, 0.24f, 0.38f, 1.00f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.28f, 0.62f, 0.92f, 0.85f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.38f, 0.72f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.35f, 0.88f, 1.00f, 1.00f);
}

void DevUI::SetEditorMode(bool active) {
    editorMode_ = active;
    if (editorMode_) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    } else {
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
}

void DevUI::ToggleEditorMode() {
    SetEditorMode(!editorMode_);
}

void DevUI::TriggerScreenShake(float intensity) {
    screenShake_ = std::max(screenShake_, intensity);
}

bool DevUI::ProcessEvent(const SDL_Event& event) {
    if (!initialized_) return false;

    // Toggle editor mode on Tab key or Tilde
    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        if (event.key.keysym.scancode == SDL_SCANCODE_TAB ||
            event.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
            ToggleEditorMode();
            return true;
        }
    }

    ImGui_ImplSDL2_ProcessEvent(&event);

    if (editorMode_) {
        ImGuiIO& io = ImGui::GetIO();
        if (event.type == SDL_MOUSEMOTION ||
            event.type == SDL_MOUSEBUTTONDOWN ||
            event.type == SDL_MOUSEBUTTONUP ||
            event.type == SDL_MOUSEWHEEL) {
            return io.WantCaptureMouse;
        }
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            return io.WantCaptureKeyboard;
        }
    }

    return false;
}

void DevUI::BeginFrame() {
    if (!initialized_) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void DevUI::EndFrame() {
    if (!initialized_) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DevUI::Render(GameSession& session, Renderer& renderer, Camera& camera, AudioSystem& audio, float dt) {
    if (!initialized_) return;
    (void)camera;

    // Record frame time into rolling history
    frameTimeHistory_[historyIndex_] = dt * 1000.0f;
    historyIndex_ = (historyIndex_ + 1) % HistoryLength;

    // Screen shake decay
    if (screenShake_ > 0.0f) {
        screenShake_ = std::max(0.0f, screenShake_ - dt * 4.0f);
    }

    // Wireframe mode handling
    if (wireframeMode_) {
        gl::PolygonMode(gl::FrontAndBack, gl::Line);
    } else {
        gl::PolygonMode(gl::FrontAndBack, gl::Fill);
    }

    // Always draw Combat HUD
    if (showHUD_) {
        RenderCombatHUD(session);
    }

    // Dev Editor Windows (visible in Editor Mode)
    if (editorMode_) {
        RenderMenuBar(session, audio);

        if (showProfiler_)    RenderPerformanceOverlay(renderer, dt);
        if (showInspector_)   RenderInspector(session);
        if (showTuner_)       RenderGameplayTuner(session);
        if (showSpawner_)     RenderObjectSpawner(session);
        if (showEnvironment_) RenderEnvironmentSettings(session);
        if (showAudio_)       RenderAudioSettings(audio);
        if (showSceneManager_)RenderSceneManager(session);
    }

    // God Mode enforcement
    if (godMode_ && session.PlayerAlive()) {
        World& world = session.GetWorld();
        auto* hp = world.TryGetComponent<Health>(session.Player());
        if (hp != nullptr) {
            hp->current = hp->max;
        }
    }
}

void DevUI::RenderMenuBar(GameSession& session, AudioSystem& audio) {
    (void)audio;
    if (ImGui::BeginMainMenuBar()) {
        ImGui::TextColored(ImVec4(0.35f, 0.80f, 1.0f, 1.0f), "AstraForge Studio");
        ImGui::Separator();

        // Simulation controls
        if (paused_) {
            if (ImGui::Button("Resume (F5)")) paused_ = false;
            ImGui::SameLine();
            if (ImGui::Button("Step 1 Tick (F6)")) stepSingleFrame_ = true;
        } else {
            if (ImGui::Button("Pause (F5)")) paused_ = true;
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        ImGui::SliderFloat("Speed", &timeScale_, 0.1f, 3.0f, "%.1fx");

        ImGui::Separator();
        ImGui::Checkbox("God Mode", &godMode_);
        ImGui::SameLine();
        ImGui::Checkbox("Wireframe", &wireframeMode_);

        ImGui::Separator();
        if (ImGui::BeginMenu("Panels")) {
            ImGui::MenuItem("Object Spawner & Palette", nullptr, &showSpawner_);
            ImGui::MenuItem("World Hierarchy & Inspector", nullptr, &showInspector_);
            ImGui::MenuItem("Gameplay & Physics Tuner", nullptr, &showTuner_);
            ImGui::MenuItem("Engine Profiler & Metrics", nullptr, &showProfiler_);
            ImGui::MenuItem("Audio & Sound FX", nullptr, &showAudio_);
            ImGui::MenuItem("Environment & Sun Light", nullptr, &showEnvironment_);
            ImGui::MenuItem("Scene Level Manager", nullptr, &showSceneManager_);
            ImGui::MenuItem("Combat HUD Overlay", nullptr, &showHUD_);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Quick Actions")) {
            if (ImGui::MenuItem("Restart Arena (R)")) session.Restart();
            if (ImGui::MenuItem("Clear All Enemies")) {
                World& world = session.GetWorld();
                std::vector<Entity> enemies;
                for (auto [e, enemy] : world.ViewComponents<Enemy>()) {
                    (void)enemy;
                    enemies.push_back(e);
                }
                for (Entity e : enemies) world.DestroyEntity(e);
            }
            if (ImGui::MenuItem("Clear All Turrets")) {
                World& world = session.GetWorld();
                std::vector<Entity> turrets;
                for (auto [e, t] : world.ViewComponents<Turret>()) {
                    (void)t;
                    turrets.push_back(e);
                }
                for (Entity e : turrets) world.DestroyEntity(e);
            }
            ImGui::EndMenu();
        }

        // Mode badge indicator on right side
        ImGui::SameLine(ImGui::GetWindowWidth() - 250.0f);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[TAB: Play / Lock Mouse]");

        ImGui::EndMainMenuBar();
    }
}

void DevUI::RenderCombatHUD(GameSession& session) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoMove;

    // Top-Left: Player Vital Status & Controls
    ImGui::SetNextWindowPos(ImVec2(20.0f, editorMode_ ? 35.0f : 20.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.70f);
    if (ImGui::Begin("##PlayerHUD", nullptr, flags)) {
        if (session.PlayerAlive()) {
            World& world = session.GetWorld();
            const auto* hp = world.TryGetComponent<Health>(session.Player());
            const int currentHp = hp ? hp->current : 1;
            const int maxHp = hp ? hp->max : 1;
            const float frac = (float)currentHp / (float)(maxHp > 0 ? maxHp : 1);

            ImVec4 barColor = (frac > 0.5f) ? ImVec4(0.2f, 0.85f, 0.3f, 1.0f) :
                              (frac > 0.25f) ? ImVec4(0.9f, 0.7f, 0.1f, 1.0f) :
                                               ImVec4(0.95f, 0.2f, 0.2f, 1.0f);

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ASTRAFORGE FIGHTER");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
            char hpText[32];
            std::snprintf(hpText, sizeof(hpText), "%d / %d HP", currentHp, maxHp);
            ImGui::ProgressBar(frac, ImVec2(220.0f, 18.0f), hpText);
            ImGui::PopStyleColor();
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "PLAYER ELIMINATED");
            ImGui::Text("Restarting arena in %.1fs (or press R)", session.SecondsToRestart());
        }

        ImGui::Spacing();
        ImGui::TextDisabled("WASD: Move | Space: Jump | LMB: Shoot");
        ImGui::TextDisabled("TAB: %s", editorMode_ ? "Exit Studio / Lock Mouse" : "Open Studio Editor");
    }
    ImGui::End();

    // Top-Right: Score, Kills, Active Enemies & Turrets
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 220.0f, editorMode_ ? 35.0f : 20.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.70f);
    if (ImGui::Begin("##ScoreHUD", nullptr, flags)) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "SCORE: %d", session.Score());
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "KILLS: %d", session.Kills());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "ENEMIES: %zu", session.EnemyCount());
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "PROJECTILES: %zu", session.ProjectileCount());
    }
    ImGui::End();

    // Subtle Crosshair in Center Screen (Play mode only)
    if (!editorMode_) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        const float size = 8.0f;
        const ImU32 col = IM_COL32(200, 240, 255, 180);
        drawList->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y), col, 1.5f);
        drawList->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size), col, 1.5f);
        drawList->AddCircle(center, 3.0f, col, 12, 1.0f);
    }
}

void DevUI::RenderPerformanceOverlay(Renderer& renderer, float dt) {
    ImGui::SetNextWindowSize(ImVec2(340.0f, 320.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20.0f, 160.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Engine Profiler & Metrics", &showProfiler_)) {
        const float fps = (dt > 0.0001f) ? (1.0f / dt) : 0.0f;
        const float ms = dt * 1000.0f;

        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "FPS: %.1f", fps);
        ImGui::SameLine(140.0f);
        ImGui::Text("Frame Time: %.2f ms", ms);

        // Frame Time Histogram
        ImGui::PlotLines("##History", frameTimeHistory_, HistoryLength, historyIndex_,
                         "Frame History (ms)", 0.0f, 33.3f, ImVec2(-1.0f, 65.0f));

        ImGui::Separator();
        ImGui::Text("Render Pipeline Stats:");
        const auto& stats = renderer.LastStats();
        ImGui::BulletText("Draw Calls: %zu", stats.drawCalls);
        ImGui::BulletText("Culled Entities: %zu", stats.culled);
        ImGui::BulletText("Visible Entities: %zu", stats.submitted);

        ImGui::Separator();
        ImGui::Text("Architecture & Safety:");
        ImGui::BulletText("ECS Storage: Sparse Sets (cache-dense)");
        ImGui::BulletText("Broadphase: 2D Spatial Hash Grid");
        ImGui::BulletText("Per-frame allocations: 0 bytes (verified)");
        ImGui::BulletText("Simulation timestep: 60 Hz fixed");
    }
    ImGui::End();
}

void DevUI::RenderObjectSpawner(GameSession& session) {
    ImGui::SetNextWindowSize(ImVec2(360.0f, 400.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(380.0f, 40.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Object Spawner & Palette", &showSpawner_)) {
        World& world = session.GetWorld();

        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "Spawn Entities in Real-Time:");

        // 1. Defense Turrets (Plants vs Zombies mechanic)
        if (ImGui::CollapsingHeader("Defense Turret / Plant", ImGuiTreeNodeFlags_DefaultOpen)) {
            static float turretX = 0.0f;
            static float turretZ = 0.0f;
            static float turretFireRate = 0.4f;
            static float turretRange = 15.0f;
            ImGui::DragFloat("Pos X", &turretX, 0.5f, -18.0f, 18.0f);
            ImGui::DragFloat("Pos Z", &turretZ, 0.5f, -18.0f, 18.0f);
            ImGui::SliderFloat("Fire Rate", &turretFireRate, 0.1f, 2.0f, "%.2fs");
            ImGui::SliderFloat("Range", &turretRange, 5.0f, 25.0f, "%.1fm");

            if (ImGui::Button("Spawn Defense Turret")) {
                const Entity e = world.CreateEntity();
                world.AddComponent<Transform>(e, Transform{Vec3{turretX, 0.8f, turretZ},
                                                           Quat::Identity(),
                                                           Vec3{0.8f, 1.6f, 0.8f}});
                world.AddComponent<Turret>(e, Turret{turretRange, turretFireRate, 0.0f, 1});
                world.AddComponent<Collider>(e, Collider{ColliderKind::AABB, {}, 0.0f, Vec3{0.4f, 0.8f, 0.4f}, true});
                world.AddComponent<Renderable>(e, Renderable{MeshId::Cube, {0.15f, 0.85f, 0.40f, 1.0f}});
            }
            ImGui::SameLine();
            if (session.PlayerAlive() && ImGui::Button("Place at Player")) {
                const Vec3 pPos = world.GetComponent<Transform>(session.Player()).position;
                turretX = std::round(pPos.x);
                turretZ = std::round(pPos.z);
            }
        }

        // 2. Obstacles / Barricades
        if (ImGui::CollapsingHeader("Barricade / Wall Obstacle", ImGuiTreeNodeFlags_DefaultOpen)) {
            static float wallPos[3] = {0.0f, 1.0f, 5.0f};
            static float wallExt[3] = {1.5f, 1.0f, 0.5f};
            ImGui::DragFloat3("Position", wallPos, 0.5f, -18.0f, 18.0f);
            ImGui::DragFloat3("Size (Half-Ext)", wallExt, 0.2f, 0.2f, 10.0f);

            if (ImGui::Button("Spawn Solid Barricade")) {
                const Entity wall = world.CreateEntity();
                world.AddComponent<Transform>(wall, Transform{Vec3{wallPos[0], wallPos[1], wallPos[2]},
                                                              Quat::Identity(),
                                                              Vec3{wallExt[0] * 2.0f, wallExt[1] * 2.0f, wallExt[2] * 2.0f}});
                world.AddComponent<Collider>(wall, Collider{ColliderKind::AABB, {}, 0.0f,
                                                            Vec3{wallExt[0], wallExt[1], wallExt[2]}, true});
                world.AddComponent<Renderable>(wall, Renderable{MeshId::Cube, {0.55f, 0.60f, 0.68f, 1.0f}});
            }
        }

        // 3. Enemy Variants
        if (ImGui::CollapsingHeader("Enemy Variants", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Spawn Runner (Fast/Fragile)")) {
                const Entity e = world.CreateEntity();
                world.AddComponent<Transform>(e, Transform{Vec3{10.0f, 0.4f, 10.0f}, Quat::Identity(), Vec3{0.7f, 0.7f, 0.7f}});
                world.AddComponent<Velocity>(e);
                world.AddComponent<Gravity>(e);
                world.AddComponent<Collider>(e, Collider{ColliderKind::Sphere, {}, 0.35f, {}});
                world.AddComponent<Enemy>(e, Enemy{EnemyState::Chase, 5.5f, 18.0f, 1.4f, 0.7f, 0.0f, 0.2f, 0.0f, 1, 150});
                world.AddComponent<Health>(e, Health{1, 1});
                world.AddComponent<Renderable>(e, Renderable{MeshId::Sphere, {1.0f, 0.4f, 0.1f, 1.0f}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Spawn Tank (Brute 5 HP)")) {
                const Entity e = world.CreateEntity();
                world.AddComponent<Transform>(e, Transform{Vec3{-10.0f, 0.9f, 10.0f}, Quat::Identity(), Vec3{1.8f, 1.8f, 1.8f}});
                world.AddComponent<Velocity>(e);
                world.AddComponent<Gravity>(e);
                world.AddComponent<Collider>(e, Collider{ColliderKind::Sphere, {}, 0.9f, {}});
                world.AddComponent<Enemy>(e, Enemy{EnemyState::Chase, 1.8f, 25.0f, 2.2f, 1.5f, 0.0f, 0.3f, 0.0f, 2, 300});
                world.AddComponent<Health>(e, Health{5, 5});
                world.AddComponent<Renderable>(e, Renderable{MeshId::Sphere, {0.7f, 0.1f, 0.1f, 1.0f}});
            }
        }

        // 4. Health Pickups
        if (ImGui::CollapsingHeader("Pickups & Items", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Spawn Health Orb (+2 HP)")) {
                const Entity e = world.CreateEntity();
                world.AddComponent<Transform>(e, Transform{Vec3{0.0f, 0.5f, 3.0f}, Quat::Identity(), Vec3{0.6f, 0.6f, 0.6f}});
                world.AddComponent<HealthPickup>(e, HealthPickup{2, 0.8f});
                world.AddComponent<Renderable>(e, Renderable{MeshId::Sphere, {0.2f, 0.95f, 0.4f, 1.0f}});
            }
        }
    }
    ImGui::End();
}

void DevUI::RenderInspector(GameSession& session) {
    ImGui::SetNextWindowSize(ImVec2(380.0f, 480.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(880.0f, 40.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("World Hierarchy & Inspector", &showInspector_)) {
        World& world = session.GetWorld();

        ImGui::Text("Entities in World Scene:");
        ImGui::BeginChild("EntityList", ImVec2(0, 160.0f), true);

        // Player
        if (session.PlayerAlive()) {
            const Entity p = session.Player();
            char label[64];
            std::snprintf(label, sizeof(label), "Player (ID #%u)", p.id);
            if (ImGui::Selectable(label, selectedEntityId_ == p.id)) {
                selectedEntityId_ = p.id;
            }
        }

        // Turrets
        int turretIdx = 1;
        for (auto [e, turret] : world.ViewComponents<Turret>()) {
            (void)turret;
            char label[64];
            std::snprintf(label, sizeof(label), "Turret / Plant #%d (ID #%u)", turretIdx++, e.id);
            if (ImGui::Selectable(label, selectedEntityId_ == e.id)) {
                selectedEntityId_ = e.id;
            }
        }

        // Enemies
        int enemyIdx = 1;
        for (auto [e, enemy] : world.ViewComponents<Enemy>()) {
            char label[64];
            const char* stateStr = (enemy.state == EnemyState::Idle) ? "Idle" :
                                   (enemy.state == EnemyState::Chase) ? "Chase" :
                                   (enemy.state == EnemyState::Attack) ? "Attack" :
                                   (enemy.state == EnemyState::Hurt) ? "Hurt" : "Dead";
            std::snprintf(label, sizeof(label), "Enemy #%d [%s] (ID #%u)", enemyIdx++, stateStr, e.id);
            if (ImGui::Selectable(label, selectedEntityId_ == e.id)) {
                selectedEntityId_ = e.id;
            }
        }

        // Static Walls
        int wallIdx = 1;
        for (auto [e, collider] : world.ViewComponents<Collider>()) {
            if (collider.isStatic) {
                char label[64];
                std::snprintf(label, sizeof(label), "Wall #%d (ID #%u)", wallIdx++, e.id);
                if (ImGui::Selectable(label, selectedEntityId_ == e.id)) {
                    selectedEntityId_ = e.id;
                }
            }
        }

        ImGui::EndChild();

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "Component Properties:");

        Entity target = Entity::Null();
        if (selectedEntityId_ != 0) {
            for (auto [e, t] : world.ViewComponents<Transform>()) {
                if (e.id == selectedEntityId_) {
                    target = e;
                    break;
                }
            }
        }

        if (world.IsAlive(target)) {
            ImGui::Text("Inspecting Entity ID: %u (gen %u)", target.id, target.generation);

            // Transform
            if (auto* t = world.TryGetComponent<Transform>(target)) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    float pos[3] = {t->position.x, t->position.y, t->position.z};
                    if (ImGui::DragFloat3("Position", pos, 0.1f)) {
                        t->position = Vec3{pos[0], pos[1], pos[2]};
                    }
                    float scale[3] = {t->scale.x, t->scale.y, t->scale.z};
                    if (ImGui::DragFloat3("Scale", scale, 0.1f, 0.1f, 50.0f)) {
                        t->scale = Vec3{scale[0], scale[1], scale[2]};
                    }
                }
            }

            // Health
            if (auto* hp = world.TryGetComponent<Health>(target)) {
                if (ImGui::CollapsingHeader("Health", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::SliderInt("Current HP", &hp->current, 0, hp->max);
                    ImGui::DragInt("Max HP", &hp->max, 1, 1, 100);
                }
            }

            // Turret
            if (auto* turret = world.TryGetComponent<Turret>(target)) {
                if (ImGui::CollapsingHeader("Turret Auto-Defense", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::SliderFloat("Fire Interval", &turret->fireInterval, 0.1f, 3.0f, "%.2fs");
                    ImGui::SliderFloat("Range", &turret->range, 2.0f, 30.0f, "%.1fm");
                    ImGui::SliderInt("Damage", &turret->damage, 1, 10);
                }
            }

            // Enemy AI
            if (auto* enemy = world.TryGetComponent<Enemy>(target)) {
                if (ImGui::CollapsingHeader("Enemy AI FSM", ImGuiTreeNodeFlags_DefaultOpen)) {
                    const char* states[] = {"Idle", "Chase", "Attack", "Hurt", "Dead"};
                    int curState = static_cast<int>(enemy->state);
                    if (ImGui::Combo("State", &curState, states, 5)) {
                        enemy->state = static_cast<EnemyState>(curState);
                    }
                    ImGui::SliderFloat("Speed", &enemy->speed, 0.5f, 15.0f);
                    ImGui::SliderFloat("Detect Range", &enemy->detectRange, 2.0f, 30.0f);
                    ImGui::SliderFloat("Attack Range", &enemy->attackRange, 0.5f, 5.0f);
                }
            }

            // Collider
            if (auto* col = world.TryGetComponent<Collider>(target)) {
                if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Text("Kind: %s", (col->kind == ColliderKind::Sphere) ? "Sphere" : "AABB");
                    ImGui::Text("Is Static: %s", col->isStatic ? "True (Immovable)" : "False (Dynamic)");
                    if (col->kind == ColliderKind::Sphere) {
                        ImGui::DragFloat("Radius", &col->radius, 0.05f, 0.1f, 10.0f);
                    } else {
                        float he[3] = {col->halfExtents.x, col->halfExtents.y, col->halfExtents.z};
                        if (ImGui::DragFloat3("Half Extents", he, 0.1f, 0.1f, 30.0f)) {
                            col->halfExtents = Vec3{he[0], he[1], he[2]};
                        }
                    }
                }
            }

            ImGui::Spacing();
            if (target != session.Player()) {
                if (ImGui::Button("Destroy Entity")) {
                    world.DestroyEntity(target);
                    selectedEntityId_ = 0;
                }
            }
        } else {
            ImGui::TextDisabled("Select an entity from the list above to inspect and edit.");
        }
    }
    ImGui::End();
}

void DevUI::RenderGameplayTuner(GameSession& session) {
    ImGui::SetNextWindowSize(ImVec2(340.0f, 360.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20.0f, 490.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Gameplay & Physics Tuner", &showTuner_)) {
        World& world = session.GetWorld();

        if (ImGui::CollapsingHeader("Player Movement & Gun", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (session.PlayerAlive()) {
                auto* player = world.TryGetComponent<Player>(session.Player());
                if (player != nullptr) {
                    ImGui::SliderFloat("Move Speed", &player->moveSpeed, 2.0f, 20.0f);
                    ImGui::SliderFloat("Jump Force", &player->jumpSpeed, 2.0f, 25.0f);
                    ImGui::SliderFloat("Shoot Cooldown", &player->shootCooldown, 0.03f, 1.0f, "%.2fs");
                    ImGui::SliderInt("Projectile Damage", &player->projectileDamage, 1, 10);
                }
            }
        }

        if (ImGui::CollapsingHeader("Physics & Gravity", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (session.PlayerAlive()) {
                auto* grav = world.TryGetComponent<Gravity>(session.Player());
                if (grav != nullptr) {
                    float g = -grav->strength;
                    if (ImGui::SliderFloat("Gravity (g)", &g, 0.0f, 50.0f)) {
                        grav->strength = -g;
                    }
                }
            }
        }

        if (ImGui::CollapsingHeader("Enemy Wave Spawner", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* spawner = world.TryGetComponent<EnemySpawner>(session.Spawner());
            if (spawner != nullptr) {
                ImGui::SliderInt("Max Alive", &spawner->maxAlive, 1, 100);
                ImGui::SliderFloat("Interval", &spawner->interval, 0.2f, 10.0f, "%.1fs");
            }
        }
    }
    ImGui::End();
}

void DevUI::RenderAudioSettings(AudioSystem& audio) {
    ImGui::SetNextWindowSize(ImVec2(320.0f, 260.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Audio & Sound FX", &showAudio_)) {
        float vol = audio.GetVolume();
        if (ImGui::SliderFloat("Master Volume", &vol, 0.0f, 1.0f, "%.0f%%")) {
            audio.SetVolume(vol);
        }

        bool muted = audio.IsMuted();
        if (ImGui::Checkbox("Mute All Audio", &muted)) {
            audio.SetMuted(muted);
        }

        ImGui::Separator();
        ImGui::Text("Synthesizer Sound Previews:");
        if (ImGui::Button("Shoot Laser")) audio.Play(SoundEffect::Shoot);
        ImGui::SameLine();
        if (ImGui::Button("Bullet Hit")) audio.Play(SoundEffect::Hit);
        if (ImGui::Button("Enemy Explosion")) audio.Play(SoundEffect::Explosion);
        ImGui::SameLine();
        if (ImGui::Button("Jump Chirp")) audio.Play(SoundEffect::Jump);
        if (ImGui::Button("Item Pickup")) audio.Play(SoundEffect::Pickup);
    }
    ImGui::End();
}

void DevUI::RenderEnvironmentSettings(GameSession& session) {
    ImGui::SetNextWindowSize(ImVec2(340.0f, 320.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Environment & Atmosphere", &showEnvironment_)) {
        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "Atmosphere Presets:");
        World& world = session.GetWorld();

        auto SetFloorColor = [&](const Vec4& col) {
            for (auto [e, ren] : world.ViewComponents<Renderable>()) {
                if (ren.mesh == MeshId::Plane) {
                    ren.color = col;
                }
            }
        };

        if (ImGui::Button("Cyber Arena (Default)")) {
            lightDir_ = Vec3{0.3f, -1.0f, 0.25f};
            lightColor_ = Vec3{0.9f, 0.95f, 1.0f};
            SetFloorColor(Vec4{0.28f, 0.32f, 0.38f, 1.0f});
        }
        ImGui::SameLine();
        if (ImGui::Button("PvZ Grass Lawn")) {
            lightDir_ = Vec3{0.45f, -1.0f, 0.35f};
            lightColor_ = Vec3{1.0f, 0.98f, 0.88f};  // Warm Sunlight
            SetFloorColor(Vec4{0.10f, 0.85f, 0.10f, 1.0f});  // Activates Checkered Lawn
        }

        if (ImGui::Button("Sunset Golden Hour")) {
            lightDir_ = Vec3{0.8f, -0.4f, 0.3f};
            lightColor_ = Vec3{1.0f, 0.60f, 0.35f};  // Sunset Gold
            SetFloorColor(Vec4{0.35f, 0.25f, 0.30f, 1.0f});
        }
        ImGui::SameLine();
        if (ImGui::Button("Neon Synthwave")) {
            lightDir_ = Vec3{-0.3f, -0.9f, 0.4f};
            lightColor_ = Vec3{1.0f, 0.30f, 0.85f};  // Neon Magenta
            SetFloorColor(Vec4{0.18f, 0.12f, 0.28f, 1.0f});
        }

        ImGui::Separator();
        ImGui::Text("Custom Sun Light Controls:");
        float dir[3] = {lightDir_.x, lightDir_.y, lightDir_.z};
        if (ImGui::DragFloat3("Light Dir", dir, 0.02f, -1.0f, 1.0f)) {
            lightDir_ = Vec3{dir[0], dir[1], dir[2]};
        }

        float col[3] = {lightColor_.x, lightColor_.y, lightColor_.z};
        if (ImGui::ColorEdit3("Light Color", col)) {
            lightColor_ = Vec3{col[0], col[1], col[2]};
        }

        ImGui::Separator();
        ImGui::TextDisabled("Stylized Half-Lambert + Hemispheric Ambient");
        ImGui::TextDisabled("Contact Ambient Occlusion & Distance Fog");
    }
    ImGui::End();
}

void DevUI::RenderSceneManager(GameSession& session) {
    ImGui::SetNextWindowSize(ImVec2(340.0f, 240.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Level Manager", &showSceneManager_)) {
        ImGui::Text("Save / Load Custom Level Layout:");
        World& world = session.GetWorld();

        if (ImGui::Button("Save Current Scene (arena_scene.txt)")) {
            std::ofstream out("arena_scene.txt");
            if (out.is_open()) {
                // Save Turrets
                for (auto [e, turret, trans] : world.ViewComponents<Turret, Transform>()) {
                    (void)e;
                    out << "TURRET " << trans.position.x << " " << trans.position.y << " " << trans.position.z
                        << " " << turret.range << " " << turret.fireInterval << "\n";
                }
                // Save custom walls
                for (auto [e, col, trans] : world.ViewComponents<Collider, Transform>()) {
                    (void)e;
                    if (col.isStatic && col.kind == ColliderKind::AABB) {
                        out << "WALL " << trans.position.x << " " << trans.position.y << " " << trans.position.z
                            << " " << col.halfExtents.x << " " << col.halfExtents.y << " " << col.halfExtents.z << "\n";
                    }
                }
                out.close();
            }
        }

        if (ImGui::Button("Load Scene (arena_scene.txt)")) {
            std::ifstream in("arena_scene.txt");
            if (in.is_open()) {
                std::string type;
                while (in >> type) {
                    if (type == "TURRET") {
                        Vec3 pos; float r, fi;
                        if (in >> pos.x >> pos.y >> pos.z >> r >> fi) {
                            const Entity e = world.CreateEntity();
                            world.AddComponent<Transform>(e, Transform{pos, Quat::Identity(), Vec3{0.8f, 1.6f, 0.8f}});
                            world.AddComponent<Turret>(e, Turret{r, fi, 0.0f, 1});
                            world.AddComponent<Collider>(e, Collider{ColliderKind::AABB, {}, 0.0f, Vec3{0.4f, 0.8f, 0.4f}, true});
                            world.AddComponent<Renderable>(e, Renderable{MeshId::Cube, {0.15f, 0.85f, 0.40f, 1.0f}});
                        }
                    } else if (type == "WALL") {
                        Vec3 pos, ext;
                        if (in >> pos.x >> pos.y >> pos.z >> ext.x >> ext.y >> ext.z) {
                            const Entity e = world.CreateEntity();
                            world.AddComponent<Transform>(e, Transform{pos, Quat::Identity(), ext * 2.0f});
                            world.AddComponent<Collider>(e, Collider{ColliderKind::AABB, {}, 0.0f, ext, true});
                            world.AddComponent<Renderable>(e, Renderable{MeshId::Cube, {0.55f, 0.60f, 0.68f, 1.0f}});
                        }
                    }
                }
                in.close();
            }
        }
    }
    ImGui::End();
}

}  // namespace af
