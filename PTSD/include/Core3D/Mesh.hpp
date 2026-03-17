#ifndef CORE3D_MESH_HPP
#define CORE3D_MESH_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core/Program.hpp"
#include "Core/Texture.hpp"

namespace Core3D {

/**
 * @brief Maximum number of bones that can influence a single vertex.
 */
constexpr int MAX_BONE_INFLUENCE = 4;

/**
 * @brief 3D vertex with position, normal, UV, tangent, and bone data.
 */
struct Vertex3D {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 texCoords{0.0f};
    glm::vec3 tangent{1.0f, 0.0f, 0.0f};
    glm::vec3 bitangent{0.0f, 0.0f, 1.0f};
    int boneIDs[MAX_BONE_INFLUENCE] = {-1, -1, -1, -1};
    float boneWeights[MAX_BONE_INFLUENCE] = {0.0f, 0.0f, 0.0f, 0.0f};
};

/**
 * @brief Texture type for material slots.
 */
enum class TextureType {
    DIFFUSE,
    SPECULAR,
    NORMAL,
    METALLIC,
    ROUGHNESS,
    AO,
    EMISSIVE,
    HEIGHT
};

/**
 * @brief A texture bound to a specific material slot.
 */
struct MeshTexture {
    std::shared_ptr<Core::Texture> texture;
    TextureType type;
    std::string path; ///< For deduplication via AssetStore
};

/**
 * @brief Per-mesh PBR material factors extracted from model files.
 *
 * Used as shader fallback values when texture maps are not present
 * (e.g. glTF models that use baseColorFactor instead of textures).
 */
struct MeshMaterial {
    glm::vec3 albedo{0.8f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
};

/**
 * @brief A single mesh containing vertices, indices, and textures.
 *
 * This is the fundamental 3D rendering unit. A Model may contain
 * multiple Mesh instances.
 *
 * @code
 * // Usually created by Model, but can be used standalone:
 * std::vector<Core3D::Vertex3D> vertices = { ... };
 * std::vector<unsigned int> indices = { ... };
 * std::vector<Core3D::MeshTexture> textures = { ... };
 * Core3D::Mesh mesh(vertices, indices, textures);
 * mesh.Draw(program);
 * @endcode
 */
class Mesh {
public:
    Mesh(const std::vector<Vertex3D> &vertices,
         const std::vector<unsigned int> &indices,
         const std::vector<MeshTexture> &textures,
         const MeshMaterial &material = {});

    Mesh(const Mesh &) = delete;
    Mesh(Mesh &&other);
    ~Mesh();

    Mesh &operator=(const Mesh &) = delete;
    Mesh &operator=(Mesh &&other);

    /** @brief Draw this mesh using the given shader program. */
    void Draw(const Core::Program &program) const;

    /** @brief Draw this mesh using a raw OpenGL program ID
     *  (for cases where the renderer already bound the program). */
    void Draw(GLuint programId) const;

    const std::vector<Vertex3D> &GetVertices() const { return m_Vertices; }
    const std::vector<unsigned int> &GetIndices() const { return m_Indices; }
    const std::vector<MeshTexture> &GetTextures() const { return m_Textures; }

    /** @brief Compute the AABB of this mesh's vertices. */
    glm::vec3 ComputeBoundingBox() const;

private:
    void SetupMesh();

    std::vector<Vertex3D> m_Vertices;
    std::vector<unsigned int> m_Indices;
    std::vector<MeshTexture> m_Textures;
    MeshMaterial m_Material;

    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;
};

} // namespace Core3D

#endif
