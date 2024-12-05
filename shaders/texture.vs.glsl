#version 330 core
layout(location = 0) in vec3 vPos;           // Vertex positions
layout(location = 1) in vec2 textureCoords;  // Texture coordinates

out vec2 TexCoords;                         // Pass texture coordinates to fragment shader

uniform mat4 mvpMatrix;                     // Model-View-Projection matrix

void main() {
    gl_Position = mvpMatrix * vec4(vPos, 1.0);
    TexCoords = textureCoords;
}
