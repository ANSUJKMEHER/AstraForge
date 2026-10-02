#pragma once

#include <SDL.h>
#include <vector>
#include <cstdint>
#include <string>

#include "af/math/Vec3.h"

namespace af {

class GameSession;
class Renderer;
class Camera;
class AudioSystem;

class DevUI {
public:
    DevUI();
    ~DevUI();

    DevUI(const DevUI&) = delete;
    DevUI& operator=(const DevUI&) = delete;

    bool Init(SDL_Window* window, void* glContext);
    void Shutdown();

    // Returns true if event is consumed by ImGui / DevUI
    bool ProcessEvent(const SDL_Event& event);

    void BeginFrame();
    void Render(GameSession& session, Renderer& renderer, Camera& camera, AudioSystem& audio, float dt);
    void EndFrame();

    // Editor / Mouse Mode
    bool IsEditorMode() const { return editorMode_; }
    void SetEditorMode(bool active);
    void ToggleEditorMode();

    // Simulation controls
    bool IsPaused() const { return paused_; }
    bool ShouldStepSingleFrame() {
        if (stepSingleFrame_) {
            stepSingleFrame_ = false;
            return true;
        }
        return false;
    }
    float TimeScale() const { return timeScale_; }

    // Visual / Gameplay debug features
    bool GodMode() const { return godMode_; }
    bool WireframeMode() const { return wireframeMode_; }

    // Lighting parameters
    Vec3 LightDirection() const { return lightDir_; }
    Vec3 LightColor() const { return lightColor_; }

    // Screen Shake
    float ScreenShake() const { return screenShake_; }
    void TriggerScreenShake(float intensity);

private:
    void SetupTheme();
    void RenderMenuBar(GameSession& session, AudioSystem& audio);
    void RenderPerformanceOverlay(Renderer& renderer, float dt);
    void RenderInspector(GameSession& session);
    void RenderGameplayTuner(GameSession& session);
    void RenderObjectSpawner(GameSession& session);
    void RenderEnvironmentSettings(GameSession& session);
    void RenderAudioSettings(AudioSystem& audio);
    void RenderSceneManager(GameSession& session);
    void RenderCombatHUD(GameSession& session);

    SDL_Window* window_ = nullptr;
    bool initialized_ = false;
    bool editorMode_ = true;  // Start with Dev Editor visible

    // Simulation states
    bool paused_ = false;
    bool stepSingleFrame_ = false;
    float timeScale_ = 1.0f;
    bool godMode_ = false;
    bool wireframeMode_ = false;
    float screenShake_ = 0.0f;

    // Lighting controls
    Vec3 lightDir_{0.3f, -1.0f, 0.25f};
    Vec3 lightColor_{1.0f, 0.95f, 0.85f};

    // Window visibility toggles
    bool showProfiler_ = true;
    bool showInspector_ = true;
    bool showTuner_ = true;
    bool showSpawner_ = true;
    bool showEnvironment_ = false;
    bool showAudio_ = false;
    bool showSceneManager_ = false;
    bool showHUD_ = true;

    // Entity selection
    uint32_t selectedEntityId_ = 0;

    // Frame timing history for plot
    static constexpr int HistoryLength = 120;
    float frameTimeHistory_[HistoryLength] = {};
    int historyIndex_ = 0;
};

}  // namespace af
