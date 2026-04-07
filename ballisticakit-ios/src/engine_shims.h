// Shim layer providing minimal engine globals for standalone iOS rendering.
// This lets us compile the real RendererGL without pulling in the full engine.

#ifndef ENGINE_SHIMS_H_
#define ENGINE_SHIMS_H_

namespace ballistica {
namespace base {
class GraphicsServer;
class RendererGL;
class RenderPass;
}  // namespace base
}  // namespace ballistica

// Call once at startup to initialize engine globals (g_base, g_core, etc.)
void InitEngineShims();

// Get shim instances for direct use.
auto GetShimGraphicsServer() -> ballistica::base::GraphicsServer*;

// Update screen resolution in the shim Graphics object.
void ShimSetScreenResolution(float w, float h);

// Set camera on a RenderPass's private cam fields (from shim code in base namespace).
void ShimSetPassCamera(ballistica::base::RenderPass* pass,
                       float px, float py, float pz,
                       float tx, float ty, float tz,
                       float ux, float uy, float uz);

#endif  // ENGINE_SHIMS_H_
