// Resolves the af::gl entry points via SDL_GL_GetProcAddress (which falls
// back to opengl32.dll exports on Windows for legacy functions). Must be
// called once, after an SDL GL context is current.

#include "af/render/GL.h"

#include <cstdio>

#include <SDL.h>

namespace af::gl {

namespace {

void* Resolve(const char* name) { return SDL_GL_GetProcAddress(name); }

}  // namespace

#define AF_GL_DEFINE(ret, name, args) ret (*name) args = nullptr;
AF_GL_ENTRY_POINTS(AF_GL_DEFINE)
#undef AF_GL_DEFINE

#define AF_GL_RESOLVE(ret, name, args)                                   \
    do {                                                                 \
        name = reinterpret_cast<decltype(name)>(Resolve("gl" #name));    \
        if (name == nullptr) {                                           \
            std::fprintf(stderr, "AstraForge: missing GL entry point gl" #name "\n"); \
            ok = false;                                                  \
        }                                                                \
    } while (0);

bool LoadGL() {
    bool ok = true;
    AF_GL_ENTRY_POINTS(AF_GL_RESOLVE)
    return ok;
}

#undef AF_GL_RESOLVE

}  // namespace af::gl
