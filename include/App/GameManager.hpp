#ifndef CS_APP_GAMEMANAGER_HPP
#define CS_APP_GAMEMANAGER_HPP

#include "Scene/SceneGraph.hpp"
#include "Render/ForwardRenderer.hpp"
#include "Physics/CollisionMesh.hpp"
#include "Entity/Player.hpp"
#include "Entity/RemotePlayer.hpp"
#include "Effects/BulletHole.hpp"
#include "Core3D/Model.hpp"

#include <memory>
#include <unordered_map>

namespace App {

/**
 * @brief Manages core game objects: scene, renderer, player, collision, effects.
 */
class GameManager {
public:
    GameManager();

    /** @brief Initialize the game (load map, setup player, etc.). */
    void Initialize();

    /** @brief Update player physics and movement. */
    void UpdatePlayer(float dt);

    /** @brief Render the scene. */
    void Render();

    /** @brief Draw bullet hole effects. */
    void DrawEffects();

    /** @brief Update bullet hole effects. */
    void UpdateEffects(float dt);

    /** @brief Spawn a bullet hole at the given position. */
    void SpawnBulletHole(const glm::vec3& pos, const glm::vec3& normal);

    /** @brief Cleanup resources. */
    void Cleanup();

    // ── Accessors ──

    Scene::SceneGraph& GetScene() { return m_Scene; }
    const Scene::SceneGraph& GetScene() const { return m_Scene; }

    Core3D::Camera& GetCamera() { return m_Scene.GetCamera(); }
    const Core3D::Camera& GetCamera() const { return m_Scene.GetCamera(); }

    Entity::Player& GetPlayer() { return m_Player; }
    const Entity::Player& GetPlayer() const { return m_Player; }

    const Physics::CollisionMesh& GetCollisionMesh() const { return m_CollisionMesh; }

    std::unordered_map<uint8_t, Entity::RemotePlayer>& GetRemotePlayers() { return m_RemotePlayers; }
    const std::unordered_map<uint8_t, Entity::RemotePlayer>& GetRemotePlayers() const { return m_RemotePlayers; }

    const Render::ForwardRenderer::RenderStats& GetRenderStats() const { return m_Renderer.GetStats(); }

    size_t GetBulletHoleCount() const { return m_BulletHoles.GetCount(); }

    /** @brief Switch player character type. */
    void SwitchPlayerCharacter(Entity::CharacterType type);

    /** @brief Get character type ID (0 = FBI, 1 = Terrorist). */
    uint8_t GetCharacterTypeId() const;

private:
    Scene::SceneGraph m_Scene;
    Render::ForwardRenderer m_Renderer;

    std::shared_ptr<Core3D::Model> m_MapModel;
    std::shared_ptr<Scene::SceneNode> m_MapNode;

    Physics::CollisionMesh m_CollisionMesh;
    Entity::Player m_Player;
    Effects::BulletHoleManager m_BulletHoles;

    std::unordered_map<uint8_t, Entity::RemotePlayer> m_RemotePlayers;
};

} // namespace App

#endif // CS_APP_GAMEMANAGER_HPP
