#include "Core3D/Mesh.hpp"
#include "Util/Logger.hpp"

namespace Core3D {

// ============================================================================
// Pre-computed static uniform names to avoid per-frame string allocation
// ============================================================================
namespace {

// Maximum number of textures per type (should be enough for most models)
constexpr unsigned int MAX_TEXTURES_PER_TYPE = 4;

// Pre-computed texture uniform names
const char *const TEXTURE_DIFFUSE_NAMES[] = {
    "texture_diffuse0", "texture_diffuse1", "texture_diffuse2", "texture_diffuse3"
};
const char *const TEXTURE_SPECULAR_NAMES[] = {
    "texture_specular0", "texture_specular1", "texture_specular2", "texture_specular3"
};
const char *const TEXTURE_NORMAL_NAMES[] = {
    "texture_normal0", "texture_normal1", "texture_normal2", "texture_normal3"
};
const char *const TEXTURE_METALLIC_NAMES[] = {
    "texture_metallic0", "texture_metallic1", "texture_metallic2", "texture_metallic3"
};
const char *const TEXTURE_ROUGHNESS_NAMES[] = {
    "texture_roughness0", "texture_roughness1", "texture_roughness2", "texture_roughness3"
};
const char *const TEXTURE_AO_NAMES[] = {
    "texture_ao0", "texture_ao1", "texture_ao2", "texture_ao3"
};

// Helper to get texture uniform name without allocation
const char *GetTextureUniformName(TextureType type, unsigned int index) {
    if (index >= MAX_TEXTURES_PER_TYPE) return nullptr;

    switch (type) {
    case TextureType::DIFFUSE:   return TEXTURE_DIFFUSE_NAMES[index];
    case TextureType::SPECULAR:  return TEXTURE_SPECULAR_NAMES[index];
    case TextureType::NORMAL:    return TEXTURE_NORMAL_NAMES[index];
    case TextureType::METALLIC:  return TEXTURE_METALLIC_NAMES[index];
    case TextureType::ROUGHNESS: return TEXTURE_ROUGHNESS_NAMES[index];
    case TextureType::AO:        return TEXTURE_AO_NAMES[index];
    default: return nullptr;
    }
}

// Inline helper for setting uniform flags
inline void SetUniformFlag(GLuint pid, GLint loc, bool val) {
    if (loc >= 0) glUniform1i(loc, val ? 1 : 0);
}

inline void SetUniformVec3(GLuint pid, GLint loc, const glm::vec3 &v) {
    if (loc >= 0) glUniform3f(loc, v.x, v.y, v.z);
}

inline void SetUniformFloat(GLuint pid, GLint loc, float v) {
    if (loc >= 0) glUniform1f(loc, v);
}

} // anonymous namespace

// ============================================================================
// Mesh Implementation
// ============================================================================

Mesh::Mesh(const std::vector<Vertex3D> &vertices,
           const std::vector<unsigned int> &indices,
           const std::vector<MeshTexture> &textures,
           const MeshMaterial &material)
    : m_Vertices(vertices), m_Indices(indices), m_Textures(textures),
      m_Material(material) {
    SetupMesh();
}

Mesh::Mesh(Mesh &&other) {
    m_Vertices = std::move(other.m_Vertices);
    m_Indices = std::move(other.m_Indices);
    m_Textures = std::move(other.m_Textures);
    m_Material = other.m_Material;
    m_VAO = other.m_VAO;
    m_VBO = other.m_VBO;
    m_EBO = other.m_EBO;
    other.m_VAO = 0;
    other.m_VBO = 0;
    other.m_EBO = 0;
}

Mesh::~Mesh() {
    if (m_VAO != 0) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO != 0) glDeleteBuffers(1, &m_VBO);
    if (m_EBO != 0) glDeleteBuffers(1, &m_EBO);
}

Mesh &Mesh::operator=(Mesh &&other) {
    if (this != &other) {
        if (m_VAO != 0) glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO != 0) glDeleteBuffers(1, &m_VBO);
        if (m_EBO != 0) glDeleteBuffers(1, &m_EBO);

        m_Vertices = std::move(other.m_Vertices);
        m_Indices = std::move(other.m_Indices);
        m_Textures = std::move(other.m_Textures);
        m_Material = other.m_Material;
        m_VAO = other.m_VAO;
        m_VBO = other.m_VBO;
        m_EBO = other.m_EBO;
        other.m_VAO = 0;
        other.m_VBO = 0;
        other.m_EBO = 0;
    }
    return *this;
}

