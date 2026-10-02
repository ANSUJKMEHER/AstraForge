#pragma once
// Application — SDL2 window + GL 3.3 core context + input + fixed-step driver.
// The game loop lives in main.cpp: PumpEvents → RunFixedSteps (60 Hz sim) →
// render → Present. Render rate is free; simulation is fixed (context.md
// Decision 4).

#include <SDL.h>

namespace af {

struct InputState {
    // Currently-held keys, indexed by SDL scancode (SDL_NUM_SCANCODES).
    bool keys[SDL_NUM_SCANCODES] = {};
    // Currently-held mouse buttons, indexed by SDL_BUTTON_* (1 = left).
    bool mouseButtons[8] = {};
    int mouseDx = 0;   // accumulated relative motion since last PumpEvents
    int mouseDy = 0;
    int wheelY = 0;    // accumulated scroll since last PumpEvents
    bool quitRequested = false;
};

class Application {
public:
    Application(const char* title, int width, int height);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool IsValid() const { return window_ != nullptr; }
    bool ShouldQuit() const { return input_.quitRequested; }
    SDL_Window* windowHandle() const { return window_; }
    SDL_GLContext glContext() const { return glContext_; }

    // Polls SDL events into InputState, optionally letting a filter intercept events.
    template <typename FilterFn>
    void PumpEvents(FilterFn&& filter) {
        input_.mouseDx = 0;
        input_.mouseDy = 0;
        input_.wheelY = 0;

        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (filter(event)) continue;
            ProcessEventInternal(event);
        }
    }

    void PumpEvents();

    void Present();  // swap buffers (vsync when the driver honors it)

    // Fixed-timestep driver: runs sim(dt) zero or more times to catch up.
    // frameTime is clamped (spiral-of-death guard, 250 ms).
    template <typename Fn>
    void RunFixedSteps(Fn&& sim, double dt) {
        const uint64_t now = SDL_GetTicks64();
        const double frameSeconds =
            static_cast<double>(now - lastFrameTicks_) / 1000.0;
        lastFrameTicks_ = now;
        double frameTime = frameSeconds;
        if (frameTime > 0.25) frameTime = 0.25;  // clamp huge hitches
        accumulator_ += frameTime;
        while (accumulator_ >= dt) {
            sim(static_cast<float>(dt));
            accumulator_ -= dt;
        }
    }

    const InputState& GetInput() const { return input_; }

private:
    void ProcessEventInternal(const SDL_Event& event);

    SDL_Window* window_ = nullptr;
    SDL_GLContext glContext_ = nullptr;
    InputState input_{};
    uint64_t lastFrameTicks_ = 0;
    double accumulator_ = 0.0;
};

}  // namespace af
