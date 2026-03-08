#ifndef CORE3D_FRAMEBUFFER_HPP
#define CORE3D_FRAMEBUFFER_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Core3D {

/**
 * @brief Wraps an OpenGL Framebuffer Object (FBO).
 *
 * Used for off-screen rendering targets such as shadow maps
 * and post-processing effects.
 *
 * @code
 * // Create a depth-only FBO for shadow mapping:
 * Core3D::Framebuffer shadowFBO(2048, 2048, true);
 * shadowFBO.Bind();
 * // ... render depth pass ...
 * shadowFBO.Unbind();
 * GLuint depthTex = shadowFBO.GetDepthTexture();
 * @endcode
 */
class Framebuffer {
public:
    /**
     * @brief Create a framebuffer with optional color and depth attachments.
     * @param width Texture width in pixels.
     * @param height Texture height in pixels.
     * @param depthOnly If true, only a depth attachment is created (for shadow maps).
     */
    Framebuffer(int width, int height, bool depthOnly = false);
    Framebuffer(const Framebuffer &) = delete;
    Framebuffer(Framebuffer &&other);
    ~Framebuffer();

    Framebuffer &operator=(const Framebuffer &) = delete;
    Framebuffer &operator=(Framebuffer &&other);

    void Bind() const;
    void Unbind() const;

    /** @brief Resize the framebuffer and its attachments. */
    void Resize(int width, int height);

    GLuint GetColorTexture() const { return m_ColorTexture; }
    GLuint GetDepthTexture() const { return m_DepthTexture; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

private:
    void Create();
    void Destroy();

    GLuint m_FBO = 0;
    GLuint m_ColorTexture = 0;
    GLuint m_DepthTexture = 0;
    int m_Width;
    int m_Height;
    bool m_DepthOnly;
};

} // namespace Core3D

#endif
