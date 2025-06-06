#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent; // NOVO: Atributo para tangente

out vec3 FragPos;      
out vec3 Normal;       
out vec2 TexCoords;    
out vec3 Tangent;      // NOVO: Passar tangente para o fragment shader
out vec3 Bitangent;    // NOVO: Passar bitangente para o fragment shader

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));    
    Normal = mat3(transpose(inverse(uModel))) * aNormal; // Normal transformada para espaço do mundo
    TexCoords = aTexCoords; 

    // NOVO: Calcular Bitangente e transformar TBN para o fragment shader
    Tangent = mat3(transpose(inverse(uModel))) * aTangent; // Tangente transformada para espaço do mundo
    Bitangent = normalize(cross(Normal, Tangent)); // Calcula Bitangente no espaço do mundo
    Tangent = normalize(Tangent); // Normaliza tangente para evitar problemas de escala

    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}