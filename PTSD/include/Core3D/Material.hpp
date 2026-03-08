#ifndef CORE3D_MATERIAL_HPP
#define CORE3D_MATERIAL_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core/Texture.hpp"
#include "Core/Program.hpp"

namespace Core3D {

/**
 * @brief Standard Blinn-Phong material with texture maps.
 *
 * @code
 * Core3D::Material mat;
 * mat.diffuseMap = myDiffuseTex;
 * mat.normalMap = myNormalTex;
 * mat.shininess = 64.0f;
 * mat.Apply(program);
 * @endcode
 */
struct Material {
    std::shared_ptr<Core::Texture> diffuseMap;
    std::shared_ptr<Core::Texture> specularMap;
    std::shared_ptr<Core::Texture> normalMap;

    glm::vec3 ambient{0.1f};
    glm::vec3 diffuse{0.8f};
    glm::vec3 specular{1.0f};
    float shininess = 32.0f;

    bool hasNormalMap = false;

    /**
     * @brief Bind material textures and set uniforms on the shader.
     * @param program The shader program to set uniforms on.
     */
    void Apply(const Core::Program &program) const {
        program.Bind();
        GLuint pid = program.GetId();

        glUniform3fv(glGetUniformLocation(pid, "material.ambient"), 1,
                     &ambient[0]);
        glUniform3fv(glGetUniformLocation(pid, "material.diffuse"), 1,
                     &diffuse[0]);
        glUniform3fv(glGetUniformLocation(pid, "material.specular"), 1,
                     &specular[0]);
        glUniform1f(glGetUniformLocation(pid, "material.shininess"),
                    shininess);

        int slot = 0;
        if (diffuseMap) {
            glActiveTexture(GL_TEXTURE0 + slot);
            diffuseMap->Bind(slot);
            glUniform1i(glGetUniformLocation(pid, "texture_diffuse0"), slot);
        }
        slot = 1;
        if (specularMap) {
            glActiveTexture(GL_TEXTURE0 + slot);
            specularMap->Bind(slot);
            glUniform1i(glGetUniformLocation(pid, "texture_specular0"), slot);
        }
        slot = 2;
        if (normalMap) {
            glActiveTexture(GL_TEXTURE0 + slot);
            normalMap->Bind(slot);
            glUniform1i(glGetUniformLocation(pid, "texture_normal0"), slot);
            glUniform1i(glGetUniformLocation(pid, "hasNormalMap"), 1);
        } else {
            glUniform1i(glGetUniformLocation(pid, "hasNormalMap"), 0);
        }
    }
};

} // namespace Core3D

#endif
