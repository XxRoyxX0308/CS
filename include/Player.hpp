#ifndef CS_PLAYER_HPP
#define CS_PLAYER_HPP

#include "Character.hpp"
#include "Core3D/Camera.hpp"
#include "MapCollider.hpp"

class Player : public Character {
public:
    Player() = default;

    // 初始化攝影機參數
    void Init(Core3D::Camera &camera);
    
    // 在地圖上找出生點並放置角色
    void SpawnOnMap(Core3D::Camera &camera, const MapCollider &collider);

    // 每幀更新（輸入 → 移動 → 物理 → 同步攝影機）
    void Update(float dt, Core3D::Camera &camera, const MapCollider &collider);
};

#endif // CS_PLAYER_HPP
