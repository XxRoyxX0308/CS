#ifndef RENDER_SHADOW_MAP_HPP
#define RENDER_SHADOW_MAP_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/Framebuffer.hpp"
#include "Core/Program.hpp"
#include "Scene/Light.hpp"

namespace Render {

/**
 * @brief Manages shadow mapping for a directional light.
 *
 * Creates a depth-only FBO and provides the light-space matrix
 * for shadow sampling in the main rendering pass.
 *
 * @code
 * Render::ShadowMap shadow(2048);
 * shadow.BeginShadowPass(dirLight);
 * // render scene depth to shadow map
 * shadow.EndShadowPass();
 *
 * // In main pass:
 * glm::mat4 lsm = shadow.GetLightSpaceMatrix();
 * shadow.BindShadowTexture(shadowSlot);
 * @endcode
 */
class ShadowMap {
public:
    /**
     * @brief Create a shadow map with given resolution.
     * @param resolution Shadow map texture size (square).
     */
    explicit ShadowMap(int resolution = 2048);

    /**
     * @brief Begin the shadow pass.
     *
     * Binds the shadow FBO and sets up the light-space matrix.
     * @param dirLight The directional light to cast shadows from.
     * @param sceneRadius Approximate scene bounding radius for ortho size.
     */
    void BeginShadowPass(const Scene::DirectionalLight &dirLight,
                         float sceneRadius = 30.0f);

    /**
     * @brief End the shadow pass and restore the default framebuffer.
     */
    void EndShadowPass();

    /**
     * @brief Bind the shadow depth texture to a texture slot.
     * @param slot The texture unit to bind to.
     */
    void BindShadowTexture(int slot) const;

    const glm::mat4 &GetLightSpaceMatrix() const { return m_LightSpaceMatrix; }
    const Core::Program &GetShadowProgram() const { return *m_ShadowProgram; }

private:
    Core3D::Framebuffer m_ShadowFBO;
    std::unique_ptr<Core::Program> m_ShadowProgram;
    glm::mat4 m_LightSpaceMatrix{1.0f};
    int m_Resolution;
    int m_PrevViewport[4] = {0}; ///< Saved viewport for restoration
};

} // namespace Render

#endif
