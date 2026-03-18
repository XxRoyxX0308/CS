#include "Render/ForwardRenderer.hpp"
#include "Scene/Skybox.hpp"
#include "config.hpp"
#include "Util/Logger.hpp"

#include <chrono>

namespace Render {

ForwardRenderer::ForwardRenderer()
    : m_ShadowMap(PTSD_Config::SHADOW_MAP_SIZE),
      m_ShadowsEnabled(PTSD_Config::ENABLE_SHADOWS),
      m_UsePBR(PTSD_Config::ENABLE_PBR) {
    InitPrograms();
    InitLightsUBO();
    CacheUniformLocations();

    // Pre-allocate buffers
    m_VisibleNodes.reserve(1024);
    m_FlattenBuffer.reserve(1024);
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

void ForwardRenderer::CacheUniformLocations() {
    // Cache Phong uniform locations
    GLuint phongId = m_PhongProgram->GetId();
    m_PhongUniforms.lightSpaceMatrix = glGetUniformLocation(phongId, "lightSpaceMatrix");
    m_PhongUniforms.shadowEnabled = glGetUniformLocation(phongId, "shadowEnabled");
    m_PhongUniforms.materialDiffuse = glGetUniformLocation(phongId, "material_diffuse");
    m_PhongUniforms.materialSpecular = glGetUniformLocation(phongId, "material_specular");
    m_PhongUniforms.materialAmbient = glGetUniformLocation(phongId, "material_ambient");
    m_PhongUniforms.materialShininess = glGetUniformLocation(phongId, "material_shininess");
    m_PhongUniforms.hasDiffuseMap = glGetUniformLocation(phongId, "hasDiffuseMap");
    m_PhongUniforms.hasSpecularMap = glGetUniformLocation(phongId, "hasSpecularMap");
    m_PhongUniforms.hasNormalMap = glGetUniformLocation(phongId, "hasNormalMap");

    // Cache PBR uniform locations
    GLuint pbrId = m_PBRProgram->GetId();
    m_PBRUniforms.lightSpaceMatrix = glGetUniformLocation(pbrId, "lightSpaceMatrix");
    m_PBRUniforms.shadowEnabled = glGetUniformLocation(pbrId, "shadowEnabled");
    m_PBRUniforms.pbrAlbedo = glGetUniformLocation(pbrId, "pbr_albedo");
    m_PBRUniforms.pbrMetallic = glGetUniformLocation(pbrId, "pbr_metallic");
    m_PBRUniforms.pbrRoughness = glGetUniformLocation(pbrId, "pbr_roughness");
    m_PBRUniforms.pbrAO = glGetUniformLocation(pbrId, "pbr_ao");
    m_PBRUniforms.hasAlbedoMap = glGetUniformLocation(pbrId, "hasAlbedoMap");
    m_PBRUniforms.pbrHasNormalMap = glGetUniformLocation(pbrId, "hasNormalMap");
    m_PBRUniforms.hasMetallicMap = glGetUniformLocation(pbrId, "hasMetallicMap");
    m_PBRUniforms.hasRoughnessMap = glGetUniformLocation(pbrId, "hasRoughnessMap");
    m_PBRUniforms.hasAOMap = glGetUniformLocation(pbrId, "hasAOMap");

    // Shadow pass uniform
    m_ShadowModelLoc = glGetUniformLocation(
        m_ShadowMap.GetShadowProgram().GetId(), "model");
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
    auto frameStart = std::chrono::high_resolution_clock::now();
    m_Stats = RenderStats{}; // Reset stats

    // Update frustum from camera
    const auto &camera = scene.GetCamera();
    glm::mat4 viewProj = camera.GetProjectionMatrix() * camera.GetViewMatrix();
    m_Frustum.Update(viewProj);

    // 1. Shadow pass
    if (m_ShadowsEnabled && scene.HasDirectionalLight()) {
        ShadowPass(scene);
    }

    // 2. Geometry pass
    GeometryPass(scene);

    // 3. Skybox pass
    SkyboxPass(scene);

    auto frameEnd = std::chrono::high_resolution_clock::now();
    m_Stats.renderTimeMs = std::chrono::duration<float, std::milli>(
        frameEnd - frameStart).count();
}

void ForwardRenderer::FrustumCull(
    Scene::SceneGraph &scene,
    std::vector<Scene::SceneNode *> &outVisibleNodes) {

    auto cullStart = std::chrono::high_resolution_clock::now();
    outVisibleNodes.clear();

    // Reuse flatten buffer (avoids allocation)
    m_FlattenBuffer.clear();
    scene.FlattenTreeInto(m_FlattenBuffer);

    m_Stats.totalNodes = static_cast<uint32_t>(m_FlattenBuffer.size());

    for (const auto &nodePtr : m_FlattenBuffer) {
        Scene::SceneNode *node = nodePtr.get();
        if (!node->GetDrawable()) continue;

        // Always-visible nodes bypass culling
        if (node->IsAlwaysVisible()) {
            outVisibleNodes.push_back(node);
            continue;
        }

        // Frustum culling
        if (m_FrustumCullingEnabled && node->HasAABB()) {
            const Core3D::AABB &worldAABB = node->GetWorldAABB();
            if (!m_Frustum.ContainsAABB(worldAABB)) {
                m_Stats.culledNodes++;
                continue;
            }
        }

        outVisibleNodes.push_back(node);
    }

    m_Stats.visibleNodes = static_cast<uint32_t>(outVisibleNodes.size());

    auto cullEnd = std::chrono::high_resolution_clock::now();
    m_Stats.frustumCullTimeMs = std::chrono::duration<float, std::milli>(
        cullEnd - cullStart).count();
}

void ForwardRenderer::ShadowPass(Scene::SceneGraph &scene) {
    m_ShadowMap.BeginShadowPass(scene.GetDirectionalLight(), m_SceneRadius);

    // Cull nodes for shadow pass (use same frustum - could optimize with light frustum)
    FrustumCull(scene, m_VisibleNodes);

    for (Scene::SceneNode *node : m_VisibleNodes) {
        const glm::mat4 &model = node->GetWorldTransform();
        glUniformMatrix4fv(m_ShadowModelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Draw with shadow shader (just depth)
        node->GetDrawable()->Draw(Core3D::Matrices3D{});
        m_Stats.drawCalls++;
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

    // Choose shader program and uniforms
    Core::Program &program = m_UsePBR ? *m_PBRProgram : *m_PhongProgram;
    const UniformLocations &uniforms = m_UsePBR ? m_PBRUniforms : m_PhongUniforms;
    program.Bind();

    // Upload lights
    Scene::LightsUBO lightsData = scene.BuildLightsUBO();
    UploadLights(lightsData);

    // Upload shadow uniforms (using cached locations)
    if (m_ShadowsEnabled && scene.HasDirectionalLight()) {
        m_ShadowMap.BindShadowTexture(8);
        if (uniforms.lightSpaceMatrix >= 0) {
            glUniformMatrix4fv(uniforms.lightSpaceMatrix, 1, GL_FALSE,
                               glm::value_ptr(m_ShadowMap.GetLightSpaceMatrix()));
        }
        if (uniforms.shadowEnabled >= 0) {
            glUniform1i(uniforms.shadowEnabled, 1);
        }
    } else {
        if (uniforms.shadowEnabled >= 0) {
            glUniform1i(uniforms.shadowEnabled, 0);
        }
    }

    // Set default material properties (using cached locations)
    if (m_UsePBR) {
        if (m_PBRUniforms.pbrAlbedo >= 0)
            glUniform3f(m_PBRUniforms.pbrAlbedo, 0.8f, 0.8f, 0.8f);
        if (m_PBRUniforms.pbrMetallic >= 0)
            glUniform1f(m_PBRUniforms.pbrMetallic, 0.0f);
        if (m_PBRUniforms.pbrRoughness >= 0)
            glUniform1f(m_PBRUniforms.pbrRoughness, 0.5f);
        if (m_PBRUniforms.pbrAO >= 0)
            glUniform1f(m_PBRUniforms.pbrAO, 1.0f);
        if (m_PBRUniforms.hasAlbedoMap >= 0)
            glUniform1i(m_PBRUniforms.hasAlbedoMap, 0);
        if (m_PBRUniforms.pbrHasNormalMap >= 0)
            glUniform1i(m_PBRUniforms.pbrHasNormalMap, 0);
        if (m_PBRUniforms.hasMetallicMap >= 0)
            glUniform1i(m_PBRUniforms.hasMetallicMap, 0);
        if (m_PBRUniforms.hasRoughnessMap >= 0)
            glUniform1i(m_PBRUniforms.hasRoughnessMap, 0);
        if (m_PBRUniforms.hasAOMap >= 0)
            glUniform1i(m_PBRUniforms.hasAOMap, 0);
    } else {
        if (m_PhongUniforms.materialDiffuse >= 0)
            glUniform3f(m_PhongUniforms.materialDiffuse, 0.8f, 0.8f, 0.8f);
        if (m_PhongUniforms.materialSpecular >= 0)
            glUniform3f(m_PhongUniforms.materialSpecular, 1.0f, 1.0f, 1.0f);
        if (m_PhongUniforms.materialAmbient >= 0)
            glUniform3f(m_PhongUniforms.materialAmbient, 0.1f, 0.1f, 0.1f);
        if (m_PhongUniforms.materialShininess >= 0)
            glUniform1f(m_PhongUniforms.materialShininess, 32.0f);
        if (m_PhongUniforms.hasDiffuseMap >= 0)
            glUniform1i(m_PhongUniforms.hasDiffuseMap, 0);
        if (m_PhongUniforms.hasSpecularMap >= 0)
            glUniform1i(m_PhongUniforms.hasSpecularMap, 0);
        if (m_PhongUniforms.hasNormalMap >= 0)
            glUniform1i(m_PhongUniforms.hasNormalMap, 0);
    }

    // Frustum cull and render visible nodes
    FrustumCull(scene, m_VisibleNodes);

    for (Scene::SceneNode *node : m_VisibleNodes) {
        // Use cached world transform and normal matrix
        const glm::mat4 &model = node->GetWorldTransform();
        const glm::mat4 &normalMatrix = node->GetNormalMatrix();

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
        m_Stats.drawCalls++;
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
