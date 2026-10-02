#pragma once
// Default shader sources, embedded as C++ strings so the engine has a
// working render path with zero file I/O (no asset path assumptions on the
// student laptop). The renderer (Phase 6) compiles these.

namespace af {

const char* DefaultVertexShader();
const char* DefaultFragmentShader();

}  // namespace af
