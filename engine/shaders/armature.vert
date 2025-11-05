// // engine/shaders/armature.vert
#version 330 core

// 1. Entrada: Posição do bone no espaço do mundo (já transformada pelo CPU)
layout (location = 0) in vec3 aPos;

// Uniforms (Matrizes MVP)
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel; 

void main()
{
    // Transforma a posição final (World Space) para Clip Space
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}