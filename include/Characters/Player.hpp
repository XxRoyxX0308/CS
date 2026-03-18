#ifndef CS_PLAYER_HPP
#define CS_PLAYER_HPP

#include "Characters/Character.hpp"
#include "Collision/CollisionMesh.hpp"
#include "Core3D/Camera.hpp"
#include "Gun/Gun.hpp"

#include <memory>

class Player : public Character {
public:
    Player() = default;

    // 初始化攝影機參數
    void Init(Core3D::Camera &camera);

    // 在地圖上找出生點並放置角色
    void SpawnOnMap(Core3D::Camera &camera, const Collision::CollisionMesh &mesh);

    // 每幀更新（輸入 → 移動 → 物理 → 同步攝影機 → 武器）
    void Update(float dt, Core3D::Camera &camera, const Collision::CollisionMesh &mesh);

    // 裝備武器
    void EquipGun(std::unique_ptr<Gun::Gun> gun, Scene::SceneGraph &scene);

    // 取得目前武器（可能為 nullptr）
    Gun::Gun *GetGun() const { return m_Gun.get(); }

private:
    std::unique_ptr<Gun::Gun> m_Gun;
};

#endif // CS_PLAYER_HPP
