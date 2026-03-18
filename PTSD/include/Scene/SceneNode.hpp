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
 * **Performance optimizations**:
 * - Transform caching with dirty flags (avoids redundant matrix computation)
 * - AABB for frustum culling
 * - Static flag for batching hints
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
        MarkTransformDirty();
    }
    void SetRotation(const glm::quat &rot) {
        m_Rotation = rot;
        MarkTransformDirty();
    }
    void SetScale(const glm::vec3 &scl) {
        m_Scale = scl;
        MarkTransformDirty();
    }

    const glm::vec3 &GetPosition() const { return m_Position; }
    const glm::quat &GetRotation() const { return m_Rotation; }
    const glm::vec3 &GetScale() const { return m_Scale; }

    /** @brief Compute the local TRS matrix. */
    glm::mat4 GetLocalTransform() const;

    /**
     * @brief Get the cached world-space transform.
     *
     * Uses dirty flag optimization - only recomputes when necessary.
     */
    const glm::mat4 &GetWorldTransform() const;

    /**
     * @brief Get the cached inverse-transpose of world transform (for normals).
     */
    const glm::mat4 &GetNormalMatrix() const;

    // ====== Draw ======

    void SetDrawable(const std::shared_ptr<Core3D::Drawable3D> &drawable);
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

    // ====== Hierarchy ======

    void AddChild(const std::shared_ptr<SceneNode> &child);
    void RemoveChild(const std::shared_ptr<SceneNode> &child);
    const std::vector<std::shared_ptr<SceneNode>> &GetChildren() const {
        return m_Children;
    }
    SceneNode *GetParent() const { return m_Parent; }

    // ====== Culling & Optimization ======

    /**
     * @brief Set the local-space AABB for this node.
     *
     * If not set, will attempt to compute from Drawable.
     */
    void SetLocalAABB(const Core3D::AABB &aabb) {
        m_LocalAABB = aabb;
        m_HasLocalAABB = true;
    }

    /**
     * @brief Get the local-space AABB.
     */
    const Core3D::AABB &GetLocalAABB() const { return m_LocalAABB; }

    /**
     * @brief Get the world-space AABB (transformed by world matrix).
     */
    const Core3D::AABB &GetWorldAABB() const;

    /**
     * @brief Check if this node has a valid AABB for culling.
     */
    bool HasAABB() const { return m_HasLocalAABB || m_Drawable != nullptr; }

    /**
     * @brief Mark this node as static (won't move after setup).
     *
     * Static nodes can be batched together for better performance.
     */
    void SetStatic(bool isStatic) { m_IsStatic = isStatic; }
    bool IsStatic() const { return m_IsStatic; }

    /**
     * @brief Mark this node for culling bypass (always render).
     *
     * Use for skybox, UI elements, or objects that must always render.
     */
    void SetAlwaysVisible(bool always) { m_AlwaysVisible = always; }
    bool IsAlwaysVisible() const { return m_AlwaysVisible; }

    /**
     * @brief Force recalculation of cached transforms on next access.
     */
    void MarkTransformDirty();

protected:
    void UpdateCachedTransforms() const;
    void UpdateWorldAABB() const;

    // Transform
    glm::vec3 m_Position{0.0f};
    glm::quat m_Rotation{1.0f, 0.0f, 0.0f, 0.0f}; ///< Identity quaternion
    glm::vec3 m_Scale{1.0f};

    // Cached transforms (mutable for lazy evaluation in const methods)
    mutable glm::mat4 m_CachedWorldTransform{1.0f};
    mutable glm::mat4 m_CachedNormalMatrix{1.0f};
    mutable bool m_TransformDirty = true;

    // AABB for culling
    Core3D::AABB m_LocalAABB;
    mutable Core3D::AABB m_CachedWorldAABB;
    mutable bool m_AABBDirty = true;
    bool m_HasLocalAABB = false;

    // Rendering
    std::shared_ptr<Core3D::Drawable3D> m_Drawable;
    std::shared_ptr<Core3D::Material> m_Material;
    bool m_Visible = true;
    bool m_IsStatic = false;
    bool m_AlwaysVisible = false;

    // Hierarchy
    std::vector<std::shared_ptr<SceneNode>> m_Children;
    SceneNode *m_Parent = nullptr;
};

} // namespace Scene

#endif
