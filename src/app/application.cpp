#include "af/app/Application.h"

#include <cstdio>

namespace af {

Application::Application(const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "AstraForge: SDL_Init failed: %s\n", SDL_GetError());
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width, height,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) {
        std::fprintf(stderr, "AstraForge: SDL_CreateWindow failed: %s\n",
                     SDL_GetError());
        return;
    }

    glContext_ = SDL_GL_CreateContext(window_);
    if (glContext_ == nullptr) {
        std::fprintf(stderr, "AstraForge: SDL_GL_CreateContext failed: %s\n",
                     SDL_GetError());
        return;
    }

    SDL_GL_SetSwapInterval(1);  // vsync if supported; ignore failure
    SDL_SetRelativeMouseMode(SDL_TRUE);

    lastFrameTicks_ = SDL_GetTicks64();
}

Application::~Application() {
    if (glContext_ != nullptr) SDL_GL_DeleteContext(glContext_);
    if (window_ != nullptr) SDL_DestroyWindow(window_);
    SDL_Quit();
}

void Application::PumpEvents() {
    PumpEvents([](const SDL_Event&) { return false; });
}

void Application::ProcessEventInternal(const SDL_Event& event) {
    switch (event.type) {
        case SDL_QUIT:
            input_.quitRequested = true;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (event.key.repeat == 0) {
                const bool down = event.type == SDL_KEYDOWN;
                const SDL_Scancode code = event.key.keysym.scancode;
                if (code < SDL_NUM_SCANCODES) input_.keys[code] = down;
            }
            break;
        case SDL_MOUSEMOTION:
            input_.mouseDx += event.motion.xrel;
            input_.mouseDy += event.motion.yrel;
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            const bool down = event.type == SDL_MOUSEBUTTONDOWN;
            if (event.button.button > 0 && event.button.button < 8) {
                input_.mouseButtons[event.button.button] = down;
            }
            break;
        }
        case SDL_MOUSEWHEEL:
            input_.wheelY += event.wheel.y;
            break;
        case SDL_WINDOWEVENT:
            // Handled by main via SDL_GetWindowSize on resize (see main.cpp).
            break;
        default:
            break;
    }
}

void Application::Present() { SDL_GL_SwapWindow(window_); }

}  // namespace af
