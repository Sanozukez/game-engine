// // engine/shaders/armature.frag
#version 330 core

// Saída: Cor do fragmento
out vec4 FragColor;

// Entrada: Cor de debug definida pelo CPU
uniform vec3 uColor;

void main()
{
    FragColor = vec4(uColor, 1.0f);
}