void Mesh::SetupMesh() {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    // Upload vertex data (interleaved)
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_Vertices.size() * sizeof(Vertex3D)),
                 m_Vertices.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_Indices.size() * sizeof(unsigned int)),
                 m_Indices.data(), GL_STATIC_DRAW);

    // Vertex attribute layout (interleaved Vertex3D struct)
    // 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          reinterpret_cast<void *>(offsetof(Vertex3D, position)));

    // 1: normal (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          reinterpret_cast<void *>(offsetof(Vertex3D, normal)));

    // 2: texCoords (vec2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          reinterpret_cast<void *>(offsetof(Vertex3D, texCoords)));

    // 3: tangent (vec3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          reinterpret_cast<void *>(offsetof(Vertex3D, tangent)));

    // 4: bitangent (vec3)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          reinterpret_cast<void *>(offsetof(Vertex3D, bitangent)));

    // 5: boneIDs (ivec4)
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, MAX_BONE_INFLUENCE, GL_INT, sizeof(Vertex3D),
                           reinterpret_cast<void *>(offsetof(Vertex3D, boneIDs)));

    // 6: boneWeights (vec4)
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex3D),
                          reinterpret_cast<void *>(offsetof(Vertex3D, boneWeights)));

    glBindVertexArray(0);
}

void Mesh::Draw(const Core::Program &program) const {
    Draw(program.GetId());
}

void Mesh::Draw(GLuint programId) const {
    // Track texture counts per type
    unsigned int diffuseNr = 0;
    unsigned int specularNr = 0;
    unsigned int normalNr = 0;
    unsigned int metallicNr = 0;
    unsigned int roughnessNr = 0;
    unsigned int aoNr = 0;

    bool hasDiffuse = false, hasSpecular = false, hasNormal = false;
    bool hasMetallic = false, hasRoughness = false, hasAO = false;

    // Bind textures using pre-computed uniform names (no string allocation)
    for (unsigned int i = 0; i < m_Textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);

        const char *uniformName = nullptr;
        unsigned int *counter = nullptr;

        switch (m_Textures[i].type) {
        case TextureType::DIFFUSE:
            counter = &diffuseNr;
            hasDiffuse = true;
            break;
        case TextureType::SPECULAR:
            counter = &specularNr;
            hasSpecular = true;
            break;
        case TextureType::NORMAL:
            counter = &normalNr;
            hasNormal = true;
            break;
        case TextureType::METALLIC:
            counter = &metallicNr;
            hasMetallic = true;
            break;
        case TextureType::ROUGHNESS:
            counter = &roughnessNr;
            hasRoughness = true;
            break;
        case TextureType::AO:
            counter = &aoNr;
            hasAO = true;
            break;
        default:
            break;
        }

        if (counter != nullptr) {
            uniformName = GetTextureUniformName(m_Textures[i].type, *counter);
            (*counter)++;
        }

        if (uniformName != nullptr) {
            GLint loc = glGetUniformLocation(programId, uniformName);
            if (loc >= 0) {
                glUniform1i(loc, static_cast<GLint>(i));
            }
        }

        glBindTexture(GL_TEXTURE_2D, m_Textures[i].texture->GetTextureId());
    }

    // Set texture availability flags using cached locations
    // Note: These use glGetUniformLocation but with const char* - no allocation
    GLint loc;

    loc = glGetUniformLocation(programId, "hasDiffuseMap");
    SetUniformFlag(programId, loc, hasDiffuse);

    loc = glGetUniformLocation(programId, "hasSpecularMap");
    SetUniformFlag(programId, loc, hasSpecular);

    loc = glGetUniformLocation(programId, "hasNormalMap");
    SetUniformFlag(programId, loc, hasNormal);

    loc = glGetUniformLocation(programId, "hasAlbedoMap");
    SetUniformFlag(programId, loc, hasDiffuse);  // PBR uses albedo = diffuse

    loc = glGetUniformLocation(programId, "hasMetallicMap");
    SetUniformFlag(programId, loc, hasMetallic);

    loc = glGetUniformLocation(programId, "hasRoughnessMap");
    SetUniformFlag(programId, loc, hasRoughness);

    loc = glGetUniformLocation(programId, "hasAOMap");
    SetUniformFlag(programId, loc, hasAO);

    // Upload per-mesh material factors as shader fallbacks
    loc = glGetUniformLocation(programId, "pbr_albedo");
    SetUniformVec3(programId, loc, m_Material.albedo);

    loc = glGetUniformLocation(programId, "pbr_metallic");
    SetUniformFloat(programId, loc, m_Material.metallic);

    loc = glGetUniformLocation(programId, "pbr_roughness");
    SetUniformFloat(programId, loc, m_Material.roughness);

    loc = glGetUniformLocation(programId, "pbr_ao");
    SetUniformFloat(programId, loc, m_Material.ao);

    // Phong fallbacks
    loc = glGetUniformLocation(programId, "material_diffuse");
    SetUniformVec3(programId, loc, m_Material.albedo);

    loc = glGetUniformLocation(programId, "material_ambient");
    SetUniformVec3(programId, loc, m_Material.albedo * 0.1f);

    // Draw the mesh
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_Indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Reset active texture
    glActiveTexture(GL_TEXTURE0);
}

glm::vec3 Mesh::ComputeBoundingBox() const {
    if (m_Vertices.empty()) return glm::vec3(0.0f);

    glm::vec3 minPos(std::numeric_limits<float>::max());
    glm::vec3 maxPos(std::numeric_limits<float>::lowest());

    for (const auto &v : m_Vertices) {
        minPos = glm::min(minPos, v.position);
        maxPos = glm::max(maxPos, v.position);
    }

    return maxPos - minPos;
}

} // namespace Core3D
