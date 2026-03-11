#include "Core3D/Model.hpp"
#include "Core/TextureUtils.hpp"
#include "Util/Logger.hpp"
#include "config.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "../../lib/stb/stb_image.h"

namespace Core3D {

Model::Model(const std::string &path, bool flipUVs) {
    LoadModel(path, flipUVs);
    InitUBO();
}

Model::Model(std::vector<Mesh> meshes)
    : m_Meshes(std::move(meshes)) {
    InitUBO();
}

void Model::InitUBO() {
    glGenBuffers(1, &m_MatricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_MatricesUBO);
    glBufferData(GL_UNIFORM_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(Matrices3D)), nullptr,
                 GL_DYNAMIC_DRAW);
    // 注意：不在此處呼叫 glBindBufferBase！
    // 只有在 Model 自行管理 shader 時（Draw 的 m_Program 分支）
    // 才應綁定 UBO binding point 0，以免覆蓋 ForwardRenderer 的綁定。
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Model::LoadModel(const std::string &path, bool flipUVs) {
    Assimp::Importer importer;

    unsigned int flags = aiProcess_Triangulate |
                         aiProcess_GenSmoothNormals |
                         aiProcess_CalcTangentSpace;
    if (flipUVs) {
        flags |= aiProcess_FlipUVs;
    }

    const aiScene *scene = importer.ReadFile(path, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        LOG_ERROR("Assimp error: {}", importer.GetErrorString());
        return;
    }

    // Extract directory from path for texture loading
    m_Directory = path.substr(0, path.find_last_of("/\\"));

    ProcessNode(scene->mRootNode, scene);

    LOG_INFO("Model loaded: '{}' ({} meshes)", path, m_Meshes.size());
}

void Model::ProcessNode(aiNode *node, const aiScene *scene) {
    // Process all meshes in this node
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        m_Meshes.push_back(ProcessMesh(mesh, scene));
    }

    // Recurse into children
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene);
    }
}

Mesh Model::ProcessMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex3D> vertices;
    std::vector<unsigned int> indices;
    std::vector<MeshTexture> textures;

    vertices.reserve(mesh->mNumVertices);

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex3D vertex;

        vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                           mesh->mVertices[i].z};

        if (mesh->HasNormals()) {
            vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y,
                             mesh->mNormals[i].z};
        }

        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = {mesh->mTextureCoords[0][i].x,
                                mesh->mTextureCoords[0][i].y};
        }

        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y,
                              mesh->mTangents[i].z};
            vertex.bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y,
                                mesh->mBitangents[i].z};
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Process materials
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        auto diffuseMaps = LoadMaterialTextures(
            material, aiTextureType_DIFFUSE, TextureType::DIFFUSE);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        auto specularMaps = LoadMaterialTextures(
            material, aiTextureType_SPECULAR, TextureType::SPECULAR);
        textures.insert(textures.end(), specularMaps.begin(),
                        specularMaps.end());

        auto normalMaps = LoadMaterialTextures(
            material, aiTextureType_NORMALS, TextureType::NORMAL);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // Also try height maps as normal maps (common in OBJ files)
        auto heightMaps = LoadMaterialTextures(
            material, aiTextureType_HEIGHT, TextureType::NORMAL);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        auto metallicMaps = LoadMaterialTextures(
            material, aiTextureType_METALNESS, TextureType::METALLIC);
        textures.insert(textures.end(), metallicMaps.begin(),
                        metallicMaps.end());

        auto roughnessMaps = LoadMaterialTextures(
            material, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::ROUGHNESS);
        textures.insert(textures.end(), roughnessMaps.begin(),
                        roughnessMaps.end());

        auto aoMaps = LoadMaterialTextures(
            material, aiTextureType_AMBIENT_OCCLUSION, TextureType::AO);
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

std::vector<MeshTexture> Model::LoadMaterialTextures(
    aiMaterial *mat, aiTextureType type, TextureType typeName) {
    std::vector<MeshTexture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string texPath = m_Directory + "/" + str.C_Str();

        // Check if already loaded
        bool skip = false;
        for (const auto &loaded : m_LoadedTextures) {
            if (loaded.path == texPath) {
                textures.push_back(loaded);
                skip = true;
                break;
            }
        }

        if (!skip) {
            stbi_set_flip_vertically_on_load(true);
            int width, height, nrChannels;
            unsigned char *data =
                stbi_load(texPath.c_str(), &width, &height, &nrChannels, 0);
            stbi_set_flip_vertically_on_load(false);

            if (data) {
                GLenum format = GL_RGB;
                if (nrChannels == 1) format = GL_RED;
                else if (nrChannels == 4) format = GL_RGBA;

                auto texture = std::make_shared<Core::Texture>(
                    static_cast<GLint>(format), width, height, data, true);
                stbi_image_free(data);

                // Override wrap mode to GL_REPEAT so that UV values
                // outside [0,1] tile correctly (e.g. Source-engine maps).
                GLuint tid = texture->GetTextureId();
                glBindTexture(GL_TEXTURE_2D, tid);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glBindTexture(GL_TEXTURE_2D, 0);

                MeshTexture meshTex;
                meshTex.texture = texture;
                meshTex.type = typeName;
                meshTex.path = texPath;

                textures.push_back(meshTex);
                m_LoadedTextures.push_back(meshTex);
            } else {
                LOG_WARN("Texture failed to load: '{}'", texPath);
                stbi_image_free(data);
            }
        }
    }

    return textures;
}

void Model::Draw(const Matrices3D &data) {
    if (m_Program) {
        // 若有設定專屬 shader，使用自帶的 UBO 和 shader 繪製
        glBindBuffer(GL_UNIFORM_BUFFER, m_MatricesUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0,
                        static_cast<GLsizeiptr>(sizeof(Matrices3D)), &data);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_MatricesUBO);

        m_Program->Bind();

        for (auto &mesh : m_Meshes) {
            mesh.Draw(*m_Program);
        }
    } else {
        // 沒有設定專屬 shader — 使用外部已綁定的 shader（例如 ForwardRenderer）
        // 取得目前 OpenGL 綁定的 program ID
        GLint currentProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        if (currentProgram == 0) {
            LOG_ERROR("Model::Draw: No shader program bound (neither "
                      "internal nor external).");
            return;
        }
        // 以目前綁定的 program 繪製所有 mesh
        for (auto &mesh : m_Meshes) {
            mesh.Draw(static_cast<GLuint>(currentProgram));
        }
    }
}

void Model::Draw(const Core::Program &program) const {
    for (const auto &mesh : m_Meshes) {
        mesh.Draw(program);
    }
}

glm::vec3 Model::GetBoundingBox() const {
    if (m_Meshes.empty()) return glm::vec3(0.0f);

    glm::vec3 totalMin(std::numeric_limits<float>::max());
    glm::vec3 totalMax(std::numeric_limits<float>::lowest());

    for (const auto &mesh : m_Meshes) {
        for (const auto &v : mesh.GetVertices()) {
            totalMin = glm::min(totalMin, v.position);
            totalMax = glm::max(totalMax, v.position);
        }
    }

    return totalMax - totalMin;
}

} // namespace Core3D
