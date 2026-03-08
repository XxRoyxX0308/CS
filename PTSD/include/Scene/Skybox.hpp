#ifndef SCENE_SKYBOX_HPP
#define SCENE_SKYBOX_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/Cubemap.hpp"
#include "Core/Program.hpp"

namespace Scene {

/**
 * @brief Renders a cubemap skybox around the scene.
 *
 * The skybox is drawn after all geometry with depth set to GL_LEQUAL.
 * The view matrix has its translation stripped so the skybox always
 * appears at infinity.
 *
 * @code
 * auto skybox = std::make_shared<Scene::Skybox>(std::vector<std::string>{
 *     "assets/skybox/right.jpg", "assets/skybox/left.jpg",
 *     "assets/skybox/top.jpg",   "assets/skybox/bottom.jpg",
 *     "assets/skybox/front.jpg", "assets/skybox/back.jpg"
 * });
 * scene.SetSkybox(skybox);
 * @endcode
 */
class Skybox {
public:
    /**
     * @brief Construct a skybox from 6 face images.
     * @param faces Image paths: +X, -X, +Y, -Y, +Z, -Z.
     */
    explicit Skybox(const std::vector<std::string> &faces);

    /**
     * @brief Draw the skybox.
     * @param view Camera view matrix.
     * @param projection Camera projection matrix.
     */
    void Draw(const glm::mat4 &view, const glm::mat4 &projection);

    const Core3D::Cubemap &GetCubemap() const { return m_Cubemap; }

private:
    void InitCubeVAO();
    void InitProgram();

    Core3D::Cubemap m_Cubemap;
    std::unique_ptr<Core::Program> m_Program;
    GLuint m_CubeVAO = 0;
    GLuint m_CubeVBO = 0;
};

} // namespace Scene

#endif
