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
    CacheUniformLocations();
}

ForwardRenderer::~ForwardRenderer() {
    if (m_MatricesUBO != 0) glDeleteBuffers(1, &m_MatricesUBO);
    if (m_LightsUBO != 0) glDeleteBuffers(1, &m_LightsUBO);
}

void ForwardRenderer::InitPrograms() {
    m_PhongProgram = std::make_unique<Core::Program>(
        PTSD_ASSETS_DIR "/shaders/Phong.vert",
        PTSD_ASSETS_DIR "/shaders/Phong.frag");

    m_PBRProgram = std::make_unique<Core::Program>(
        PTSD_ASSETS_DIR "/shaders/PBR.vert",
        PTSD_ASSETS_DIR "/shaders/PBR.frag");

    // Setup texture sampler uniforms for Phong (done once at init)
    m_PhongProgram->Bind();
    m_PhongProgram->SetUniform1i("texture_diffuse0", 0);
    m_PhongProgram->SetUniform1i("texture_specular0", 1);
    m_PhongProgram->SetUniform1i("texture_normal0", 2);
    m_PhongProgram->SetUniform1i("shadowMap", 8);

    // Setup texture sampler uniforms for PBR (done once at init)
    m_PBRProgram->Bind();
    m_PBRProgram->SetUniform1i("texture_diffuse0", 0);
    m_PBRProgram->SetUniform1i("texture_normal0", 1);
    m_PBRProgram->SetUniform1i("texture_metallic0", 2);
    m_PBRProgram->SetUniform1i("texture_roughness0", 3);
    m_PBRProgram->SetUniform1i("texture_ao0", 4);
    m_PBRProgram->SetUniform1i("shadowMap", 8);
}

