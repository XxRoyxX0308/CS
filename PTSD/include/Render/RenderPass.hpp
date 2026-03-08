#ifndef RENDER_RENDER_PASS_HPP
#define RENDER_RENDER_PASS_HPP

namespace Render {

/**
 * @brief Identifies the current rendering pass.
 */
enum class RenderPass {
    SHADOW,   ///< Depth-only pass from light's perspective
    GEOMETRY, ///< Main geometry pass with lighting
    SKYBOX    ///< Skybox pass (drawn last)
};

} // namespace Render

#endif
