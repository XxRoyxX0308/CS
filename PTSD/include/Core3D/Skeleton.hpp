#ifndef CORE3D_SKELETON_HPP
#define CORE3D_SKELETON_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Core3D {

constexpr int MAX_BONES = 128;

/**
 * @brief Keyframe data for position interpolation.
 */
struct KeyPosition {
    glm::vec3 position;
    float timeStamp;
};

/**
 * @brief Keyframe data for rotation interpolation.
 */
struct KeyRotation {
    glm::quat orientation;
    float timeStamp;
};

/**
 * @brief Keyframe data for scale interpolation.
 */
struct KeyScale {
    glm::vec3 scale;
    float timeStamp;
};

/**
 * @brief A single bone with keyframe animation data.
 */
struct Bone {
    int id = -1;
    std::string name;
    glm::mat4 offsetMatrix{1.0f}; ///< Transforms from mesh space to bone space
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;

    /**
     * @brief Interpolate position at the given animation time.
     */
    glm::vec3 InterpolatePosition(float animationTime) const;

    /**
     * @brief Interpolate rotation at the given animation time.
     */
    glm::quat InterpolateRotation(float animationTime) const;

    /**
     * @brief Interpolate scale at the given animation time.
     */
    glm::vec3 InterpolateScale(float animationTime) const;

    /**
     * @brief Compute the local transform for this bone at the given time.
     */
    glm::mat4 GetLocalTransform(float animationTime) const;
};

/**
 * @brief Skeleton containing all bones and their hierarchy.
 *
 * Used with BoneAnimation to compute final bone transforms
 * for skeletal animation in the vertex shader.
 */
class Skeleton {
public:
    Skeleton() = default;

    void AddBone(const Bone &bone);
    Bone *FindBone(const std::string &name);
    const Bone *FindBone(const std::string &name) const;

    int GetBoneId(const std::string &name) const;
    int GetBoneCount() const { return static_cast<int>(m_Bones.size()); }

    void SetGlobalInverseTransform(const glm::mat4 &mat) {
        m_GlobalInverseTransform = mat;
    }
    const glm::mat4 &GetGlobalInverseTransform() const {
        return m_GlobalInverseTransform;
    }

private:
    std::vector<Bone> m_Bones;
    std::unordered_map<std::string, int> m_BoneMapping;
    glm::mat4 m_GlobalInverseTransform{1.0f};
};

} // namespace Core3D

#endif
