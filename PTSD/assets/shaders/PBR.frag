#version 410 core

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;
layout(location = 3) in vec4 FragPosLightSpace;
layout(location = 4) in mat3 TBN;

out vec4 FragColor;

// ====== PBR textures ======
uniform sampler2D texture_diffuse0;  // albedo
uniform sampler2D texture_normal0;
uniform sampler2D texture_metallic0;
uniform sampler2D texture_roughness0;
uniform sampler2D texture_ao0;
uniform sampler2D shadowMap;

// ====== PBR material fallback ======
uniform vec3 pbr_albedo;
uniform float pbr_metallic;
uniform float pbr_roughness;
uniform float pbr_ao;
uniform bool hasAlbedoMap;
uniform bool hasNormalMap;
uniform bool hasMetallicMap;
uniform bool hasRoughnessMap;
uniform bool hasAOMap;

// ====== Shadow ======
uniform bool shadowEnabled;
uniform mat4 lightSpaceMatrix;

// ====== Lights ======
struct DirLight {
    vec4 direction;
    vec4 color;
};

struct PointLightData {
    vec4 position;
    vec4 color;
    vec4 attenuation;
};

struct SpotLightData {
    vec4 position;
    vec4 direction;
    vec4 color;
    vec4 attenuation;
};

layout(std140) uniform Lights {
    DirLight dirLight;
    ivec4 lightCounts;
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

const float PI = 3.14159265359;

// ====== PBR Functions ======

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

// Geometry function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Geometry function (Smith's method)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel equation (Schlick approximation)
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// ====== Shadow ======
float ShadowCalculation(vec4 fragPosLS, vec3 normal, vec3 lightDir) {
    if (!shadowEnabled) return 0.0;

    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

// ====== Cook-Torrance BRDF for a single light ======
vec3 CalcPBRLight(vec3 lightDir, vec3 lightColor, float lightIntensity,
                  vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 L = normalize(lightDir);
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // Cook-Torrance specular BRDF
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic; // Metallic surfaces have no diffuse

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * lightColor * lightIntensity * NdotL;
}

void main() {
    // Sample textures with fallbacks
    vec3 albedo = hasAlbedoMap ? pow(texture(texture_diffuse0, TexCoords).rgb, vec3(2.2)) : pbr_albedo;
    float alpha = hasAlbedoMap ? texture(texture_diffuse0, TexCoords).a : 1.0;
    if (alpha < 0.01) discard;

    float metallic = hasMetallicMap ? texture(texture_metallic0, TexCoords).r : pbr_metallic;
    float roughness = hasRoughnessMap ? texture(texture_roughness0, TexCoords).r : pbr_roughness;
    float ao = hasAOMap ? texture(texture_ao0, TexCoords).r : pbr_ao;

    // Normal
    vec3 N;
    if (hasNormalMap) {
        N = texture(texture_normal0, TexCoords).rgb;
        N = N * 2.0 - 1.0;
        N = normalize(TBN * N);
    } else {
        N = normalize(Normal);
    }

    vec3 V = normalize(cameraPos.xyz - FragPos);

    // Base reflectivity (F0)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    // Directional light
    if (lightCounts.z > 0) {
        vec3 lightDir = normalize(-dirLight.direction.xyz);
        float shadow = ShadowCalculation(FragPosLightSpace, N, lightDir);
        Lo += (1.0 - shadow) * CalcPBRLight(lightDir, dirLight.color.xyz,
                                              dirLight.direction.w, N, V, albedo, metallic, roughness, F0);
    }

    // Point lights
    for (int i = 0; i < lightCounts.x; ++i) {
        vec3 lightDir = pointLights[i].position.xyz - FragPos;
        float distance = length(lightDir);
        float attenuation = 1.0 / (pointLights[i].position.w +
                                    pointLights[i].attenuation.x * distance +
                                    pointLights[i].attenuation.y * distance * distance);
        Lo += CalcPBRLight(lightDir, pointLights[i].color.xyz,
                           pointLights[i].color.w, N, V, albedo, metallic, roughness, F0) * attenuation;
    }

    // Spot lights
    for (int i = 0; i < lightCounts.y; ++i) {
        vec3 lightDir = spotLights[i].position.xyz - FragPos;
        float distance = length(lightDir);
        vec3 L = normalize(lightDir);

        float theta = dot(L, normalize(-spotLights[i].direction.xyz));
        float cutOff = spotLights[i].direction.w;
        float outerCutOff = spotLights[i].attenuation.z;
        float epsilon = cutOff - outerCutOff;
        float spotIntensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

        float attenuation = 1.0 / (spotLights[i].position.w +
                                    spotLights[i].attenuation.x * distance +
                                    spotLights[i].attenuation.y * distance * distance);

        Lo += CalcPBRLight(lightDir, spotLights[i].color.xyz,
                           spotLights[i].color.w, N, V, albedo, metallic, roughness, F0) * attenuation * spotIntensity;
    }

    // Ambient (simple)
    vec3 ambient = vec3(0.03) * albedo * ao;

    // If no lights, show ambient only
    if (lightCounts.x == 0 && lightCounts.y == 0 && lightCounts.z == 0) {
        ambient = albedo * 0.3;
    }

    vec3 color = ambient + Lo;

    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, alpha);
}
