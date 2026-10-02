#pragma once
// RenderSystem — walks Transform+Renderable entities and submits them to the
// Renderer with frustum culling. Runs at RENDER rate (every presented frame),
// not inside the fixed-timestep SystemManager — rendering is variable-rate by
// design (context.md Decision 4).

#include "af/core/Components.h"
#include "af/ecs/World.h"
#include "af/render/Renderer.h"

namespace af {

// World-space culling sphere from local mesh bounds + transform.
// (Collider-aware bounds arrive with gameplay in Phase 8.)
void RenderScene(World& world, Renderer& renderer, const Camera& camera);

}  // namespace af
