#include "App/GameManager.hpp"
#include "App/CombatManager.hpp"
#include "Weapon/Assaults/HoneyBadger.hpp"
#include "Scene/Light.hpp"
#include "Util/Logger.hpp"

#include <SDL.h>
#include <string>

namespace App {

namespace {
    glm::mat4 BuildMapTransform() {
        constexpr float SCALE = 0.02f;
        glm::mat4 transform(1.0f);
        transform = glm::scale(transform, glm::vec3(SCALE));
        transform = glm::rotate(transform, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        return transform;
    }
}

GameManager::GameManager() = default;

void GameManager::Initialize() {
    LOG_TRACE("GameManager::Initialize");

    auto& camera = m_Scene.GetCamera();

    // ── 1. Camera & Player Setup ──
    m_Player.Init(camera);

    // ── 2. Directional Light (Sun) ──
    Scene::DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    sun.color = glm::vec3(1.0f, 0.98f, 0.92f);
    sun.intensity = 1.5f;
    m_Scene.SetDirectionalLight(sun);

    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    // ── 3. Load de_dust2 Map ──
    const std::string mapPath =
        std::string(ASSETS_DIR) + "/de_dust2-map/source/de_dust2.obj";
    LOG_INFO("Loading map: {}", mapPath);

    m_MapModel = std::make_shared<Core3D::Model>(mapPath, false);
    m_MapNode = std::make_shared<Scene::SceneNode>();

    constexpr float SCALE = 0.02f;
    m_MapNode->SetScale(glm::vec3(SCALE));
    m_MapNode->SetRotation(
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f)));
    m_MapNode->SetDrawable(m_MapModel);
    m_Scene.GetRoot()->AddChild(m_MapNode);

    m_Renderer.SetSceneRadius(80.0f);

    // ── 4. Build Map Collision ──
    glm::mat4 mapTransform = BuildMapTransform();
    m_CollisionMesh.Build(*m_MapModel, mapTransform);

    // ── 5. Initialize Character Model ──
    m_Player.InitModel(m_Scene, m_LocalCharacterType, false);

    // ── 6. Set Team Spawn Point ──
    CombatManager combatManager;
    const glm::vec3 spawnPos = combatManager.GetSpawnPoint(m_CollisionMesh, m_LocalCharacterType);
    m_Player.Respawn(camera, m_CollisionMesh, spawnPos);

    // ── 7. Equip Weapon ──
    auto gun = std::make_unique<Weapon::HoneyBadger>();
    m_Player.EquipWeapon(std::move(gun), m_Scene);

    // ── 8. Initialize Bullet Hole Effects ──
    m_BulletHoles.Init();

    LOG_TRACE("GameManager::Initialize complete");
}

void GameManager::UpdatePlayer(float dt) {
    auto& camera = m_Scene.GetCamera();
    m_Player.Update(dt, camera, m_CollisionMesh);
}

void GameManager::Render() {
    m_Renderer.Render(m_Scene);
}

void GameManager::DrawEffects() {
    auto& camera = m_Scene.GetCamera();
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();
    m_BulletHoles.Draw(camera, view, proj);
}

void GameManager::UpdateEffects(float dt) {
    m_BulletHoles.Update(dt);
}

void GameManager::SpawnBulletHole(const glm::vec3& pos, const glm::vec3& normal) {
    m_BulletHoles.SpawnHole(pos, normal);
}

void GameManager::Cleanup() {
    LOG_TRACE("GameManager::Cleanup");
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

void GameManager::SwitchPlayerCharacter(Entity::CharacterType type) {
    m_Player.SwitchCharacter(m_Scene, type);
}

uint8_t GameManager::GetCharacterTypeId() const {
    auto charType = m_Player.GetCharacterModel().GetCharacterType();
    return (charType == Entity::CharacterType::FBI) ? 0 : 1;
}

} // namespace App
