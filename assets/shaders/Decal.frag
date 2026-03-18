#version 410 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D decalTexture;
uniform float alpha;

void main() {
    vec4 texColor = texture(decalTexture, TexCoord);

    // Discard fully transparent pixels
    if (texColor.a < 0.01)
        discard;

    // Apply fade-out alpha
    FragColor = vec4(texColor.rgb, texColor.a * alpha);
}
