#include "Render/ShadowMap.hpp"
#include "config.hpp"

namespace Render {

ShadowMap::ShadowMap(int resolution)
    : m_ShadowFBO(resolution, resolution, true),
      m_Resolution(resolution) {
    m_ShadowProgram = std::make_unique<Core::Program>(
        PTSD_ASSETS_DIR "/shaders/Shadow.vert",
        PTSD_ASSETS_DIR "/shaders/Shadow.frag");
}

void ShadowMap::BeginShadowPass(const Scene::DirectionalLight &dirLight,
                                 float sceneRadius) {
    // Save current viewport
    glGetIntegerv(GL_VIEWPORT, m_PrevViewport);

    // Compute light-space matrix
    glm::vec3 lightDir = glm::normalize(dirLight.direction);
    glm::vec3 lightPos = -lightDir * sceneRadius;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::ortho(
        -sceneRadius, sceneRadius,
        -sceneRadius, sceneRadius,
        0.1f, sceneRadius * 3.0f);
    m_LightSpaceMatrix = lightProjection * lightView;

    // Bind shadow FBO
    m_ShadowFBO.Bind();
    glClear(GL_DEPTH_BUFFER_BIT);

    // Use shadow shader
    m_ShadowProgram->Bind();
    GLint loc = glGetUniformLocation(m_ShadowProgram->GetId(),
                                      "lightSpaceMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m_LightSpaceMatrix));
}

void ShadowMap::EndShadowPass() {
    m_ShadowFBO.Unbind();
    // Restore viewport
    glViewport(m_PrevViewport[0], m_PrevViewport[1],
               m_PrevViewport[2], m_PrevViewport[3]);
}

void ShadowMap::BindShadowTexture(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ShadowFBO.GetDepthTexture());
}

} // namespace Render
