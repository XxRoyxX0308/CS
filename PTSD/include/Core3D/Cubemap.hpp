#ifndef CORE3D_CUBEMAP_HPP
#define CORE3D_CUBEMAP_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Core3D {

/**
 * @brief Wraps an OpenGL GL_TEXTURE_CUBE_MAP.
 *
 * Used for skyboxes and environment mapping (IBL).
 *
 * @code
 * // Load from 6 individual face images:
 * Core3D::Cubemap skybox({
 *     "assets/skybox/right.jpg",
 *     "assets/skybox/left.jpg",
 *     "assets/skybox/top.jpg",
 *     "assets/skybox/bottom.jpg",
 *     "assets/skybox/front.jpg",
 *     "assets/skybox/back.jpg"
 * });
 * skybox.Bind(0);
 * @endcode
 */
class Cubemap {
public:
    /**
     * @brief Load a cubemap from 6 face image file paths.
     *
     * Order: +X, -X, +Y, -Y, +Z, -Z
     * (right, left, top, bottom, front, back).
     */
    explicit Cubemap(const std::vector<std::string> &faces);

    Cubemap(const Cubemap &) = delete;
    Cubemap(Cubemap &&other);
    ~Cubemap();

    Cubemap &operator=(const Cubemap &) = delete;
    Cubemap &operator=(Cubemap &&other);

    GLuint GetTextureId() const { return m_TextureId; }

    void Bind(int slot) const;
    void Unbind() const;

private:
    GLuint m_TextureId = 0;
};

} // namespace Core3D

#endif
