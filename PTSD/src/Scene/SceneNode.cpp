#include "Scene/SceneNode.hpp"

namespace Scene {

glm::mat4 SceneNode::GetLocalTransform() const {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_Position);
    glm::mat4 rotation = glm::toMat4(m_Rotation);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
    return translation * rotation * scale;
}

glm::mat4 SceneNode::GetWorldTransform() const {
    glm::mat4 local = GetLocalTransform();
    if (m_Parent) {
        return m_Parent->GetWorldTransform() * local;
    }
    return local;
}

void SceneNode::AddChild(const std::shared_ptr<SceneNode> &child) {
    child->m_Parent = this;
    m_Children.push_back(child);
}

void SceneNode::RemoveChild(const std::shared_ptr<SceneNode> &child) {
    m_Children.erase(
        std::remove(m_Children.begin(), m_Children.end(), child),
        m_Children.end());
    child->m_Parent = nullptr;
}

} // namespace Scene
