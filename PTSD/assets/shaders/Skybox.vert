#version 410 core

layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

layout(std140) uniform Matrices3D {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
    vec4 cameraPos;
};

void main() {
    TexCoords = aPos;
    // Remove translation from the view matrix so the skybox follows the camera
    mat4 viewNoTranslation = mat4(mat3(view));
    vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);
    // Set z = w so after perspective divide depth = 1.0 (the farthest value)
    gl_Position = pos.xyww;
}
