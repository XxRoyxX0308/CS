#ifndef SCENE_LIGHT_HPP
#define SCENE_LIGHT_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Scene {

/**
 * @brief Base light properties shared by all light types.
 */
struct LightBase {
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
};

/**
 * @brief Directional light (sun-like, parallel rays).
 *
 * @code
 * Scene::DirectionalLight sun;
 * sun.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
 * sun.color = glm::vec3(1.0f, 0.95f, 0.9f);
 * sun.intensity = 1.5f;
 * @endcode
 */
struct DirectionalLight : LightBase {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
};

/**
 * @brief Point light (omnidirectional, attenuates with distance).
 *
 * @code
 * Scene::PointLight lamp;
 * lamp.position = glm::vec3(2.0f, 3.0f, 1.0f);
 * lamp.color = glm::vec3(1.0f, 0.8f, 0.6f);
 * lamp.constant = 1.0f;
 * lamp.linear = 0.09f;
 * lamp.quadratic = 0.032f;
 * @endcode
 */
struct PointLight : LightBase {
    glm::vec3 position{0.0f};
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
};

/**
 * @brief Spot light (cone-shaped, like a flashlight).
 *
 * @code
 * Scene::SpotLight flashlight;
 * flashlight.position = camera.GetPosition();
 * flashlight.direction = camera.GetFront();
 * flashlight.cutOff = glm::cos(glm::radians(12.5f));
 * flashlight.outerCutOff = glm::cos(glm::radians(17.5f));
 * @endcode
 */
struct SpotLight : LightBase {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(17.5f));
};

// ====== GPU-side structs for UBO (std140 aligned) ======

/**
 * @brief std140-aligned directional light data for shader UBO.
 */
struct alignas(16) DirLightGPU {
    glm::vec4 direction{0.0f}; // xyz = dir, w = intensity
    glm::vec4 color{1.0f};    // xyz = color, w unused
};

/**
 * @brief std140-aligned point light data for shader UBO.
 */
struct alignas(16) PointLightGPU {
    glm::vec4 position{0.0f};  // xyz = pos, w = constant
    glm::vec4 color{1.0f};     // xyz = color, w = intensity
    glm::vec4 attenuation{0.0f}; // x = linear, y = quadratic, z/w unused
};

/**
 * @brief std140-aligned spot light data for shader UBO.
 */
struct alignas(16) SpotLightGPU {
    glm::vec4 position{0.0f};    // xyz = pos, w = constant
    glm::vec4 direction{0.0f};   // xyz = dir, w = cutOff
    glm::vec4 color{1.0f};       // xyz = color, w = intensity
    glm::vec4 attenuation{0.0f}; // x = linear, y = quadratic, z = outerCutOff, w unused
};

constexpr int GPU_MAX_POINT_LIGHTS = 16;
constexpr int GPU_MAX_SPOT_LIGHTS = 8;

/**
 * @brief Combined light UBO data for shader (std140 layout).
 */
struct LightsUBO {
    DirLightGPU dirLight;
    glm::ivec4 lightCounts{0}; // x = numPointLights, y = numSpotLights
    PointLightGPU pointLights[GPU_MAX_POINT_LIGHTS];
    SpotLightGPU spotLights[GPU_MAX_SPOT_LIGHTS];
};

/**
 * @brief Convert CPU-side light structs to GPU UBO data.
 */
inline DirLightGPU ToGPU(const DirectionalLight &light) {
    DirLightGPU gpu;
    gpu.direction = glm::vec4(light.direction, light.intensity);
    gpu.color = glm::vec4(light.color, 0.0f);
    return gpu;
}

inline PointLightGPU ToGPU(const PointLight &light) {
    PointLightGPU gpu;
    gpu.position = glm::vec4(light.position, light.constant);
    gpu.color = glm::vec4(light.color, light.intensity);
    gpu.attenuation = glm::vec4(light.linear, light.quadratic, 0.0f, 0.0f);
    return gpu;
}

inline SpotLightGPU ToGPU(const SpotLight &light) {
    SpotLightGPU gpu;
    gpu.position = glm::vec4(light.position, light.constant);
    gpu.direction = glm::vec4(light.direction, light.cutOff);
    gpu.color = glm::vec4(light.color, light.intensity);
    gpu.attenuation = glm::vec4(light.linear, light.quadratic,
                                 light.outerCutOff, 0.0f);
    return gpu;
}

} // namespace Scene

#endif
