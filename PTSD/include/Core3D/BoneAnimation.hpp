#ifndef CORE3D_BONE_ANIMATION_HPP
#define CORE3D_BONE_ANIMATION_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/Skeleton.hpp"

#include <assimp/scene.h>

namespace Core3D {

/**
 * @brief Node in the bone hierarchy tree.
 */
struct AssimpNodeData {
    std::string name;
    glm::mat4 transformation{1.0f};
    std::vector<AssimpNodeData> children;
};

/**
 * @brief Manages a skeletal animation loaded from an Assimp scene.
 *
 * Computes per-bone transformation matrices each frame by traversing
 * the bone hierarchy and interpolating between keyframes.
 *
 * @code
 * // Load animation from scene
 * Core3D::BoneAnimation anim("assets/models/character.fbx", 0);
 *
 * // Each frame:
 * float time = ...; // animation time in seconds
 * std::vector<glm::mat4> boneTransforms;
 * anim.UpdateBoneTransforms(time, boneTransforms);
 * // Upload boneTransforms to UBO
 * @endcode
 */
class BoneAnimation {
public:
    /**
     * @brief Load an animation from a model file.
     * @param modelPath Path to the 3D model file.
     * @param animIndex Index of the animation to load (default: 0).
     */
    BoneAnimation(const std::string &modelPath, int animIndex = 0);
    BoneAnimation() = default;

    /**
     * @brief Compute final bone matrices at the given time.
     * @param time Current animation time in seconds.
     * @param finalTransforms Output: bone matrices ready for the shader.
     */
    void UpdateBoneTransforms(float time,
                              std::vector<glm::mat4> &finalTransforms);

    float GetDuration() const { return m_Duration; }
    float GetTicksPerSecond() const { return m_TicksPerSecond; }
    const Skeleton &GetSkeleton() const { return m_Skeleton; }

    /**
     * @brief Check if this animation is valid (loaded successfully).
     */
    bool IsValid() const { return m_Duration > 0.0f; }

private:
    void ReadHierarchyData(AssimpNodeData &dest, const aiNode *src);
    void ReadBones(const aiAnimation *animation, const aiScene *scene);
    void CalculateBoneTransform(const AssimpNodeData &node,
                                const glm::mat4 &parentTransform,
                                float animTime,
                                std::vector<glm::mat4> &finalTransforms);

    float m_Duration = 0.0f;
    float m_TicksPerSecond = 25.0f;
    Skeleton m_Skeleton;
    AssimpNodeData m_RootNode;
};

} // namespace Core3D

#endif
