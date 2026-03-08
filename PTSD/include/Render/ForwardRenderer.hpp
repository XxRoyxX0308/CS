#ifndef RENDER_FORWARD_RENDERER_HPP
#define RENDER_FORWARD_RENDERER_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core/Program.hpp"
#include "Render/ShadowMap.hpp"
#include "Scene/SceneGraph.hpp"

namespace Render {

/**
 * @brief Forward rendering pipeline for 3D scenes.
 *
 * Supports Blinn-Phong and PBR lighting, shadow mapping, skybox,
 * and skeletal animation. Integrates with the existing 2D system
 * (draw 3D first, then 2D overlay).
 *
 * Pipeline stages:
 * 1. **Shadow Pass**: Render depth from directional light POV
 * 2. **Geometry Pass**: Render all visible SceneNodes with Phong/PBR
 * 3. **Skybox Pass**: Draw skybox (depth = GL_LEQUAL)
 *
 * @code
 * Render::ForwardRenderer renderer;
 * Scene::SceneGraph scene;
 *
 * // Setup scene with camera, lights, nodes, skybox...
 *
 * // In main loop:
 * renderer.Render(scene);
 *
 * // Then draw 2D overlay with Util::Renderer if needed
 * @endcode
 */
class ForwardRenderer {
public:
    ForwardRenderer();

    /**
     * @brief Render the entire scene.
     * @param scene The scene graph to render.
     */
    void Render(Scene::SceneGraph &scene);

    /**
     * @brief Get the Phong shader program (for advanced usage).
     */
    Core::Program &GetPhongProgram() { return *m_PhongProgram; }

    /**
     * @brief Get the PBR shader program.
     */
    Core::Program &GetPBRProgram() { return *m_PBRProgram; }

    /**
     * @brief Enable or disable shadow mapping.
     */
    void SetShadowsEnabled(bool enabled) { m_ShadowsEnabled = enabled; }

    /**
     * @brief Enable or disable PBR rendering (vs Phong).
     */
    void SetPBREnabled(bool enabled) { m_UsePBR = enabled; }

    /**
     * @brief Set scene radius for shadow map frustum calculation.
     */
    void SetSceneRadius(float radius) { m_SceneRadius = radius; }

private:
    void InitPrograms();
    void InitLightsUBO();

    void ShadowPass(Scene::SceneGraph &scene);
    void GeometryPass(Scene::SceneGraph &scene);
    void SkyboxPass(Scene::SceneGraph &scene);

    void UploadMatrices(const Core::Program &program,
                        const Core3D::Matrices3D &matrices);
    void UploadLights(const Scene::LightsUBO &lightsData);

    std::unique_ptr<Core::Program> m_PhongProgram;
    std::unique_ptr<Core::Program> m_PBRProgram;

    ShadowMap m_ShadowMap;
    bool m_ShadowsEnabled;
    bool m_UsePBR;
    float m_SceneRadius = 30.0f;

    GLuint m_MatricesUBO = 0;
    GLuint m_LightsUBO = 0;
};

} // namespace Render

#endif
