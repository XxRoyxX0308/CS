#include "Effects/BulletHole.hpp"

#include "Util/Logger.hpp"

#include "stb/stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

namespace Effects {

BulletHoleManager::~BulletHoleManager() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

void BulletHoleManager::Init() {
    InitQuadMesh();
    InitShader();

    // Load bullet hole texture
    std::string texPath = std::string(ASSETS_DIR) + "/effects/bullet_hole.png";

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *data = stbi_load(texPath.c_str(), &width, &height, &channels, 0);
    stbi_set_flip_vertically_on_load(false);

    if (data) {
        GLenum format = GL_RGBA;
        if (channels == 1) format = GL_RED;
        else if (channels == 3) format = GL_RGB;
        else if (channels == 4) format = GL_RGBA;

        m_Texture = std::make_shared<Core::Texture>(format, width, height, data, true);
        stbi_image_free(data);
        LOG_INFO("BulletHoleManager: Loaded texture {}x{} ({}ch)", width, height, channels);
    } else {
        LOG_ERROR("BulletHoleManager: Failed to load texture: {}", texPath);
    }
}

void BulletHoleManager::InitQuadMesh() {
    // A simple quad in the XY plane, centered at origin
    // Will be transformed to face the wall using the normal
    float halfSize = HOLE_SIZE * 0.5f;

    // clang-format off
    float vertices[] = {
        // Position (XYZ)          // TexCoord (UV)
        -halfSize, -halfSize, 0.0f,  0.0f, 0.0f,
         halfSize, -halfSize, 0.0f,  1.0f, 0.0f,
         halfSize,  halfSize, 0.0f,  1.0f, 1.0f,
        -halfSize, -halfSize, 0.0f,  0.0f, 0.0f,
         halfSize,  halfSize, 0.0f,  1.0f, 1.0f,
        -halfSize,  halfSize, 0.0f,  0.0f, 1.0f,
    };
    // clang-format on

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void BulletHoleManager::InitShader() {
    std::string vertPath = std::string(ASSETS_DIR) + "/shaders/Decal.vert";
    std::string fragPath = std::string(ASSETS_DIR) + "/shaders/Decal.frag";

    m_Shader = std::make_unique<Core::Program>(vertPath, fragPath);

    // Cache uniform locations
    m_LocModel = m_Shader->GetUniformLocation("model");
    m_LocViewProj = m_Shader->GetUniformLocation("viewProj");
    m_LocAlpha = m_Shader->GetUniformLocation("alpha");

    LOG_INFO("BulletHoleManager: Shader initialized");
}

void BulletHoleManager::SpawnHole(const glm::vec3 &position, const glm::vec3 &normal) {
    // Remove oldest hole if at max capacity
    if (m_Holes.size() >= MAX_HOLES) {
        m_Holes.erase(m_Holes.begin());
    }

    BulletHoleData hole;
    hole.position = position + normal * WALL_OFFSET; // Offset from wall
    hole.normal = normal;
    hole.age = 0.0f;
    hole.alpha = 1.0f;

    m_Holes.push_back(hole);
}

void BulletHoleManager::Update(float dt) {
    // Update each hole
    for (auto &hole : m_Holes) {
        hole.age += dt;

        // Calculate alpha based on age
        if (hole.age > LIFETIME) {
            float fadeProgress = (hole.age - LIFETIME) / FADE_DURATION;
            hole.alpha = 1.0f - std::min(fadeProgress, 1.0f);
        }
    }

    // Remove fully faded holes
    m_Holes.erase(
        std::remove_if(m_Holes.begin(), m_Holes.end(),
                       [](const BulletHoleData &h) {
                           return h.age > (LIFETIME + FADE_DURATION);
                       }),
        m_Holes.end());
}

void BulletHoleManager::Draw(const Core3D::Camera &camera,
                              const glm::mat4 &view,
                              const glm::mat4 &proj) {
    if (m_Holes.empty() || !m_Shader || !m_Texture) return;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth writing (but keep depth testing)
    glDepthMask(GL_FALSE);

    m_Shader->Bind();

    // Set view-projection matrix
    glm::mat4 viewProj = proj * view;
    glUniformMatrix4fv(m_LocViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));

    // Bind texture
    m_Texture->Bind(0);
    m_Shader->SetUniform1i("decalTexture", 0);

    glBindVertexArray(m_VAO);

    // Draw each hole
    for (const auto &hole : m_Holes) {
        // Build model matrix to orient quad to face away from wall
        glm::mat4 model(1.0f);
        model = glm::translate(model, hole.position);

        // Rotate to align with surface normal
        // The quad is in XY plane (facing +Z), we need to rotate it to face along normal
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        // Handle case where normal is parallel to up vector
        if (std::abs(glm::dot(hole.normal, up)) > 0.99f) {
            up = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        glm::vec3 right = glm::normalize(glm::cross(up, hole.normal));
        glm::vec3 actualUp = glm::cross(hole.normal, right);

        glm::mat4 rotation(1.0f);
        rotation[0] = glm::vec4(right, 0.0f);
        rotation[1] = glm::vec4(actualUp, 0.0f);
        rotation[2] = glm::vec4(hole.normal, 0.0f);

        model = model * rotation;

        glUniformMatrix4fv(m_LocModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1f(m_LocAlpha, hole.alpha);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    m_Shader->Unbind();

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace Effects
