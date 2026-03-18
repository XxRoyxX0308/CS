#ifndef CS_EFFECTS_BULLET_HOLE_HPP
#define CS_EFFECTS_BULLET_HOLE_HPP

#include "pch.hpp"

#include "Core/Program.hpp"
#include "Core/Texture.hpp"
#include "Core3D/Camera.hpp"
#include "Scene/SceneGraph.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Effects {

/**
 * @brief A single bullet hole decal.
 */
struct BulletHoleData {
    glm::vec3 position;   ///< World position of the bullet hole
    glm::vec3 normal;     ///< Surface normal at hit point
    float age = 0.0f;     ///< Time since creation (seconds)
    float alpha = 1.0f;   ///< Current alpha (for fade out)
};

/**
 * @brief Manager for bullet hole decals in the scene.
 *
 * Creates bullet hole decals at hit points, maintains their lifecycle,
 * and renders them with fade-out effect.
 *
 * Usage:
 * @code
 * Effects::BulletHoleManager holes;
 * holes.Init();
 *
 * // When bullet hits wall:
 * holes.SpawnHole(hitPoint, hitNormal);
 *
 * // Each frame:
 * holes.Update(dt);
 * holes.Draw(camera, viewMatrix, projMatrix);
 * @endcode
 */
class BulletHoleManager {
public:
    BulletHoleManager() = default;
    ~BulletHoleManager();

    /**
     * @brief Initialize the manager, load texture and shader.
     */
    void Init();

    /**
     * @brief Spawn a new bullet hole at the given position.
     * @param position World position of the hit point.
     * @param normal Surface normal at hit point.
     */
    void SpawnHole(const glm::vec3 &position, const glm::vec3 &normal);

    /**
     * @brief Update all bullet holes (age, fade, removal).
     * @param dt Delta time in seconds.
     */
    void Update(float dt);

    /**
     * @brief Draw all bullet holes.
     * @param camera The scene camera.
     * @param view View matrix.
     * @param proj Projection matrix.
     */
    void Draw(const Core3D::Camera &camera,
              const glm::mat4 &view,
              const glm::mat4 &proj);

    /**
     * @brief Get the number of active bullet holes.
     */
    size_t GetCount() const { return m_Holes.size(); }

private:
    void InitQuadMesh();
    void InitShader();

    std::vector<BulletHoleData> m_Holes;

    // Rendering resources
    std::shared_ptr<Core::Texture> m_Texture;
    std::unique_ptr<Core::Program> m_Shader;
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;

    // Cached uniform locations
    GLint m_LocModel = -1;
    GLint m_LocViewProj = -1;
    GLint m_LocAlpha = -1;

    // Configuration
    static constexpr float HOLE_SIZE = 0.6f;      ///< Size of bullet hole (meters)
    static constexpr float LIFETIME = 10.0f;       ///< Seconds before fade starts
    static constexpr float FADE_DURATION = 1.0f;   ///< Fade out duration (seconds)
    static constexpr float WALL_OFFSET = 0.001f;   ///< Offset from wall to prevent z-fighting
    static constexpr size_t MAX_HOLES = 100;       ///< Maximum number of holes
};

} // namespace Effects

#endif // CS_EFFECTS_BULLET_HOLE_HPP
