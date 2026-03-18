#ifndef SCENE_SCENE_NODE_HPP
#define SCENE_SCENE_NODE_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/Drawable3D.hpp"
#include "Core3D/Material.hpp"
#include "Core3D/AABB.hpp"

namespace Scene {

/**
 * @brief A node in the 3D scene graph.
 *
 * Each node has a 3D transform (position, rotation as quaternion, scale),
 * an optional drawable, an optional material, and any number of children.
 * World transforms are computed by multiplying parent transforms recursively.
 *
 * Includes AABB bounding box for frustum culling optimization.
 *
 * @code
 * auto node = std::make_shared<Scene::SceneNode>();
 * node->SetPosition(glm::vec3(0, 2, -5));
 * node->SetRotation(glm::quat(glm::radians(glm::vec3(0, 45, 0))));
 * node->SetDrawable(myModel);
 * node->SetMaterial(myMaterial);
 * sceneGraph.AddNode(node);
 * @endcode
 */
class SceneNode : public std::enable_shared_from_this<SceneNode> {
public:
    SceneNode() = default;
    virtual ~SceneNode() = default;

    // ====== Transform ======

    void SetPosition(const glm::vec3 &pos) {
        m_Position = pos;
        m_TransformDirty = true;
    }
    void SetRotation(const glm::quat &rot) {
        m_Rotation = rot;
        m_TransformDirty = true;
    }
    void SetScale(const glm::vec3 &scl) {
        m_Scale = scl;
        m_TransformDirty = true;
    }

    const glm::vec3 &GetPosition() const { return m_Position; }
    const glm::quat &GetRotation() const { return m_Rotation; }
    const glm::vec3 &GetScale() const { return m_Scale; }

    /** @brief Compute the local TRS matrix. */
    glm::mat4 GetLocalTransform() const;

    /**
     * @brief Compute the world-space transform.
     *
     * Recursively multiplies by parent transforms.
     */
    glm::mat4 GetWorldTransform() const;

    // ====== Draw ======

    void SetDrawable(const std::shared_ptr<Core3D::Drawable3D> &drawable) {
        m_Drawable = drawable;
        m_BoundsDirty = true;
    }
    void SetMaterial(const std::shared_ptr<Core3D::Material> &material) {
        m_Material = material;
    }
    void SetVisible(bool visible) { m_Visible = visible; }
    bool IsVisible() const { return m_Visible; }

    const std::shared_ptr<Core3D::Drawable3D> &GetDrawable() const {
        return m_Drawable;
    }
    const std::shared_ptr<Core3D::Material> &GetMaterial() const {
        return m_Material;
    }

    // ====== Bounding Box (for Frustum Culling) ======

    /**
     * @brief Set the local-space bounding box manually.
     */
    void SetLocalBounds(const Core3D::AABB &bounds) {
        m_LocalBounds = bounds;
        m_BoundsDirty = false;
    }

    /**
     * @brief Get the local-space bounding box.
     *
     * If bounds are dirty and a drawable is attached, tries to compute
     * bounds from the drawable.
     */
    const Core3D::AABB &GetLocalBounds() const {
        return m_LocalBounds;
    }

    /**
     * @brief Get world-space bounding box (transformed by world matrix).
     */
    Core3D::AABB GetWorldBounds() const {
        return m_LocalBounds.TransformFast(GetWorldTransform());
    }

    /**
     * @brief Check if this node should be culled.
     */
    bool IsCullingEnabled() const { return m_CullingEnabled; }
    void SetCullingEnabled(bool enabled) { m_CullingEnabled = enabled; }

    // ====== Hierarchy ======

    void AddChild(const std::shared_ptr<SceneNode> &child);
    void RemoveChild(const std::shared_ptr<SceneNode> &child);
    const std::vector<std::shared_ptr<SceneNode>> &GetChildren() const {
        return m_Children;
    }
    SceneNode *GetParent() const { return m_Parent; }

protected:
    // Transform
    glm::vec3 m_Position{0.0f};
    glm::quat m_Rotation{1.0f, 0.0f, 0.0f, 0.0f}; ///< Identity quaternion
    glm::vec3 m_Scale{1.0f};
    mutable bool m_TransformDirty = true;
    mutable glm::mat4 m_CachedWorldTransform{1.0f};

    // Rendering
    std::shared_ptr<Core3D::Drawable3D> m_Drawable;
    std::shared_ptr<Core3D::Material> m_Material;
    bool m_Visible = true;

    // Bounding box for culling
    Core3D::AABB m_LocalBounds;
    mutable bool m_BoundsDirty = true;
    bool m_CullingEnabled = true;

    // Hierarchy
    std::vector<std::shared_ptr<SceneNode>> m_Children;
    SceneNode *m_Parent = nullptr;
};

} // namespace Scene

#endif
