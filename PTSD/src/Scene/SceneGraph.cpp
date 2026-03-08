#include "Scene/SceneGraph.hpp"
#include "Scene/Skybox.hpp"
#include "Util/Logger.hpp"

namespace Scene {

SceneGraph::SceneGraph()
    : m_Root(std::make_shared<SceneNode>()) {}

void SceneGraph::AddNode(const std::shared_ptr<SceneNode> &node) {
    m_Root->AddChild(node);
}

void SceneGraph::RemoveNode(const std::shared_ptr<SceneNode> &node) {
    m_Root->RemoveChild(node);
}

std::vector<std::shared_ptr<SceneNode>> SceneGraph::FlattenTree() const {
    std::vector<std::shared_ptr<SceneNode>> result;
    for (const auto &child : m_Root->GetChildren()) {
        FlattenNode(child, result);
    }
    return result;
}

void SceneGraph::FlattenNode(
    const std::shared_ptr<SceneNode> &node,
    std::vector<std::shared_ptr<SceneNode>> &out) const {

    if (node->IsVisible()) {
        out.push_back(node);
    }

    for (const auto &child : node->GetChildren()) {
        FlattenNode(child, out);
    }
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
