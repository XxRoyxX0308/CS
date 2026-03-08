#include "Core3D/Skeleton.hpp"

namespace Core3D {

// ====== Bone implementation ======

template <typename T>
static int FindKeyIndex(float animTime, const std::vector<T> &keys) {
    for (int i = 0; i < static_cast<int>(keys.size()) - 1; ++i) {
        if (animTime < keys[i + 1].timeStamp) {
            return i;
        }
    }
    return static_cast<int>(keys.size()) - 1;
}

static float GetScaleFactor(float lastTimeStamp, float nextTimeStamp,
                             float animationTime) {
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    if (framesDiff <= 0.0f) return 0.0f;
    return midWayLength / framesDiff;
}

glm::vec3 Bone::InterpolatePosition(float animationTime) const {
    if (positions.size() == 1) return positions[0].position;
    if (positions.empty()) return glm::vec3(0.0f);

    int p0Index = FindKeyIndex(animationTime, positions);
    int p1Index = p0Index + 1;
    if (p1Index >= static_cast<int>(positions.size())) {
        return positions[p0Index].position;
    }

    float scaleFactor =
        GetScaleFactor(positions[p0Index].timeStamp,
                       positions[p1Index].timeStamp, animationTime);
    return glm::mix(positions[p0Index].position,
                    positions[p1Index].position, scaleFactor);
}

glm::quat Bone::InterpolateRotation(float animationTime) const {
    if (rotations.size() == 1) return rotations[0].orientation;
    if (rotations.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    int p0Index = FindKeyIndex(animationTime, rotations);
    int p1Index = p0Index + 1;
    if (p1Index >= static_cast<int>(rotations.size())) {
        return rotations[p0Index].orientation;
    }

    float scaleFactor =
        GetScaleFactor(rotations[p0Index].timeStamp,
                       rotations[p1Index].timeStamp, animationTime);
    return glm::normalize(
        glm::slerp(rotations[p0Index].orientation,
                    rotations[p1Index].orientation, scaleFactor));
}

glm::vec3 Bone::InterpolateScale(float animationTime) const {
    if (scales.size() == 1) return scales[0].scale;
    if (scales.empty()) return glm::vec3(1.0f);

    int p0Index = FindKeyIndex(animationTime, scales);
    int p1Index = p0Index + 1;
    if (p1Index >= static_cast<int>(scales.size())) {
        return scales[p0Index].scale;
    }

    float scaleFactor =
        GetScaleFactor(scales[p0Index].timeStamp,
                       scales[p1Index].timeStamp, animationTime);
    return glm::mix(scales[p0Index].scale, scales[p1Index].scale,
                    scaleFactor);
}

glm::mat4 Bone::GetLocalTransform(float animationTime) const {
    glm::vec3 pos = InterpolatePosition(animationTime);
    glm::quat rot = InterpolateRotation(animationTime);
    glm::vec3 scl = InterpolateScale(animationTime);

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 rotation = glm::toMat4(rot);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), scl);

    return translation * rotation * scale;
}

// ====== Skeleton implementation ======

void Skeleton::AddBone(const Bone &bone) {
    m_BoneMapping[bone.name] = static_cast<int>(m_Bones.size());
    m_Bones.push_back(bone);
}

Bone *Skeleton::FindBone(const std::string &name) {
    auto it = m_BoneMapping.find(name);
    if (it != m_BoneMapping.end()) {
        return &m_Bones[it->second];
    }
    return nullptr;
}

const Bone *Skeleton::FindBone(const std::string &name) const {
    auto it = m_BoneMapping.find(name);
    if (it != m_BoneMapping.end()) {
        return &m_Bones[it->second];
    }
    return nullptr;
}

int Skeleton::GetBoneId(const std::string &name) const {
    auto it = m_BoneMapping.find(name);
    if (it != m_BoneMapping.end()) {
        return it->second;
    }
    return -1;
}

} // namespace Core3D
