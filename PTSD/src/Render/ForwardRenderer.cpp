#include "Render/ForwardRenderer.hpp"
#include "Scene/Skybox.hpp"
#include "config.hpp"
#include "Util/Logger.hpp"

namespace Render {

ForwardRenderer::ForwardRenderer()
    : m_ShadowMap(PTSD_Config::SHADOW_MAP_SIZE),
      m_ShadowsEnabled(PTSD_Config::ENABLE_SHADOWS),
      m_UsePBR(PTSD_Config::ENABLE_PBR) {
    InitPrograms();
    InitLightsUBO();
}

void ForwardRenderer::InitPrograms() {
    m_PhongProgram = std::make_unique<Core::Program>(
        PTSD_ASSETS_DIR "/shaders/Phong.vert",
        PTSD_ASSETS_DIR "/shaders/Phong.frag");

    m_PBRProgram = std::make_unique<Core::Program>(
        PTSD_ASSETS_DIR "/shaders/PBR.vert",
        PTSD_ASSETS_DIR "/shaders/PBR.frag");

    // Setup texture sampler uniforms for Phong
    m_PhongProgram->Bind();
    glUniform1i(glGetUniformLocation(m_PhongProgram->GetId(), "texture_diffuse0"), 0);
    glUniform1i(glGetUniformLocation(m_PhongProgram->GetId(), "texture_specular0"), 1);
    glUniform1i(glGetUniformLocation(m_PhongProgram->GetId(), "texture_normal0"), 2);
    glUniform1i(glGetUniformLocation(m_PhongProgram->GetId(), "shadowMap"), 8);

    // Setup texture sampler uniforms for PBR
    m_PBRProgram->Bind();
    glUniform1i(glGetUniformLocation(m_PBRProgram->GetId(), "texture_diffuse0"), 0);
    glUniform1i(glGetUniformLocation(m_PBRProgram->GetId(), "texture_normal0"), 1);
    glUniform1i(glGetUniformLocation(m_PBRProgram->GetId(), "texture_metallic0"), 2);
    glUniform1i(glGetUniformLocation(m_PBRProgram->GetId(), "texture_roughness0"), 3);
    glUniform1i(glGetUniformLocation(m_PBRProgram->GetId(), "texture_ao0"), 4);
    glUniform1i(glGetUniformLocation(m_PBRProgram->GetId(), "shadowMap"), 8);
}

void ForwardRenderer::InitLightsUBO() {
    // Matrices UBO at binding point 0
    glGenBuffers(1, &m_MatricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_MatricesUBO);
    glBufferData(GL_UNIFORM_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(Core3D::Matrices3D)),
                 nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_MatricesUBO);

    // Lights UBO at binding point 1
    glGenBuffers(1, &m_LightsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_LightsUBO);
    glBufferData(GL_UNIFORM_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(Scene::LightsUBO)),
                 nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_LightsUBO);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind UBO block indices for both programs
    auto bindUBOs = [this](const Core::Program &prog) {
        GLuint matricesIdx = glGetUniformBlockIndex(prog.GetId(), "Matrices3D");
        if (matricesIdx != GL_INVALID_INDEX)
            glUniformBlockBinding(prog.GetId(), matricesIdx, 0);

        GLuint lightsIdx = glGetUniformBlockIndex(prog.GetId(), "Lights");
        if (lightsIdx != GL_INVALID_INDEX)
            glUniformBlockBinding(prog.GetId(), lightsIdx, 1);
    };

    bindUBOs(*m_PhongProgram);
    bindUBOs(*m_PBRProgram);
}

void ForwardRenderer::Render(Scene::SceneGraph &scene) {
    // 1. Shadow pass
    if (m_ShadowsEnabled && scene.HasDirectionalLight()) {
        ShadowPass(scene);
    }

    // 2. Geometry pass
    GeometryPass(scene);

    // 3. Skybox pass
    SkyboxPass(scene);
}

