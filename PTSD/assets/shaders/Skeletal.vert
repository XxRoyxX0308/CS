#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in ivec4 aBoneIDs;
layout(location = 6) in vec4 aBoneWeights;

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

const int MAX_BONES = 128;
uniform mat4 boneMatrices[MAX_BONES];
uniform bool hasBones;
uniform mat4 lightSpaceMatrix;

void main() {
    mat4 skinMatrix = mat4(1.0);

    if (hasBones) {
        skinMatrix = mat4(0.0);
        for (int i = 0; i < 4; ++i) {
            if (aBoneIDs[i] >= 0 && aBoneIDs[i] < MAX_BONES) {
                skinMatrix += boneMatrices[aBoneIDs[i]] * aBoneWeights[i];
            }
        }
        // Ensure valid matrix even if weights don't sum to 1
        if (length(skinMatrix[0]) < 0.001)
            skinMatrix = mat4(1.0);
    }

    vec4 worldPos = model * skinMatrix * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = projection * view * worldPos;

    mat3 normalMat3 = mat3(normalMatrix) * mat3(skinMatrix);
    Normal = normalize(normalMat3 * aNormal);

    TexCoords = aTexCoords;

    // TBN matrix for normal mapping
    vec3 T = normalize(normalMat3 * aTangent);
    vec3 B = normalize(normalMat3 * aBitangent);
    vec3 N = Normal;
    TBN = mat3(T, B, N);

    FragPosLightSpace = lightSpaceMatrix * worldPos;
}
