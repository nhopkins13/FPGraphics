/*
 *  CSCI 441, Computer Graphics, Fall 2024
 *
 *  Project: lab12
 *  File: billboardQuadShader.g.glsl
 *
 *  Description:
 *      Geometry Shader that expands a point to a billboarded quad
 *      with texture coordinates
 *
 *  Author:
 *      Dr. Jeffrey Paone, Colorado School of Mines
 *
 *  Notes:
 *
 */

// we are using OpenGL 4.1 Core profile
#version 410 core

// ***** GEOMETRY SHADER PARAMETERS *****
// TODO #A set the input primitive type
layout( points ) in;

// TODO #B set the output primitive type and size
layout( triangle_strip, max_vertices = 4 ) out;

// ***** GEOMETRY SHADER UNIFORMS *****
uniform mat4 projMatrix;

// ***** GEOMETRY SHADER INPUT *****

// ***** GEOMETRY SHADER OUTPUT *****
// TODO #I add varying output
out vec2 texCoord;

// ***** GEOMETRY SHADER HELPER FUNCTIONS *****

// ***** GEOMETRY SHADER MAIN FUNCTION *****
void main() {
    // TODO #C output point in clip space
    vec4 vector = vec4(-1,-1,0,0);
    gl_Position = projMatrix * (gl_in[0].gl_Position + vector);
    texCoord = vec2(0,0);

    // TODO #D emit
    EmitVertex();

    // TODO #F make another point
    vector = vec4(-1,1,0,0);
    gl_Position = projMatrix * (gl_in[0].gl_Position + vector);
    texCoord = vec2(1,0);
    EmitVertex();

    // TODO #G make another point
    vector = vec4(1,-1,0,0);
    gl_Position = projMatrix * (gl_in[0].gl_Position + vector);
    texCoord = vec2(0,1);
    EmitVertex();

    // TODO #H make another point
    vector = vec4(1,1,0,0);
    gl_Position = projMatrix * (gl_in[0].gl_Position + vector);
    texCoord = vec2(1,1);
    EmitVertex();

    // TODO #E and end!
    EndPrimitive();
}