void ForwardRenderer::ShadowPass(Scene::SceneGraph &scene) {
    m_ShadowMap.BeginShadowPass(scene.GetDirectionalLight(), m_SceneRadius);

    auto nodes = scene.FlattenTree();
    for (const auto &node : nodes) {
        if (!node->GetDrawable()) continue;

        glm::mat4 model = node->GetWorldTransform();
        GLint modelLoc = glGetUniformLocation(
            m_ShadowMap.GetShadowProgram().GetId(), "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Draw with shadow shader (just depth)
        node->GetDrawable()->Draw(Core3D::Matrices3D{});
    }

    m_ShadowMap.EndShadowPass();
}

void ForwardRenderer::GeometryPass(Scene::SceneGraph &scene) {
    // 確保繪製到預設 Framebuffer（螢幕），並重新綁定 UBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_MatricesUBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_LightsUBO);

    const auto &camera = scene.GetCamera();
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix();

    // Choose shader program
    Core::Program &program = m_UsePBR ? *m_PBRProgram : *m_PhongProgram;
    program.Bind();

    // Upload lights
    Scene::LightsUBO lightsData = scene.BuildLightsUBO();
    UploadLights(lightsData);

    // Upload shadow uniforms
    if (m_ShadowsEnabled && scene.HasDirectionalLight()) {
        m_ShadowMap.BindShadowTexture(8);
        GLint lsmLoc = glGetUniformLocation(program.GetId(), "lightSpaceMatrix");
        glUniformMatrix4fv(lsmLoc, 1, GL_FALSE,
                           glm::value_ptr(m_ShadowMap.GetLightSpaceMatrix()));
        GLint shadowEnabledLoc = glGetUniformLocation(program.GetId(),
                                                       "shadowEnabled");
        if (shadowEnabledLoc >= 0) glUniform1i(shadowEnabledLoc, 1);
    } else {
        GLint shadowEnabledLoc = glGetUniformLocation(program.GetId(),
                                                       "shadowEnabled");
        if (shadowEnabledLoc >= 0) glUniform1i(shadowEnabledLoc, 0);
    }

    // Set default material properties (used when no Material is attached)
    if (m_UsePBR) {
        GLint loc;
        loc = glGetUniformLocation(program.GetId(), "pbr_albedo");
        if (loc >= 0) glUniform3f(loc, 0.8f, 0.8f, 0.8f);
        loc = glGetUniformLocation(program.GetId(), "pbr_metallic");
        if (loc >= 0) glUniform1f(loc, 0.0f);
        loc = glGetUniformLocation(program.GetId(), "pbr_roughness");
        if (loc >= 0) glUniform1f(loc, 0.5f);
        loc = glGetUniformLocation(program.GetId(), "pbr_ao");
        if (loc >= 0) glUniform1f(loc, 1.0f);
        loc = glGetUniformLocation(program.GetId(), "hasAlbedoMap");
        if (loc >= 0) glUniform1i(loc, 0);
        loc = glGetUniformLocation(program.GetId(), "hasNormalMap");
        if (loc >= 0) glUniform1i(loc, 0);
        loc = glGetUniformLocation(program.GetId(), "hasMetallicMap");
        if (loc >= 0) glUniform1i(loc, 0);
        loc = glGetUniformLocation(program.GetId(), "hasRoughnessMap");
        if (loc >= 0) glUniform1i(loc, 0);
        loc = glGetUniformLocation(program.GetId(), "hasAOMap");
        if (loc >= 0) glUniform1i(loc, 0);
    } else {
        GLint loc;
        loc = glGetUniformLocation(program.GetId(), "material_diffuse");
        if (loc >= 0) glUniform3f(loc, 0.8f, 0.8f, 0.8f);
        loc = glGetUniformLocation(program.GetId(), "material_specular");
        if (loc >= 0) glUniform3f(loc, 1.0f, 1.0f, 1.0f);
        loc = glGetUniformLocation(program.GetId(), "material_ambient");
        if (loc >= 0) glUniform3f(loc, 0.1f, 0.1f, 0.1f);
        loc = glGetUniformLocation(program.GetId(), "material_shininess");
        if (loc >= 0) glUniform1f(loc, 32.0f);
        loc = glGetUniformLocation(program.GetId(), "hasDiffuseMap");
        if (loc >= 0) glUniform1i(loc, 0);
        loc = glGetUniformLocation(program.GetId(), "hasSpecularMap");
        if (loc >= 0) glUniform1i(loc, 0);
        loc = glGetUniformLocation(program.GetId(), "hasNormalMap");
        if (loc >= 0) glUniform1i(loc, 0);
    }

    // Render all nodes
    auto nodes = scene.FlattenTree();
    for (const auto &node : nodes) {
        if (!node->GetDrawable()) continue;

        glm::mat4 model = node->GetWorldTransform();
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(model));

        Core3D::Matrices3D matrices;
        matrices.m_Model = model;
        matrices.m_View = view;
        matrices.m_Projection = projection;
        matrices.m_Normal = normalMatrix;
        matrices.m_CameraPos = glm::vec4(camera.GetPosition(), 1.0f);

        UploadMatrices(program, matrices);

        // Apply material if available
        if (node->GetMaterial()) {
            node->GetMaterial()->Apply(program);
        }

        node->GetDrawable()->Draw(matrices);
    }
}

void ForwardRenderer::SkyboxPass(Scene::SceneGraph &scene) {
    if (scene.GetSkybox()) {
        scene.GetSkybox()->Draw(scene.GetCamera().GetViewMatrix(),
                                 scene.GetCamera().GetProjectionMatrix());
    }
}

void ForwardRenderer::UploadMatrices(const Core::Program & /*program*/,
                                      const Core3D::Matrices3D &matrices) {
    glBindBuffer(GL_UNIFORM_BUFFER, m_MatricesUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0,
                    static_cast<GLsizeiptr>(sizeof(Core3D::Matrices3D)),
                    &matrices);
}

void ForwardRenderer::UploadLights(const Scene::LightsUBO &lightsData) {
    glBindBuffer(GL_UNIFORM_BUFFER, m_LightsUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0,
                    static_cast<GLsizeiptr>(sizeof(Scene::LightsUBO)),
                    &lightsData);
}

} // namespace Render
