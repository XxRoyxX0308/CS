#ifndef SCENE_SCENE_GRAPH_HPP
#define SCENE_SCENE_GRAPH_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/Camera.hpp"
#include "Core3D/Frustum.hpp"
#include "Scene/SceneNode.hpp"
#include "Scene/Light.hpp"

namespace Scene {

// Forward declaration
class Skybox;

/**
 * @brief The root container for a 3D scene.
 *
 * Manages the scene tree, camera, lights, and skybox.
 * Includes frustum culling optimization.
 * Pass this to ForwardRenderer to draw the entire scene.
 *
 * @code
 * Scene::SceneGraph scene;
 *
 * // Camera
 * scene.GetCamera().SetPosition(glm::vec3(0, 5, 10));
 *
 * // Lights
 * Scene::DirectionalLight sun;
 * sun.direction = glm::normalize(glm::vec3(-1, -1, -1));
 * scene.SetDirectionalLight(sun);
 *
 * Scene::PointLight lamp;
 * lamp.position = glm::vec3(3, 2, 0);
 * scene.AddPointLight(lamp);
 *
 * // Nodes
 * auto node = std::make_shared<Scene::SceneNode>();
 * node->SetDrawable(model);
 * scene.AddNode(node);
 *
 * // Skybox
 * scene.SetSkybox(mySkybox);
 * @endcode
 */
class SceneGraph {
public:
    SceneGraph();

    // ====== Node Management ======

    void AddNode(const std::shared_ptr<SceneNode> &node);
    void RemoveNode(const std::shared_ptr<SceneNode> &node);
    const std::shared_ptr<SceneNode> &GetRoot() const { return m_Root; }

    /**
     * @brief Flatten the scene tree into a linear list for rendering.
     *
     * Uses a pre-allocated internal buffer to avoid per-frame allocations.
     * Returns a const reference that is valid until the next call.
     */
    const std::vector<std::shared_ptr<SceneNode>> &FlattenTree();

    /**
     * @brief Flatten the scene tree while applying frustum culling.
     *
     * Only returns nodes whose world AABB is visible in the frustum.
     * @param frustum The view frustum to cull against.
     */
    const std::vector<std::shared_ptr<SceneNode>> &FlattenTreeCulled(
        const Core3D::Frustum &frustum);

    /**
     * @brief Get rendering statistics from the last FlattenTreeCulled call.
     */
    struct CullStats {
        size_t totalNodes = 0;
        size_t visibleNodes = 0;
        size_t culledNodes = 0;
    };
    const CullStats &GetCullStats() const { return m_CullStats; }

    // ====== Camera ======

    Core3D::Camera &GetCamera() { return m_Camera; }
    const Core3D::Camera &GetCamera() const { return m_Camera; }
    void SetCamera(const Core3D::Camera &camera) { m_Camera = camera; }

    // ====== Frustum ======

    /**
     * @brief Update the internal frustum from the current camera.
     *
     * Call this at the start of each frame before GetVisibleNodes().
     */
    void UpdateFrustum();

    /**
     * @brief Get the current view frustum.
     */
    const Core3D::Frustum &GetFrustum() const { return m_Frustum; }

    /**
     * @brief Enable or disable frustum culling.
     */
    void SetFrustumCullingEnabled(bool enabled) {
        m_FrustumCullingEnabled = enabled;
    }
    bool IsFrustumCullingEnabled() const { return m_FrustumCullingEnabled; }

    // ====== Lights ======

    void SetDirectionalLight(const DirectionalLight &light) {
        m_DirLight = light;
        m_HasDirLight = true;
    }
    void AddPointLight(const PointLight &light) {
        m_PointLights.push_back(light);
    }
    void AddSpotLight(const SpotLight &light) {
        m_SpotLights.push_back(light);
    }
    void ClearPointLights() { m_PointLights.clear(); }
    void ClearSpotLights() { m_SpotLights.clear(); }

    bool HasDirectionalLight() const { return m_HasDirLight; }
    DirectionalLight &GetDirectionalLight() { return m_DirLight; }
    const DirectionalLight &GetDirectionalLight() const { return m_DirLight; }
    std::vector<PointLight> &GetPointLights() { return m_PointLights; }
    const std::vector<PointLight> &GetPointLights() const {
        return m_PointLights;
    }
    std::vector<SpotLight> &GetSpotLights() { return m_SpotLights; }
    const std::vector<SpotLight> &GetSpotLights() const {
        return m_SpotLights;
    }

    // ====== Skybox ======

    void SetSkybox(const std::shared_ptr<Skybox> &skybox) {
        m_Skybox = skybox;
    }
    const std::shared_ptr<Skybox> &GetSkybox() const { return m_Skybox; }

    /**
     * @brief Build the LightsUBO data from current scene lights.
     */
    LightsUBO BuildLightsUBO() const;

private:
    void FlattenNode(const std::shared_ptr<SceneNode> &node);
    void FlattenNodeCulled(const std::shared_ptr<SceneNode> &node,
                           const Core3D::Frustum &frustum);

    std::shared_ptr<SceneNode> m_Root;
    Core3D::Camera m_Camera;
    Core3D::Frustum m_Frustum;
    bool m_FrustumCullingEnabled = true;

    // Pre-allocated buffer for FlattenTree to avoid per-frame allocations
    mutable std::vector<std::shared_ptr<SceneNode>> m_FlattenBuffer;
    mutable CullStats m_CullStats;

    DirectionalLight m_DirLight;
    bool m_HasDirLight = false;
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight> m_SpotLights;

    std::shared_ptr<Skybox> m_Skybox;
};

} // namespace Scene

#endif
