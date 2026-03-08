#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 TexCoords;
layout(location = 3) out vec4 FragPosLightSpace;
layout(location = 4) out mat3 TBN;

layout(std140) uniform Matrices3D {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
    vec4 cameraPos;
};

uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(normalMatrix) * aNormal;
    TexCoords = aTexCoords;
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    // TBN matrix for normal mapping
    vec3 T = normalize(mat3(normalMatrix) * aTangent);
    vec3 B = normalize(mat3(normalMatrix) * aBitangent);
    vec3 N = normalize(Normal);
    TBN = mat3(T, B, N);

    gl_Position = projection * view * worldPos;
}
