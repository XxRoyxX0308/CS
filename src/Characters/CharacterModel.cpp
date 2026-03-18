#include "Characters/CharacterModel.hpp"
#include "Util/Logger.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Characters {

// ============================================================================
//  GetModelPath — Returns the GLTF model path for a character type
// ============================================================================
std::string CharacterModel::GetModelPath(CharacterType type) {
    switch (type) {
        case CharacterType::FBI:
            return std::string(ASSETS_DIR) + "/characters/fbi/scene.gltf";
        case CharacterType::TERRORIST:
            return std::string(ASSETS_DIR) + "/characters/terrorist/scene.gltf";
        default:
            return std::string(ASSETS_DIR) + "/characters/fbi/scene.gltf";
    }
}

// ============================================================================
//  Init — Initialize the character model
// ============================================================================
void CharacterModel::Init(Scene::SceneGraph &scene, CharacterType type) {
    m_CurrentType = type;
    m_Scene = &scene;
    LoadModel(scene);
    m_Initialized = true;
    LOG_INFO("CharacterModel initialized with type: {}",
             type == CharacterType::FBI ? "FBI" : "Terrorist");
}

// ============================================================================
//  LoadModel — Load the 3D model and optional animation
// ============================================================================
void CharacterModel::LoadModel(Scene::SceneGraph &scene) {
    std::string modelPath = GetModelPath(m_CurrentType);

    // Remove old node if switching characters
    if (m_Node && m_Scene) {
        m_Scene->GetRoot()->RemoveChild(m_Node);
    }

    // Load the model
    m_Model = std::make_shared<Core3D::Model>(modelPath, false);

    // Create scene node
    m_Node = std::make_shared<Scene::SceneNode>();
    m_Node->SetScale(glm::vec3(MODEL_SCALE));
    m_Node->SetDrawable(m_Model);

    // Initially hidden based on m_Visible flag
    m_Node->SetVisible(m_Visible);

    // Add to scene
    scene.GetRoot()->AddChild(m_Node);

    // Try to load walk animation (if available in the model)
    // Note: These CS:GO models may not have embedded animations
    m_WalkAnimation = Core3D::BoneAnimation(modelPath, 0);
    if (m_WalkAnimation.IsValid()) {
        LOG_INFO("Walk animation loaded for character model");
    }
}

// ============================================================================
//  SwitchCharacter — Switch to a different character type
// ============================================================================
void CharacterModel::SwitchCharacter(Scene::SceneGraph &scene, CharacterType type) {
    if (m_CurrentType == type && m_Initialized) {
        return; // Already using this character
    }

    m_CurrentType = type;
    m_Scene = &scene;
    LoadModel(scene);

    LOG_INFO("Switched character to: {}",
             type == CharacterType::FBI ? "FBI" : "Terrorist");
}

// ============================================================================
//  Update — Update position and animation
// ============================================================================
void CharacterModel::Update(float dt, const glm::vec3 &position, float yaw, bool isWalking) {
    if (!m_Node) return;

    // Update position (position is eye height, need to offset to feet)
    glm::vec3 modelPos = position;
    modelPos.y += MODEL_Y_OFFSET;
    m_Node->SetPosition(modelPos);

    // Update rotation (yaw only, character faces camera direction)
    // GLTF models typically face -Z, so we rotate 180 degrees + yaw
    float rotationY = glm::radians(yaw - 90.0f);
    glm::quat rotation = glm::angleAxis(-rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat upTurn90 = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    rotation = rotation * upTurn90;
    m_Node->SetRotation(rotation);

    // Update walking state for animation
    m_IsWalking = isWalking;

    // Update animation if available and walking
    if (m_WalkAnimation.IsValid() && m_IsWalking) {
        m_AnimationTime += dt;

        // Loop the animation
        float duration = m_WalkAnimation.GetDuration() /
                        m_WalkAnimation.GetTicksPerSecond();
        if (duration > 0.0f) {
            while (m_AnimationTime > duration) {
                m_AnimationTime -= duration;
            }
        }

        // Note: Skeleton animation would need shader support to apply
        // For now, the model will be static but positioned correctly
    } else if (!m_IsWalking) {
        // Reset animation time when not walking
        m_AnimationTime = 0.0f;
    }
}

// ============================================================================
//  SetVisible — Toggle model visibility
// ============================================================================
void CharacterModel::SetVisible(bool visible) {
    m_Visible = visible;
    if (m_Node) {
        m_Node->SetVisible(visible);
    }
}

} // namespace Characters
