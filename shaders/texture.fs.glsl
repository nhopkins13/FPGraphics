#version 410 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D textureMap;

void main() {
    FragColor = texture(textureMap, TexCoords);
}
