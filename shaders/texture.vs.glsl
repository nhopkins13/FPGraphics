#version 330 core

layout(location = 0) in vec3 vPos;           // Vertex positions
layout(location = 1) in vec2 textureCoords;  // Texture coordinates

out vec3 FragPos;                            // World-space position
out vec2 TexCoords;                          // Pass texture coordinates to fragment shader

uniform mat4 mvpMatrix;                      // Model-View-Projection matrix
uniform mat4 modelMatrix;                    // Model matrix for world-space position

void main() {
    gl_Position = mvpMatrix * vec4(vPos, 1.0);
    TexCoords = textureCoords;
    FragPos = vec3(modelMatrix * vec4(vPos, 1.0)); // Transform position to world space
}
