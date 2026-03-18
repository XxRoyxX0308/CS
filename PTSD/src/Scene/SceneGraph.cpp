#include "Scene/SceneGraph.hpp"
#include "Scene/Skybox.hpp"
#include "Util/Logger.hpp"

namespace Scene {

SceneGraph::SceneGraph()
    : m_Root(std::make_shared<SceneNode>()) {
    // Pre-allocate buffer with reasonable initial capacity
    m_FlattenBuffer.reserve(256);
}

void SceneGraph::AddNode(const std::shared_ptr<SceneNode> &node) {
    m_Root->AddChild(node);
}

void SceneGraph::RemoveNode(const std::shared_ptr<SceneNode> &node) {
    m_Root->RemoveChild(node);
}

const std::vector<std::shared_ptr<SceneNode>> &SceneGraph::FlattenTree() {
    // Clear but keep capacity to avoid reallocation
    m_FlattenBuffer.clear();
    m_CullStats = {};

    for (const auto &child : m_Root->GetChildren()) {
        FlattenNode(child);
    }

    m_CullStats.totalNodes = m_CullStats.visibleNodes;
    return m_FlattenBuffer;
}

const std::vector<std::shared_ptr<SceneNode>> &SceneGraph::FlattenTreeCulled(
    const Core3D::Frustum &frustum) {
    // Clear but keep capacity to avoid reallocation
    m_FlattenBuffer.clear();
    m_CullStats = {};

    for (const auto &child : m_Root->GetChildren()) {
        FlattenNodeCulled(child, frustum);
    }

    return m_FlattenBuffer;
}

void SceneGraph::FlattenNode(const std::shared_ptr<SceneNode> &node) {
    if (node->IsVisible()) {
        m_FlattenBuffer.push_back(node);
        ++m_CullStats.visibleNodes;
    }

    for (const auto &child : node->GetChildren()) {
        FlattenNode(child);
    }
}

void SceneGraph::FlattenNodeCulled(const std::shared_ptr<SceneNode> &node,
                                    const Core3D::Frustum &frustum) {
    if (!node->IsVisible()) {
        return;
    }

    ++m_CullStats.totalNodes;

    // Check frustum culling
    bool visible = true;
    if (m_FrustumCullingEnabled && node->IsCullingEnabled()) {
        Core3D::AABB worldBounds = node->GetWorldBounds();
        if (worldBounds.IsValid()) {
            visible = frustum.IsAABBVisible(worldBounds);
        }
    }

    if (visible) {
        m_FlattenBuffer.push_back(node);
        ++m_CullStats.visibleNodes;
    } else {
        ++m_CullStats.culledNodes;
    }

    // Recursively process children
    // Note: If parent is culled, we still need to check children
    // because they might be visible if they have their own bounds
    for (const auto &child : node->GetChildren()) {
        FlattenNodeCulled(child, frustum);
    }
}

void SceneGraph::UpdateFrustum() {
    glm::mat4 view = m_Camera.GetViewMatrix();
    glm::mat4 proj = m_Camera.GetProjectionMatrix();
    m_Frustum.Update(proj * view);
}

LightsUBO SceneGraph::BuildLightsUBO() const {
    LightsUBO ubo{};

    if (m_HasDirLight) {
        ubo.dirLight = ToGPU(m_DirLight);
    }

    int numPoint = std::min(static_cast<int>(m_PointLights.size()),
                            GPU_MAX_POINT_LIGHTS);
    int numSpot = std::min(static_cast<int>(m_SpotLights.size()),
                           GPU_MAX_SPOT_LIGHTS);

    ubo.lightCounts = glm::ivec4(numPoint, numSpot, m_HasDirLight ? 1 : 0, 0);

    for (int i = 0; i < numPoint; ++i) {
        ubo.pointLights[i] = ToGPU(m_PointLights[i]);
    }

    for (int i = 0; i < numSpot; ++i) {
        ubo.spotLights[i] = ToGPU(m_SpotLights[i]);
    }

    return ubo;
}

} // namespace Scene
