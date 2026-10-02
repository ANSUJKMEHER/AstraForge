# AstraForge — lightweight C++20 3D game engine + arena prototype
# Build: cmake -B build && cmake --build build
# CMake 3.16+ required. See README.md, context.md, docs/.

cmake_minimum_required(VERSION 3.16)
project(AstraForge VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Options ---------------------------------------------------------------
option(ASTRAFORGE_BUILD_TESTS      "Build unit tests"          ON)
option(ASTRAFORGE_BUILD_APP        "Build the SDL2/OpenGL app" ON)
option(ASTRAFORGE_BUILD_HEADLESS   "Build headless sim runner" ON)
option(ASTRAFORGE_BUILD_BENCHMARKS "Build benchmark binaries"  ON)
option(ASTRAFORGE_SANITIZE        "Enable ASan+UBSan (GCC/Clang only)" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  # Single-config generators (Makefiles/Ninja): default to an optimized,
  # debug-friendly build.
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

if(ASTRAFORGE_SANITIZE)
  if(MSVC)
    message(WARNING "ASTRAFORGE_SANITIZE is not supported on MSVC; ignoring")
  else()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
  endif()
endif()

# --- Engine core library (pure C++, no platform dependencies) --------------
# This library contains everything testable headless: math, ECS, collision,
# physics, resources, profiling. The SDL2/OpenGL application links against it.
add_library(af_core STATIC
  src/core/version.cpp
  src/core/ecs/entity_manager.cpp
  src/core/ecs/world.cpp
  src/core/collision/spatial_hash_grid.cpp
  src/core/collision/collision_system.cpp
  src/core/physics/physics_system.cpp
  src/core/resources/mesh_gen.cpp
  src/core/resources/image_loader.cpp
  src/core/resources/default_shaders.cpp
  src/core/frustum.cpp
  src/core/camera.cpp
  src/core/profiling.cpp
)
target_include_directories(af_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
)

if(MSVC)
  target_compile_options(af_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(af_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Game library (arena gameplay, pure C++ — headless-testable) -----------
# Player/enemy/projectile/score/spawner systems + the GameSession orchestrator
# + the pooled projectiles. Depends on af_core only.
add_library(af_game STATIC
  src/game/projectile_pool.cpp
  src/game/player_controller_system.cpp
  src/game/enemy_ai_system.cpp
  src/game/projectile_system.cpp
  src/game/combat_system.cpp
  src/game/score_system.cpp
  src/game/spawner_system.cpp
  src/game/game_session.cpp
)
target_include_directories(af_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(af_game PUBLIC af_core)

if(MSVC)
  target_compile_options(af_game PRIVATE /W4 /permissive-)
else()
  target_compile_options(af_game PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Unit tests ------------------------------------------------------------
if(ASTRAFORGE_BUILD_TESTS)
  enable_testing()
  add_executable(af_tests
    tests/main.cpp
    tests/framework/allocation_probe.cpp
    tests/core/test_version.cpp
    tests/math/test_vec.cpp
    tests/math/test_mat.cpp
    tests/math/test_quat.cpp
    tests/math/test_transform.cpp
    tests/ecs/test_entity.cpp
    tests/ecs/test_sparse_set.cpp
    tests/ecs/test_world.cpp
    tests/ecs/test_view.cpp
    tests/ecs/test_systems.cpp
    tests/ecs/test_determinism.cpp
    tests/ecs/test_allocation.cpp
    tests/collision/test_shapes.cpp
    tests/collision/test_grid.cpp
    tests/collision/test_collision_system.cpp
    tests/physics/test_physics.cpp
    tests/resources/test_mesh.cpp
    tests/resources/test_image.cpp
    tests/resources/test_resource_cache.cpp
    tests/resources/test_shaders.cpp
    tests/core/test_frustum_camera.cpp
    tests/core/test_profiling.cpp
    tests/game/test_projectile_pool.cpp
    tests/game/test_enemy_fsm.cpp
    tests/game/test_combat.cpp
    tests/game/test_player_controller.cpp
    tests/game/test_game_session.cpp
  )
  target_include_directories(af_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_link_libraries(af_tests PRIVATE af_game)
  add_test(NAME core_tests COMMAND af_tests)
endif()

# --- Headless simulation runner --------------------------------------------
if(ASTRAFORGE_BUILD_HEADLESS)
  add_executable(af_headless src/app/headless_main.cpp)
  target_link_libraries(af_headless PRIVATE af_core)

  # Full gameplay runner (arena game, scripted input, deterministic).
  add_executable(af_headless_game src/game/headless_game_main.cpp)
  target_link_libraries(af_headless_game PRIVATE af_game)

  if(ASTRAFORGE_BUILD_TESTS)
    add_test(NAME headless_smoke COMMAND af_headless 1000 120)
    add_test(NAME game_smoke COMMAND af_headless_game 600)
  endif()
endif()

# --- Benchmarks -------------------------------------------------------------
if(ASTRAFORGE_BUILD_BENCHMARKS)
  add_executable(af_bench_collision src/bench/collision_bench.cpp)
  target_link_libraries(af_bench_collision PRIVATE af_core)

  # Gameplay benchmark: stress mode + per-system timings (Phase 9).
  add_executable(af_bench_gameplay src/bench/gameplay_bench.cpp)
  target_link_libraries(af_bench_gameplay PRIVATE af_game)

  if(ASTRAFORGE_BUILD_TESTS)
    add_test(NAME collision_bench_smoke COMMAND af_bench_collision 1000 20)
    add_test(NAME gameplay_bench_smoke COMMAND af_bench_gameplay 200 30)
  endif()
endif()

# --- SDL2/OpenGL application -----------------------------------------------
if(ASTRAFORGE_BUILD_APP)
  find_package(SDL2 QUIET)
  if(SDL2_FOUND)
    add_executable(astraforge_arena
      src/app/main.cpp
      src/app/application.cpp
      src/app/gl_load.cpp
      src/app/shader_program.cpp
      src/app/gl_mesh.cpp
      src/app/renderer.cpp
      src/app/render_system.cpp
      src/app/dev_ui.cpp
      src/app/audio_system.cpp
      src/app/particle_system.cpp
      third_party/imgui/imgui.cpp
      third_party/imgui/imgui_draw.cpp
      third_party/imgui/imgui_tables.cpp
      third_party/imgui/imgui_widgets.cpp
      third_party/imgui/backends/imgui_impl_sdl2.cpp
      third_party/imgui/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(astraforge_arena PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
      ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
    )
    if(MINGW)
      target_link_libraries(astraforge_arena PRIVATE mingw32)
    endif()
    target_link_libraries(astraforge_arena PRIVATE af_game SDL2::SDL2main SDL2::SDL2)
    if(WIN32)
      # SDL2 does not forward OpenGL link requirements on Windows.
      target_link_libraries(astraforge_arena PRIVATE opengl32)
    endif()
    if(MSVC)
      target_compile_options(astraforge_arena PRIVATE /W4 /permissive-)
    else()
      target_compile_options(astraforge_arena PRIVATE -Wall -Wextra -Wpedantic)
    endif()
  else()
    message(WARNING "SDL2 not found — astraforge_arena not built. See docs/build.md for Windows/Linux setup.")
  endif()
endif()

# --- App / headless runner / benchmarks -------------------------------------
# Wired up in later phases:
#   Phase 2 — headless simulation runner (ASTRAFORGE_BUILD_HEADLESS)
#   Phase 3 — benchmark binaries (ASTRAFORGE_BUILD_BENCHMARKS)
#   Phase 6 — SDL2/OpenGL application (ASTRAFORGE_BUILD_APP, find_package(SDL2))
