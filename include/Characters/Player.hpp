#ifndef CS_PLAYER_HPP
#define CS_PLAYER_HPP

#include "Characters/Character.hpp"
#include "Characters/CharacterModel.hpp"
#include "Collision/CollisionMesh.hpp"
#include "Core3D/Camera.hpp"
#include "Gun/Gun.hpp"

#include <memory>

class Player : public Character {
public:
    Player() = default;

    // 初始化攝影機參數
    void Init(Core3D::Camera &camera);

    /**
     * @brief Initialize the character model.
     * @param scene The scene graph.
     * @param type Character type to use.
     * @param visible Whether to show the model (for debugging/development).
     */
    void InitModel(Scene::SceneGraph &scene,
                   Characters::CharacterType type = Characters::CharacterType::FBI,
                   bool visible = true);

    // 在地圖上找出生點並放置角色
    void SpawnOnMap(Core3D::Camera &camera, const Collision::CollisionMesh &mesh);

    // 每幀更新（輸入 → 移動 → 物理 → 同步攝影機 → 武器）
    void Update(float dt, Core3D::Camera &camera, const Collision::CollisionMesh &mesh);

    // 裝備武器
    void EquipGun(std::unique_ptr<Gun::Gun> gun, Scene::SceneGraph &scene);

    // 取得目前武器（可能為 nullptr）
    Gun::Gun *GetGun() const { return m_Gun.get(); }

    // ── Character Model Access ──
    Characters::CharacterModel &GetCharacterModel() { return m_CharacterModel; }
    const Characters::CharacterModel &GetCharacterModel() const { return m_CharacterModel; }

    /**
     * @brief Switch to a different character type.
     * @param scene The scene graph.
     * @param type The new character type.
     */
    void SwitchCharacter(Scene::SceneGraph &scene, Characters::CharacterType type);

    /**
     * @brief Toggle character model visibility.
     */
    void ToggleModelVisibility() { m_CharacterModel.ToggleVisibility(); }

    /**
     * @brief Set character model visibility.
     */
    void SetModelVisible(bool visible) { m_CharacterModel.SetVisible(visible); }

    /**
     * @brief Check if character model is visible.
     */
    bool IsModelVisible() const { return m_CharacterModel.IsVisible(); }

    /**
     * @brief Check if the player is currently moving.
     */
    bool IsWalking() const { return m_IsWalking; }

private:
    std::unique_ptr<Gun::Gun> m_Gun;
    Characters::CharacterModel m_CharacterModel;
    bool m_IsWalking = false;
};

#endif // CS_PLAYER_HPP
