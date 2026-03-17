#include "Core3D/Mesh.hpp"
#include "Util/Logger.hpp"

namespace Core3D {

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
    // Bind textures to slots based on their type
    unsigned int diffuseNr = 0;
    unsigned int specularNr = 0;
    unsigned int normalNr = 0;
    unsigned int metallicNr = 0;
    unsigned int roughnessNr = 0;
    unsigned int aoNr = 0;

    bool hasDiffuse = false, hasSpecular = false, hasNormal = false;
    bool hasMetallic = false, hasRoughness = false, hasAO = false;

    for (unsigned int i = 0; i < m_Textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);

        std::string name;
        switch (m_Textures[i].type) {
        case TextureType::DIFFUSE:
            name = "texture_diffuse" + std::to_string(diffuseNr++);
            hasDiffuse = true;
            break;
        case TextureType::SPECULAR:
            name = "texture_specular" + std::to_string(specularNr++);
            hasSpecular = true;
            break;
        case TextureType::NORMAL:
            name = "texture_normal" + std::to_string(normalNr++);
            hasNormal = true;
            break;
        case TextureType::METALLIC:
            name = "texture_metallic" + std::to_string(metallicNr++);
            hasMetallic = true;
            break;
        case TextureType::ROUGHNESS:
            name = "texture_roughness" + std::to_string(roughnessNr++);
            hasRoughness = true;
            break;
        case TextureType::AO:
            name = "texture_ao" + std::to_string(aoNr++);
            hasAO = true;
            break;
        default:
            name = "texture_other" + std::to_string(i);
            break;
        }

        GLint loc = glGetUniformLocation(program.GetId(), name.c_str());
        if (loc >= 0) {
            glUniform1i(loc, static_cast<GLint>(i));
        }

        glBindTexture(GL_TEXTURE_2D, m_Textures[i].texture->GetTextureId());
    }

    // Tell the shader which texture maps are available
    GLuint pid = program.GetId();
    auto setFlag = [pid](const char *name, bool val) {
        GLint loc = glGetUniformLocation(pid, name);
        if (loc >= 0) glUniform1i(loc, val ? 1 : 0);
    };
    setFlag("hasDiffuseMap", hasDiffuse);
    setFlag("hasSpecularMap", hasSpecular);
    setFlag("hasNormalMap", hasNormal);
    setFlag("hasAlbedoMap", hasDiffuse);  // PBR uses albedo = diffuse
    setFlag("hasMetallicMap", hasMetallic);
    setFlag("hasRoughnessMap", hasRoughness);
    setFlag("hasAOMap", hasAO);

    // Upload per-mesh material factors as shader fallbacks
    auto setVec3 = [pid](const char *name, const glm::vec3 &v) {
        GLint loc = glGetUniformLocation(pid, name);
        if (loc >= 0) glUniform3f(loc, v.x, v.y, v.z);
    };
    auto setFloat = [pid](const char *name, float v) {
        GLint loc = glGetUniformLocation(pid, name);
        if (loc >= 0) glUniform1f(loc, v);
    };
    setVec3("pbr_albedo", m_Material.albedo);
    setFloat("pbr_metallic", m_Material.metallic);
    setFloat("pbr_roughness", m_Material.roughness);
    setFloat("pbr_ao", m_Material.ao);
    // Phong fallbacks
    setVec3("material_diffuse", m_Material.albedo);
    setVec3("material_ambient", m_Material.albedo * 0.1f);

    // Draw the mesh
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_Indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Reset
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::Draw(GLuint programId) const {
    // Bind textures to slots based on their type (same logic, raw program ID)
    unsigned int diffuseNr = 0;
    unsigned int specularNr = 0;
    unsigned int normalNr = 0;
    unsigned int metallicNr = 0;
    unsigned int roughnessNr = 0;
    unsigned int aoNr = 0;

    bool hasDiffuse = false, hasSpecular = false, hasNormal = false;
    bool hasMetallic = false, hasRoughness = false, hasAO = false;

    for (unsigned int i = 0; i < m_Textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);

        std::string name;
        switch (m_Textures[i].type) {
        case TextureType::DIFFUSE:
            name = "texture_diffuse" + std::to_string(diffuseNr++);
            hasDiffuse = true;
            break;
        case TextureType::SPECULAR:
            name = "texture_specular" + std::to_string(specularNr++);
            hasSpecular = true;
            break;
        case TextureType::NORMAL:
            name = "texture_normal" + std::to_string(normalNr++);
            hasNormal = true;
            break;
        case TextureType::METALLIC:
            name = "texture_metallic" + std::to_string(metallicNr++);
            hasMetallic = true;
            break;
        case TextureType::ROUGHNESS:
            name = "texture_roughness" + std::to_string(roughnessNr++);
            hasRoughness = true;
            break;
        case TextureType::AO:
            name = "texture_ao" + std::to_string(aoNr++);
            hasAO = true;
            break;
        default:
            name = "texture_other" + std::to_string(i);
            break;
        }

        GLint loc = glGetUniformLocation(programId, name.c_str());
        if (loc >= 0) {
            glUniform1i(loc, static_cast<GLint>(i));
        }

        glBindTexture(GL_TEXTURE_2D, m_Textures[i].texture->GetTextureId());
    }

    // Tell the shader which texture maps are available
    auto setFlag = [programId](const char *name, bool val) {
        GLint loc = glGetUniformLocation(programId, name);
        if (loc >= 0) glUniform1i(loc, val ? 1 : 0);
    };
    setFlag("hasDiffuseMap", hasDiffuse);
    setFlag("hasSpecularMap", hasSpecular);
    setFlag("hasNormalMap", hasNormal);
    setFlag("hasAlbedoMap", hasDiffuse);
    setFlag("hasMetallicMap", hasMetallic);
    setFlag("hasRoughnessMap", hasRoughness);
    setFlag("hasAOMap", hasAO);

    // Upload per-mesh material factors as shader fallbacks
    auto setVec3 = [programId](const char *name, const glm::vec3 &v) {
        GLint loc = glGetUniformLocation(programId, name);
        if (loc >= 0) glUniform3f(loc, v.x, v.y, v.z);
    };
    auto setFloat = [programId](const char *name, float v) {
        GLint loc = glGetUniformLocation(programId, name);
        if (loc >= 0) glUniform1f(loc, v);
    };
    setVec3("pbr_albedo", m_Material.albedo);
    setFloat("pbr_metallic", m_Material.metallic);
    setFloat("pbr_roughness", m_Material.roughness);
    setFloat("pbr_ao", m_Material.ao);
    setVec3("material_diffuse", m_Material.albedo);
    setVec3("material_ambient", m_Material.albedo * 0.1f);

    // Draw the mesh
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_Indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Reset
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
