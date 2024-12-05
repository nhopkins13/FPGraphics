#version 410 core

// Inputs from Vertex Shader
in vec3 vertexColor;

// Output
out vec4 fragColorOut;

void main() {
    fragColorOut = vec4(vertexColor, 1.0);
}
