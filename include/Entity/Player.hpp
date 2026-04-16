#ifndef CS_ENTITY_PLAYER_HPP
#define CS_ENTITY_PLAYER_HPP

#include "Entity/Character.hpp"
#include "Entity/CharacterModel.hpp"
#include "Physics/CollisionMesh.hpp"
#include "Core3D/Camera.hpp"
#include "Weapon/Weapon.hpp"

#include <memory>

namespace Entity {

/**
 * @brief Local player entity with input handling and weapon management.
 * 
 * Extends Character with:
 * - Camera synchronization
 * - Input-driven movement
 * - Weapon system integration
 * - Character model management
 */
class Player : public Character {
public:
    Player() = default;

    /**
     * @brief Initialize camera parameters.
     * @param camera The camera to synchronize with.
     */
    void Init(Core3D::Camera &camera);

    /**
     * @brief Initialize the character model.
     * @param scene The scene graph.
     * @param type Character type to use.
     * @param visible Whether to show the model.
     */
    void InitModel(Scene::SceneGraph &scene,
                   CharacterType type = CharacterType::FBI,
                   bool visible = true);

    /**
     * @brief Find spawn point and place the character.
     * @param camera The camera to update.
     * @param mesh The collision mesh for ground detection.
     */
    void SpawnOnMap(Core3D::Camera &camera, const Physics::CollisionMesh &mesh);

    /**
     * @brief Respawn the player (reset health and return to spawn).
     * @param camera The camera to update.
     * @param mesh The collision mesh for ground detection.
     * @param spawnPosition Team spawn position.
     */
    void Respawn(Core3D::Camera &camera,
                 const Physics::CollisionMesh &mesh,
                 const glm::vec3& spawnPosition);

    /**
     * @brief Per-frame update: input, movement, physics, camera, weapon.
     * @param dt Delta time in seconds.
     * @param camera The camera to update.
     * @param mesh The collision mesh.
     */
    void Update(float dt, Core3D::Camera &camera, const Physics::CollisionMesh &mesh);

    /**
     * @brief Equip a weapon.
     * @param weapon The weapon to equip.
     * @param scene The scene graph.
     */
    void EquipWeapon(std::unique_ptr<Weapon::Weapon> weapon, Scene::SceneGraph &scene);

    /** @brief Get the currently equipped weapon (may be nullptr). */
    Weapon::Weapon *GetWeapon() const { return m_Weapon.get(); }

    // ── Character Model Access ────────────────────────────────────────────

    CharacterModel &GetCharacterModel() { return m_CharacterModel; }
    const CharacterModel &GetCharacterModel() const { return m_CharacterModel; }

    /**
     * @brief Switch to a different character type.
     * @param scene The scene graph.
     * @param type The new character type.
     */
    void SwitchCharacter(Scene::SceneGraph &scene, CharacterType type);

    void ToggleModelVisibility() { m_CharacterModel.ToggleVisibility(); }
    void SetModelVisible(bool visible) { m_CharacterModel.SetVisible(visible); }
    bool IsModelVisible() const { return m_CharacterModel.IsVisible(); }

    /** @brief Check if the player is currently moving. */
    bool IsWalking() const { return m_IsWalking; }

private:
    std::unique_ptr<Weapon::Weapon> m_Weapon;
    CharacterModel m_CharacterModel;
    bool m_IsWalking = false;

    float m_NormalSpeed = 5.0f;  ///< Normal movement speed
    float m_CrouchSpeed = 3.0f;  ///< Crouching movement speed
};

} // namespace Entity

#endif // CS_ENTITY_PLAYER_HPP
