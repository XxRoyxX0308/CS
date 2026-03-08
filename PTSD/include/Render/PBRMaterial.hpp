#ifndef RENDER_PBR_MATERIAL_HPP
#define RENDER_PBR_MATERIAL_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core/Texture.hpp"
#include "Core/Program.hpp"

namespace Render {

/**
 * @brief PBR (Physically Based Rendering) material using metallic-roughness workflow.
 *
 * @code
 * Render::PBRMaterial pbr;
 * pbr.albedoMap = myAlbedoTex;
 * pbr.normalMap = myNormalTex;
 * pbr.metallic = 0.5f;
 * pbr.roughness = 0.3f;
 * pbr.Apply(program);
 * @endcode
 */
struct PBRMaterial {
    std::shared_ptr<Core::Texture> albedoMap;
    std::shared_ptr<Core::Texture> normalMap;
    std::shared_ptr<Core::Texture> metallicMap;
    std::shared_ptr<Core::Texture> roughnessMap;
    std::shared_ptr<Core::Texture> aoMap;

    glm::vec3 albedo{1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;

    bool hasAlbedoMap = false;
    bool hasNormalMap = false;
    bool hasMetallicMap = false;
    bool hasRoughnessMap = false;
    bool hasAOMap = false;

    /**
     * @brief Bind PBR textures and set uniforms on the shader.
     * @param program The shader program to set uniforms on.
     */
    void Apply(const Core::Program &program) const;
};

} // namespace Render

#endif
