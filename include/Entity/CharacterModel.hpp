#ifndef CS_ENTITY_CHARACTER_MODEL_HPP
#define CS_ENTITY_CHARACTER_MODEL_HPP

#include "pch.hpp"

#include "Core3D/Model.hpp"
#include "Core3D/BoneAnimation.hpp"
#include "Scene/SceneGraph.hpp"
#include "Scene/SceneNode.hpp"

#include <memory>
#include <string>

namespace Entity {

/**
 * @brief Available character types.
 */
enum class CharacterType {
    FBI,        ///< Counter-Terrorist
    TERRORIST   ///< Terrorist
};

/**
 * @brief Manages a character's 3D model and animations.
 *
 * Handles:
 * - Loading character GLTF models (FBI, Terrorist)
 * - Visibility toggle for development/debugging
 * - Animation playback (walk animation if available)
 * - Character type switching
 *
 * Usage:
 * @code
 * Entity::CharacterModel model;
 * model.Init(scene, Entity::CharacterType::FBI);
 * model.SetVisible(true);
 *
 * // Each frame:
 * model.Update(dt, position, yaw, isWalking);
 * @endcode
 */
class CharacterModel {
public:
    CharacterModel() = default;
    ~CharacterModel() = default;

    /**
     * @brief Initialize and load the character model.
     * @param scene The scene graph to attach the model to.
     * @param type The character type to load.
     * @param visible Whether the model should be visible initially.
     */
    void Init(Scene::SceneGraph &scene, CharacterType type, bool visible = false);

    /**
     * @brief Switch to a different character type.
     * @param scene The scene graph (needed to update attachments).
     * @param type The new character type.
     */
    void SwitchCharacter(Scene::SceneGraph &scene, CharacterType type);

    /**
     * @brief Update the model's position and animation.
     * @param dt Delta time in seconds.
     * @param position World position (feet position).
     * @param yaw Character facing direction in degrees.
     * @param isWalking Whether the character is currently walking.
     */
    void Update(float dt, const glm::vec3 &position, float yaw, bool isWalking);

    // ── Visibility ────────────────────────────────────────────────────────

    void SetVisible(bool visible);
    bool IsVisible() const { return m_Visible; }
    void ToggleVisibility() { SetVisible(!m_Visible); }

    // ── Getters ───────────────────────────────────────────────────────────

    CharacterType GetCharacterType() const { return m_CurrentType; }
    const std::shared_ptr<Scene::SceneNode> &GetNode() const { return m_Node; }
    std::shared_ptr<Core3D::Model> GetModel() const { return m_Model; }

    /** @brief Check if walking animation is available. */
    bool HasWalkAnimation() const { return m_WalkAnimation.IsValid(); }

private:
    /** @brief Get the model path for a character type. */
    static std::string GetModelPath(CharacterType type);

    /** @brief Load the model and animation for the current type. */
    void LoadModel(Scene::SceneGraph &scene);

    // ── Current State ─────────────────────────────────────────────────────
    CharacterType m_CurrentType = CharacterType::FBI;
    bool m_Visible = false;
    bool m_Initialized = false;

    // ── 3D Model ──────────────────────────────────────────────────────────
    std::shared_ptr<Core3D::Model> m_Model;
    std::shared_ptr<Scene::SceneNode> m_Node;
    Scene::SceneGraph *m_Scene = nullptr;

    // ── Animation ─────────────────────────────────────────────────────────
    Core3D::BoneAnimation m_WalkAnimation;
    float m_AnimationTime = 0.0f;
    bool m_IsWalking = false;

    // ── Model Configuration ───────────────────────────────────────────────
    static constexpr float MODEL_SCALE = 0.020f;    ///< GLTF models are large
    static constexpr float MODEL_Y_OFFSET = 0.0f;   ///< Y offset from feet
};

} // namespace Entity

#endif // CS_ENTITY_CHARACTER_MODEL_HPP
