#ifndef CORE3D_MODEL_HPP
#define CORE3D_MODEL_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/Drawable3D.hpp"
#include "Core3D/Mesh.hpp"
#include "Core/Program.hpp"
#include "Core/Texture.hpp"

#include <assimp/scene.h>

namespace Core3D {

/**
 * @brief Loads a 3D model from file using Assimp and stores its meshes.
 *
 * Supports 40+ formats including OBJ, FBX, glTF, DAE, 3DS, etc.
 * Automatically extracts textures (diffuse, specular, normal) from the model.
 *
 * @code
 * auto model = std::make_shared<Core3D::Model>("assets/models/backpack.obj");
 *
 * // In render loop:
 * Core3D::Matrices3D matrices;
 * matrices.m_Model = ...;
 * matrices.m_View = camera.GetViewMatrix();
 * matrices.m_Projection = camera.GetProjectionMatrix();
 * model->Draw(matrices);
 * @endcode
 */
class Model : public Drawable3D {
public:
    /**
     * @brief Load a model from file.
     * @param path File path to the 3D model.
     * @param flipUVs Whether to flip texture coordinates vertically (default: false).
     */
    explicit Model(const std::string &path, bool flipUVs = false);

    /**
     * @brief Create a model from pre-built meshes (for procedural geometry).
     * @param meshes Vector of Mesh objects (moved in).
     */
    explicit Model(std::vector<Mesh> meshes);

    void Draw(const Matrices3D &data) override;
    glm::vec3 GetBoundingBox() const override;

    /** @brief Draw with a specific shader program (for shadow pass, etc.) */
    void Draw(const Core::Program &program) const;

    const std::vector<Mesh> &GetMeshes() const { return m_Meshes; }
    const std::string &GetPath() const { return m_Directory; }

    /**
     * @brief Set the shader program used for default Draw(Matrices3D).
     */
    void SetProgram(std::shared_ptr<Core::Program> program) {
        m_Program = std::move(program);
    }

private:
    void LoadModel(const std::string &path, bool flipUVs);
    void ProcessNode(aiNode *node, const aiScene *scene);
    Mesh ProcessMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<MeshTexture> LoadMaterialTextures(
        aiMaterial *mat, aiTextureType type, TextureType typeName);

    std::vector<Mesh> m_Meshes;
    std::string m_Directory;
    std::vector<MeshTexture> m_LoadedTextures; ///< Cache to avoid reloading

    std::shared_ptr<Core::Program> m_Program;

    // Uniform buffer for Matrices3D
    GLuint m_MatricesUBO = 0;
    void InitUBO();
};

} // namespace Core3D

#endif
