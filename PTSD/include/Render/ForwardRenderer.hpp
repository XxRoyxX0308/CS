#ifndef RENDER_FORWARD_RENDERER_HPP
#define RENDER_FORWARD_RENDERER_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core/Program.hpp"
#include "Core3D/Frustum.hpp"
#include "Render/ShadowMap.hpp"
#include "Scene/SceneGraph.hpp"

namespace Render {

/**
 * @brief Rendering statistics for performance monitoring.
 */
struct RenderStats {
    uint32_t totalNodes = 0;          ///< Total nodes in scene
    uint32_t visibleNodes = 0;        ///< Nodes passing frustum culling
    uint32_t culledNodes = 0;         ///< Nodes rejected by frustum culling
    uint32_t drawCalls = 0;           ///< Number of draw calls issued
    float frustumCullTimeMs = 0.0f;   ///< Time spent on frustum culling
    float renderTimeMs = 0.0f;        ///< Total render time
};

/**
 * @brief Forward rendering pipeline for 3D scenes.
 *
 * Supports Blinn-Phong and PBR lighting, shadow mapping, skybox,
 * and skeletal animation. Integrates with the existing 2D system
 * (draw 3D first, then 2D overlay).
 *
 * **Performance optimizations**:
 * - Frustum culling (rejects objects outside camera view)
 * - Transform caching (uses SceneNode cached matrices)
 * - Pre-allocated node buffer (avoids per-frame allocation)
 * - Uniform location caching
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
 * // Check performance:
 * const auto& stats = renderer.GetLastFrameStats();
 * printf("Visible: %d/%d\n", stats.visibleNodes, stats.totalNodes);
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

    /**
     * @brief Enable or disable frustum culling.
     */
    void SetFrustumCullingEnabled(bool enabled) {
        m_FrustumCullingEnabled = enabled;
    }
    bool IsFrustumCullingEnabled() const { return m_FrustumCullingEnabled; }

    /**
     * @brief Get rendering statistics from the last frame.
     */
    const RenderStats &GetLastFrameStats() const { return m_Stats; }

private:
    void InitPrograms();
    void InitLightsUBO();
    void CacheUniformLocations();

    void ShadowPass(Scene::SceneGraph &scene);
    void GeometryPass(Scene::SceneGraph &scene);
    void SkyboxPass(Scene::SceneGraph &scene);

    void UploadMatrices(const Core::Program &program,
                        const Core3D::Matrices3D &matrices);
    void UploadLights(const Scene::LightsUBO &lightsData);

    /**
     * @brief Perform frustum culling on scene nodes.
     * @param scene The scene to cull.
     * @param outVisibleNodes Output vector of visible nodes.
     */
    void FrustumCull(Scene::SceneGraph &scene,
                     std::vector<Scene::SceneNode *> &outVisibleNodes);

    std::unique_ptr<Core::Program> m_PhongProgram;
    std::unique_ptr<Core::Program> m_PBRProgram;

    ShadowMap m_ShadowMap;
    bool m_ShadowsEnabled;
    bool m_UsePBR;
    float m_SceneRadius = 30.0f;

    // Frustum culling
    bool m_FrustumCullingEnabled = true;
    Core3D::Frustum m_Frustum;

    GLuint m_MatricesUBO = 0;
    GLuint m_LightsUBO = 0;

    // Cached uniform locations (to avoid glGetUniformLocation per frame)
    struct UniformLocations {
        GLint lightSpaceMatrix = -1;
        GLint shadowEnabled = -1;
        GLint model = -1;
        // Phong material
        GLint materialDiffuse = -1;
        GLint materialSpecular = -1;
        GLint materialAmbient = -1;
        GLint materialShininess = -1;
        GLint hasDiffuseMap = -1;
        GLint hasSpecularMap = -1;
        GLint hasNormalMap = -1;
        // PBR material
        GLint pbrAlbedo = -1;
        GLint pbrMetallic = -1;
        GLint pbrRoughness = -1;
        GLint pbrAO = -1;
        GLint hasAlbedoMap = -1;
        GLint hasMetallicMap = -1;
        GLint hasRoughnessMap = -1;
        GLint hasAOMap = -1;
        GLint pbrHasNormalMap = -1;
    };
    UniformLocations m_PhongUniforms;
    UniformLocations m_PBRUniforms;
    GLint m_ShadowModelLoc = -1;

    // Pre-allocated buffers to avoid per-frame allocation
    std::vector<Scene::SceneNode *> m_VisibleNodes;
    std::vector<std::shared_ptr<Scene::SceneNode>> m_FlattenBuffer;

    // Statistics
    RenderStats m_Stats;
};

} // namespace Render

#endif
