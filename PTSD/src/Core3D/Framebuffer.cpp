#include "Core3D/Framebuffer.hpp"
#include "Util/Logger.hpp"

namespace Core3D {

Framebuffer::Framebuffer(int width, int height, bool depthOnly)
    : m_Width(width), m_Height(height), m_DepthOnly(depthOnly) {
    Create();
}

Framebuffer::Framebuffer(Framebuffer &&other) {
    m_FBO = other.m_FBO;
    m_ColorTexture = other.m_ColorTexture;
    m_DepthTexture = other.m_DepthTexture;
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_DepthOnly = other.m_DepthOnly;
    other.m_FBO = 0;
    other.m_ColorTexture = 0;
    other.m_DepthTexture = 0;
}

Framebuffer::~Framebuffer() {
    Destroy();
}

Framebuffer &Framebuffer::operator=(Framebuffer &&other) {
    if (this != &other) {
        Destroy();
        m_FBO = other.m_FBO;
        m_ColorTexture = other.m_ColorTexture;
        m_DepthTexture = other.m_DepthTexture;
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_DepthOnly = other.m_DepthOnly;
        other.m_FBO = 0;
        other.m_ColorTexture = 0;
        other.m_DepthTexture = 0;
    }
    return *this;
}

void Framebuffer::Create() {
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // Depth attachment (always created)
    glGenTextures(1, &m_DepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           m_DepthTexture, 0);

    if (m_DepthOnly) {
        // No color buffer for shadow maps
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    } else {
        // Color attachment
        glGenTextures(1, &m_ColorTexture);
        glBindTexture(GL_TEXTURE_2D, m_ColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_ColorTexture, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Destroy() {
    if (m_ColorTexture != 0) {
        glDeleteTextures(1, &m_ColorTexture);
        m_ColorTexture = 0;
    }
    if (m_DepthTexture != 0) {
        glDeleteTextures(1, &m_DepthTexture);
        m_DepthTexture = 0;
    }
    if (m_FBO != 0) {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }
}

void Framebuffer::Bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, m_Width, m_Height);
}

void Framebuffer::Unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(int width, int height) {
    m_Width = width;
    m_Height = height;
    Destroy();
    Create();
}

} // namespace Core3D
