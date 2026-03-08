#version 410 core

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;
layout(location = 3) in vec4 FragPosLightSpace;
layout(location = 4) in mat3 TBN;

out vec4 FragColor;

// ====== Material textures ======
uniform sampler2D texture_diffuse0;
uniform sampler2D texture_specular0;
uniform sampler2D texture_normal0;
uniform sampler2D shadowMap;

// ====== Material properties (fallback if no texture) ======
uniform vec3 material_ambient;
uniform vec3 material_diffuse;
uniform vec3 material_specular;
uniform float material_shininess;
uniform bool hasDiffuseMap;
uniform bool hasSpecularMap;
uniform bool hasNormalMap;

// ====== Shadow control ======
uniform bool shadowEnabled;

// ====== Lights UBO ======
struct DirLight {
    vec4 direction; // xyz = dir, w = intensity
    vec4 color;     // xyz = color
};

struct PointLightData {
    vec4 position;    // xyz = pos, w = constant
    vec4 color;       // xyz = color, w = intensity
    vec4 attenuation; // x = linear, y = quadratic
};

struct SpotLightData {
    vec4 position;    // xyz = pos, w = constant
    vec4 direction;   // xyz = dir, w = cutOff
    vec4 color;       // xyz = color, w = intensity
    vec4 attenuation; // x = linear, y = quadratic, z = outerCutOff
};

layout(std140) uniform Lights {
    DirLight dirLight;
    ivec4 lightCounts; // x = numPoint, y = numSpot, z = hasDirLight
    PointLightData pointLights[16];
    SpotLightData spotLights[8];
};

layout(std140) uniform Matrices3D {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
    vec4 cameraPos;
};

// ====== Shadow calculation ======
float ShadowCalculation(vec4 fragPosLS, vec3 normal, vec3 lightDir) {
    if (!shadowEnabled) return 0.0;

    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Bias to reduce shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    // PCF (Percentage-Closer Filtering)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

// ====== Directional light ======
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffColor, vec3 specColor) {
    vec3 lightDir = normalize(-light.direction.xyz);
    float intensity = light.direction.w;

    float diff = max(dot(normal, lightDir), 0.0);

    // Blinn-Phong specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material_shininess);

    float shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);

    vec3 ambient = 0.1 * light.color.xyz * diffColor;
    vec3 diffuse = (1.0 - shadow) * diff * light.color.xyz * diffColor * intensity;
    vec3 specular = (1.0 - shadow) * spec * light.color.xyz * specColor * intensity;

    return ambient + diffuse + specular;
}

// ====== Point light ======
vec3 CalcPointLight(PointLightData light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor) {
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    float intensity = light.color.w;

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material_shininess);

    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (light.position.w +
                                light.attenuation.x * distance +
                                light.attenuation.y * distance * distance);

    vec3 ambient = 0.05 * light.color.xyz * diffColor;
    vec3 diffuse = diff * light.color.xyz * diffColor * intensity;
    vec3 specular = spec * light.color.xyz * specColor * intensity;

    return (ambient + diffuse + specular) * attenuation;
}

// ====== Spot light ======
vec3 CalcSpotLight(SpotLightData light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffColor, vec3 specColor) {
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    float intensity = light.color.w;

    float theta = dot(lightDir, normalize(-light.direction.xyz));
    float cutOff = light.direction.w;
    float outerCutOff = light.attenuation.z;
    float epsilon = cutOff - outerCutOff;
    float spotIntensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material_shininess);

    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (light.position.w +
                                light.attenuation.x * distance +
                                light.attenuation.y * distance * distance);

    vec3 diffuse = diff * light.color.xyz * diffColor * intensity;
    vec3 specular = spec * light.color.xyz * specColor * intensity;

    return (diffuse + specular) * attenuation * spotIntensity;
}

void main() {
    // Sample textures with fallback to material properties
    vec3 diffColor;
    float alpha;
    if (hasDiffuseMap) {
        vec4 diffTexColor = texture(texture_diffuse0, TexCoords);
        diffColor = diffTexColor.rgb;
        alpha = diffTexColor.a;
        if (alpha < 0.01) discard;
    } else {
        diffColor = material_diffuse;
        alpha = 1.0;
    }

    vec3 specColor;
    if (hasSpecularMap) {
        specColor = texture(texture_specular0, TexCoords).rgb;
    } else {
        specColor = material_specular;
    }

    // Normal mapping
    vec3 norm;
    if (hasNormalMap) {
        norm = texture(texture_normal0, TexCoords).rgb;
        norm = norm * 2.0 - 1.0;
        norm = normalize(TBN * norm);
    } else {
        norm = normalize(Normal);
    }

    vec3 viewDir = normalize(cameraPos.xyz - FragPos);

    vec3 result = vec3(0.0);

    // Directional light
    if (lightCounts.z > 0) {
        result += CalcDirLight(dirLight, norm, viewDir, diffColor, specColor);
    }

    // Point lights
    for (int i = 0; i < lightCounts.x; ++i) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, diffColor, specColor);
    }

    // Spot lights
    for (int i = 0; i < lightCounts.y; ++i) {
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir, diffColor, specColor);
    }

    // If no lights, show a basic ambient
    if (lightCounts.x == 0 && lightCounts.y == 0 && lightCounts.z == 0) {
        result = diffColor * 0.3;
    }

    FragColor = vec4(result, alpha);
}