void ForwardRenderer::CacheUniformLocations() {
    // Cache Phong shader uniform locations
    m_PhongLoc.lightSpaceMatrix = m_PhongProgram->GetUniformLocation("lightSpaceMatrix");
    m_PhongLoc.shadowEnabled = m_PhongProgram->GetUniformLocation("shadowEnabled");
    m_PhongLoc.material_diffuse = m_PhongProgram->GetUniformLocation("material_diffuse");
    m_PhongLoc.material_specular = m_PhongProgram->GetUniformLocation("material_specular");
    m_PhongLoc.material_ambient = m_PhongProgram->GetUniformLocation("material_ambient");
    m_PhongLoc.material_shininess = m_PhongProgram->GetUniformLocation("material_shininess");
    m_PhongLoc.hasDiffuseMap = m_PhongProgram->GetUniformLocation("hasDiffuseMap");
    m_PhongLoc.hasSpecularMap = m_PhongProgram->GetUniformLocation("hasSpecularMap");
    m_PhongLoc.hasNormalMap = m_PhongProgram->GetUniformLocation("hasNormalMap");

    // Cache PBR shader uniform locations
    m_PBRLoc.lightSpaceMatrix = m_PBRProgram->GetUniformLocation("lightSpaceMatrix");
    m_PBRLoc.shadowEnabled = m_PBRProgram->GetUniformLocation("shadowEnabled");
    m_PBRLoc.pbr_albedo = m_PBRProgram->GetUniformLocation("pbr_albedo");
    m_PBRLoc.pbr_metallic = m_PBRProgram->GetUniformLocation("pbr_metallic");
    m_PBRLoc.pbr_roughness = m_PBRProgram->GetUniformLocation("pbr_roughness");
    m_PBRLoc.pbr_ao = m_PBRProgram->GetUniformLocation("pbr_ao");
    m_PBRLoc.hasAlbedoMap = m_PBRProgram->GetUniformLocation("hasAlbedoMap");
    m_PBRLoc.hasNormalMap = m_PBRProgram->GetUniformLocation("hasNormalMap");
    m_PBRLoc.hasMetallicMap = m_PBRProgram->GetUniformLocation("hasMetallicMap");
    m_PBRLoc.hasRoughnessMap = m_PBRProgram->GetUniformLocation("hasRoughnessMap");
    m_PBRLoc.hasAOMap = m_PBRProgram->GetUniformLocation("hasAOMap");

    // Cache shadow shader model location
    m_ShadowModelLoc = m_ShadowMap.GetShadowProgram().GetUniformLocation("model");
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
    auto bindUBOs = [](const Core::Program &prog) {
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
    m_Stats = {};

    // Update frustum from camera
    if (m_FrustumCullingEnabled) {
        scene.UpdateFrustum();
    }

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

    // For shadow pass, we render all visible nodes (no frustum culling
    // because light's view is different from camera's view)
    const auto &nodes = scene.FlattenTree();
    for (const auto &node : nodes) {
        if (!node->GetDrawable()) continue;

        glm::mat4 model = node->GetWorldTransform();
        glUniformMatrix4fv(m_ShadowModelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Draw with shadow shader (just depth)
        node->GetDrawable()->Draw(Core3D::Matrices3D{});
    }

    m_ShadowMap.EndShadowPass();
}

void ForwardRenderer::GeometryPass(Scene::SceneGraph &scene) {
    // Bind default framebuffer and UBOs
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

    // Upload shadow uniforms using cached locations
    if (m_ShadowsEnabled && scene.HasDirectionalLight()) {
        m_ShadowMap.BindShadowTexture(8);
        GLint lsmLoc = m_UsePBR ? m_PBRLoc.lightSpaceMatrix : m_PhongLoc.lightSpaceMatrix;
        GLint shadowEnabledLoc = m_UsePBR ? m_PBRLoc.shadowEnabled : m_PhongLoc.shadowEnabled;

        if (lsmLoc >= 0) {
            glUniformMatrix4fv(lsmLoc, 1, GL_FALSE,
                               glm::value_ptr(m_ShadowMap.GetLightSpaceMatrix()));
        }
        if (shadowEnabledLoc >= 0) {
            glUniform1i(shadowEnabledLoc, 1);
        }
    } else {
        GLint shadowEnabledLoc = m_UsePBR ? m_PBRLoc.shadowEnabled : m_PhongLoc.shadowEnabled;
        if (shadowEnabledLoc >= 0) {
            glUniform1i(shadowEnabledLoc, 0);
        }
    }

    // Set default material properties
    SetDefaultMaterialUniforms(program);

    // Get visible nodes (with optional frustum culling)
    const std::vector<std::shared_ptr<Scene::SceneNode>> *nodesPtr;
    if (m_FrustumCullingEnabled) {
        nodesPtr = &scene.FlattenTreeCulled(scene.GetFrustum());
        const auto &stats = scene.GetCullStats();
        m_Stats.totalNodes = stats.totalNodes;
        m_Stats.visibleNodes = stats.visibleNodes;
        m_Stats.culledNodes = stats.culledNodes;
    } else {
        nodesPtr = &scene.FlattenTree();
        m_Stats.totalNodes = nodesPtr->size();
        m_Stats.visibleNodes = nodesPtr->size();
    }

    const auto &nodes = *nodesPtr;

    // TODO: Material sorting can be added here if m_MaterialSortingEnabled
    // For now, render in tree order

    // Render all visible nodes
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
        ++m_Stats.drawCalls;
    }
}

void ForwardRenderer::SetDefaultMaterialUniforms(Core::Program &program) {
    if (m_UsePBR) {
        if (m_PBRLoc.pbr_albedo >= 0) glUniform3f(m_PBRLoc.pbr_albedo, 0.8f, 0.8f, 0.8f);
        if (m_PBRLoc.pbr_metallic >= 0) glUniform1f(m_PBRLoc.pbr_metallic, 0.0f);
        if (m_PBRLoc.pbr_roughness >= 0) glUniform1f(m_PBRLoc.pbr_roughness, 0.5f);
        if (m_PBRLoc.pbr_ao >= 0) glUniform1f(m_PBRLoc.pbr_ao, 1.0f);
        if (m_PBRLoc.hasAlbedoMap >= 0) glUniform1i(m_PBRLoc.hasAlbedoMap, 0);
        if (m_PBRLoc.hasNormalMap >= 0) glUniform1i(m_PBRLoc.hasNormalMap, 0);
        if (m_PBRLoc.hasMetallicMap >= 0) glUniform1i(m_PBRLoc.hasMetallicMap, 0);
        if (m_PBRLoc.hasRoughnessMap >= 0) glUniform1i(m_PBRLoc.hasRoughnessMap, 0);
        if (m_PBRLoc.hasAOMap >= 0) glUniform1i(m_PBRLoc.hasAOMap, 0);
    } else {
        if (m_PhongLoc.material_diffuse >= 0) glUniform3f(m_PhongLoc.material_diffuse, 0.8f, 0.8f, 0.8f);
        if (m_PhongLoc.material_specular >= 0) glUniform3f(m_PhongLoc.material_specular, 1.0f, 1.0f, 1.0f);
        if (m_PhongLoc.material_ambient >= 0) glUniform3f(m_PhongLoc.material_ambient, 0.1f, 0.1f, 0.1f);
        if (m_PhongLoc.material_shininess >= 0) glUniform1f(m_PhongLoc.material_shininess, 32.0f);
        if (m_PhongLoc.hasDiffuseMap >= 0) glUniform1i(m_PhongLoc.hasDiffuseMap, 0);
        if (m_PhongLoc.hasSpecularMap >= 0) glUniform1i(m_PhongLoc.hasSpecularMap, 0);
        if (m_PhongLoc.hasNormalMap >= 0) glUniform1i(m_PhongLoc.hasNormalMap, 0);
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
