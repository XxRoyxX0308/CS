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
 * Performance optimizations:
 * - Frustum culling (AABB-based)
 * - Uniform location caching
 * - Pre-allocated render queues
 * - Material sorting to reduce state changes
 *
 * Pipeline stages:
 * 1. **Frustum Culling**: Collect visible nodes
 * 2. **Shadow Pass**: Render depth from directional light POV
 * 3. **Geometry Pass**: Render all visible SceneNodes with Phong/PBR
 * 4. **Skybox Pass**: Draw skybox (depth = GL_LEQUAL)
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
    ~ForwardRenderer();

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

    /**
     * @brief Enable or disable material sorting.
     */
    void SetMaterialSortingEnabled(bool enabled) {
        m_MaterialSortingEnabled = enabled;
    }

    /**
     * @brief Get rendering statistics.
     */
    struct RenderStats {
        size_t totalNodes = 0;
        size_t visibleNodes = 0;
        size_t culledNodes = 0;
        size_t drawCalls = 0;
    };
    const RenderStats &GetStats() const { return m_Stats; }

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
    void SetDefaultMaterialUniforms(Core::Program &program);

    std::unique_ptr<Core::Program> m_PhongProgram;
    std::unique_ptr<Core::Program> m_PBRProgram;

    ShadowMap m_ShadowMap;
    bool m_ShadowsEnabled;
    bool m_UsePBR;
    bool m_FrustumCullingEnabled = true;
    bool m_MaterialSortingEnabled = true;
    float m_SceneRadius = 30.0f;

    GLuint m_MatricesUBO = 0;
    GLuint m_LightsUBO = 0;

    // Cached uniform locations for Phong shader
    struct PhongLocations {
        GLint lightSpaceMatrix = -1;
        GLint shadowEnabled = -1;
        GLint material_diffuse = -1;
        GLint material_specular = -1;
        GLint material_ambient = -1;
        GLint material_shininess = -1;
        GLint hasDiffuseMap = -1;
        GLint hasSpecularMap = -1;
        GLint hasNormalMap = -1;
    } m_PhongLoc;

    // Cached uniform locations for PBR shader
    struct PBRLocations {
        GLint lightSpaceMatrix = -1;
        GLint shadowEnabled = -1;
        GLint pbr_albedo = -1;
        GLint pbr_metallic = -1;
        GLint pbr_roughness = -1;
        GLint pbr_ao = -1;
        GLint hasAlbedoMap = -1;
        GLint hasNormalMap = -1;
        GLint hasMetallicMap = -1;
        GLint hasRoughnessMap = -1;
        GLint hasAOMap = -1;
    } m_PBRLoc;

    // Cached uniform location for shadow shader
    GLint m_ShadowModelLoc = -1;

    RenderStats m_Stats;
};

} // namespace Render

#endif
