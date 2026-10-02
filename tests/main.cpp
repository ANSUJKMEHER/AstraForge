// AstraForge test runner. All AF_TEST cases registered across translation
// units run here; the exit code is non-zero if any check fails.

#include "framework/af_test.hpp"

int main() {
    return ::af::test::RunAll();
}
