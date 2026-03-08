#include "Core3D/BoneAnimation.hpp"
#include "Util/Logger.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace Core3D {

static glm::mat4 AiMatToGlm(const aiMatrix4x4 &from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

BoneAnimation::BoneAnimation(const std::string &modelPath, int animIndex) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        modelPath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace);

    if (!scene || !scene->mRootNode) {
        LOG_ERROR("BoneAnimation: Failed to load model '{}'", modelPath);
        return;
    }

    if (static_cast<int>(scene->mNumAnimations) <= animIndex) {
        LOG_WARN("BoneAnimation: No animation at index {} in '{}'",
                 animIndex, modelPath);
        return;
    }

    auto *animation = scene->mAnimations[animIndex];
    m_Duration = static_cast<float>(animation->mDuration);
    m_TicksPerSecond = animation->mTicksPerSecond != 0
                           ? static_cast<float>(animation->mTicksPerSecond)
                           : 25.0f;

    m_Skeleton.SetGlobalInverseTransform(
        glm::inverse(AiMatToGlm(scene->mRootNode->mTransformation)));

    ReadBones(animation, scene);
    ReadHierarchyData(m_RootNode, scene->mRootNode);

    LOG_INFO("BoneAnimation loaded: '{}' (duration: {:.1f}, bones: {})",
             modelPath, m_Duration, m_Skeleton.GetBoneCount());
}

void BoneAnimation::ReadBones(const aiAnimation *animation,
                               const aiScene *scene) {
    // First pass: collect bone info from meshes
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        auto *mesh = scene->mMeshes[m];
        for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
            auto *aiBone = mesh->mBones[b];
            std::string boneName = aiBone->mName.C_Str();

            if (m_Skeleton.GetBoneId(boneName) == -1) {
                Bone bone;
                bone.id = m_Skeleton.GetBoneCount();
                bone.name = boneName;
                bone.offsetMatrix = AiMatToGlm(aiBone->mOffsetMatrix);
                m_Skeleton.AddBone(bone);
            }
        }
    }

    // Second pass: read keyframes from animation channels
    for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
        auto *channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();

        Bone *bone = m_Skeleton.FindBone(boneName);
        if (!bone) {
            // Bone not in skeleton (might be a non-deforming node)
            Bone newBone;
            newBone.id = m_Skeleton.GetBoneCount();
            newBone.name = boneName;
            m_Skeleton.AddBone(newBone);
            bone = m_Skeleton.FindBone(boneName);
        }

        // Position keyframes
        for (unsigned int j = 0; j < channel->mNumPositionKeys; ++j) {
            KeyPosition key;
            key.position = {channel->mPositionKeys[j].mValue.x,
                            channel->mPositionKeys[j].mValue.y,
                            channel->mPositionKeys[j].mValue.z};
            key.timeStamp = static_cast<float>(channel->mPositionKeys[j].mTime);
            bone->positions.push_back(key);
        }

        // Rotation keyframes
        for (unsigned int j = 0; j < channel->mNumRotationKeys; ++j) {
            KeyRotation key;
            auto &q = channel->mRotationKeys[j].mValue;
            key.orientation = glm::quat(q.w, q.x, q.y, q.z);
            key.timeStamp = static_cast<float>(channel->mRotationKeys[j].mTime);
            bone->rotations.push_back(key);
        }

        // Scale keyframes
        for (unsigned int j = 0; j < channel->mNumScalingKeys; ++j) {
            KeyScale key;
            key.scale = {channel->mScalingKeys[j].mValue.x,
                         channel->mScalingKeys[j].mValue.y,
                         channel->mScalingKeys[j].mValue.z};
            key.timeStamp = static_cast<float>(channel->mScalingKeys[j].mTime);
            bone->scales.push_back(key);
        }
    }
}

void BoneAnimation::ReadHierarchyData(AssimpNodeData &dest,
                                        const aiNode *src) {
    dest.name = src->mName.C_Str();
    dest.transformation = AiMatToGlm(src->mTransformation);
    dest.children.reserve(src->mNumChildren);

    for (unsigned int i = 0; i < src->mNumChildren; ++i) {
        AssimpNodeData childData;
        ReadHierarchyData(childData, src->mChildren[i]);
        dest.children.push_back(std::move(childData));
    }
}

void BoneAnimation::UpdateBoneTransforms(
    float time, std::vector<glm::mat4> &finalTransforms) {
    finalTransforms.resize(MAX_BONES, glm::mat4(1.0f));

    if (m_Duration <= 0.0f) return;

    float animTime = fmod(time * m_TicksPerSecond, m_Duration);
    CalculateBoneTransform(m_RootNode, glm::mat4(1.0f), animTime,
                            finalTransforms);
}

void BoneAnimation::CalculateBoneTransform(
    const AssimpNodeData &node, const glm::mat4 &parentTransform,
    float animTime, std::vector<glm::mat4> &finalTransforms) {

    glm::mat4 nodeTransform = node.transformation;

    const Bone *bone = m_Skeleton.FindBone(node.name);
    if (bone) {
        nodeTransform = bone->GetLocalTransform(animTime);
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    if (bone && bone->id >= 0 && bone->id < MAX_BONES) {
        finalTransforms[bone->id] =
            m_Skeleton.GetGlobalInverseTransform() *
            globalTransformation * bone->offsetMatrix;
    }

    for (const auto &child : node.children) {
        CalculateBoneTransform(child, globalTransformation, animTime,
                                finalTransforms);
    }
}

} // namespace Core3D
