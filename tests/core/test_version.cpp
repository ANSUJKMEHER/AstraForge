#include <cstring>

#include "af/core/Version.h"
#include "framework/af_test.hpp"

AF_TEST("engine version is set and matches CMake project version") {
    const char* v = af::EngineVersion();
    AF_CHECK(v != nullptr);
    AF_CHECK_EQ(std::strcmp(v, "0.2.0"), 0);
}
