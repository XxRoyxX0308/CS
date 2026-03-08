#include "Scene/Skybox.hpp"
#include "config.hpp"

namespace Scene {

// clang-format off
static const float s_SkyboxVertices[] = {
    // positions (unit cube)
    -1.0f,  1.0f, -1.0f,   -1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,    1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,   -1.0f, -1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,    1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,    1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,    1.0f, -1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f,  1.0f
};
// clang-format on

Skybox::Skybox(const std::vector<std::string> &faces)
    : m_Cubemap(faces) {
    InitCubeVAO();
    InitProgram();
}

void Skybox::InitCubeVAO() {
    glGenVertexArrays(1, &m_CubeVAO);
    glGenBuffers(1, &m_CubeVBO);

    glBindVertexArray(m_CubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_SkyboxVertices), s_SkyboxVertices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          nullptr);

    glBindVertexArray(0);
}

void Skybox::InitProgram() {
    m_Program = std::make_unique<Core::Program>(
        PTSD_ASSETS_DIR "/shaders/Skybox.vert",
        PTSD_ASSETS_DIR "/shaders/Skybox.frag");

    m_Program->Bind();
    GLint loc = glGetUniformLocation(m_Program->GetId(), "skybox");
    glUniform1i(loc, 0);
}

void Skybox::Draw(const glm::mat4 &view, const glm::mat4 &projection) {
    // Strip translation from view matrix (skybox stays at infinity)
    glm::mat4 skyView = glm::mat4(glm::mat3(view));

    glDepthFunc(GL_LEQUAL);

    m_Program->Bind();

    GLint viewLoc = glGetUniformLocation(m_Program->GetId(), "view");
    GLint projLoc = glGetUniformLocation(m_Program->GetId(), "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(skyView));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    m_Cubemap.Bind(0);
    glBindVertexArray(m_CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}

} // namespace Scene
