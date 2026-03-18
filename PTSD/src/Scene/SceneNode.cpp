#include "Scene/SceneNode.hpp"

namespace Scene {

glm::mat4 SceneNode::GetLocalTransform() const {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_Position);
    glm::mat4 rotation = glm::toMat4(m_Rotation);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
    return translation * rotation * scale;
}

const glm::mat4 &SceneNode::GetWorldTransform() const {
    if (m_TransformDirty) {
        UpdateCachedTransforms();
    }
    return m_CachedWorldTransform;
}

const glm::mat4 &SceneNode::GetNormalMatrix() const {
    if (m_TransformDirty) {
        UpdateCachedTransforms();
    }
    return m_CachedNormalMatrix;
}

void SceneNode::UpdateCachedTransforms() const {
    glm::mat4 local = GetLocalTransform();
    if (m_Parent) {
        m_CachedWorldTransform = m_Parent->GetWorldTransform() * local;
    } else {
        m_CachedWorldTransform = local;
    }
    m_CachedNormalMatrix = glm::transpose(glm::inverse(m_CachedWorldTransform));
    m_TransformDirty = false;
    m_AABBDirty = true;
}

void SceneNode::MarkTransformDirty() {
    if (!m_TransformDirty) {
        m_TransformDirty = true;
        m_AABBDirty = true;
        // Propagate to children
        for (const auto &child : m_Children) {
            child->MarkTransformDirty();
        }
    }
}

void SceneNode::SetDrawable(const std::shared_ptr<Core3D::Drawable3D> &drawable) {
    m_Drawable = drawable;
    // Compute AABB from drawable if available
    if (drawable && !m_HasLocalAABB) {
        glm::vec3 bbox = drawable->GetBoundingBox();
        m_LocalAABB.min = -bbox * 0.5f;
        m_LocalAABB.max = bbox * 0.5f;
    }
    m_AABBDirty = true;
}

const Core3D::AABB &SceneNode::GetWorldAABB() const {
    if (m_AABBDirty) {
        UpdateWorldAABB();
    }
    return m_CachedWorldAABB;
}

void SceneNode::UpdateWorldAABB() const {
    if (m_TransformDirty) {
        UpdateCachedTransforms();
    }
    m_CachedWorldAABB = m_LocalAABB.Transform(m_CachedWorldTransform);
    m_AABBDirty = false;
}

void SceneNode::AddChild(const std::shared_ptr<SceneNode> &child) {
    child->m_Parent = this;
    child->MarkTransformDirty();
    m_Children.push_back(child);
}

void SceneNode::RemoveChild(const std::shared_ptr<SceneNode> &child) {
    m_Children.erase(
        std::remove(m_Children.begin(), m_Children.end(), child),
        m_Children.end());
    child->m_Parent = nullptr;
    child->MarkTransformDirty();
}

} // namespace Scene
