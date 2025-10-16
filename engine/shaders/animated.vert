// engine/shaders/animated.vert

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// NOVOS ATRIBUTOS DE ANIMAÇÃO (A definir no Mesh::setupMesh)
layout (location = 3) in ivec4 aBoneIDs; 
layout (location = 4) in vec4 aWeights; 

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

// UNIFORMS DE ANIMAÇÃO
uniform bool uIsAnimated;
const int MAX_BONES = 100;
uniform mat4 uBoneTransforms[MAX_BONES]; // Recebe o array do Model::draw

void main()
{
    mat4 finalBoneTransform = mat4(1.0f);
    
    // Lógica de Animação
    if (uIsAnimated)
    {
        // Mistura as transformações de até 4 ossos (por vértice)
        finalBoneTransform = aWeights.x * uBoneTransforms[aBoneIDs.x];
        finalBoneTransform += aWeights.y * uBoneTransforms[aBoneIDs.y];
        finalBoneTransform += aWeights.z * uBoneTransforms[aBoneIDs.z];
        finalBoneTransform += aWeights.w * uBoneTransforms[aBoneIDs.w];
    }

    // Se não for animado, finalBoneTransform é Identity (1.0f), o que é correto.

    // Calcula a posição final do vértice
    mat4 modelMatrix = uModel * finalBoneTransform;
    
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    Normal = mat3(modelMatrix) * aNormal; // Multiplicação correta da normal
    TexCoords = aTexCoords;
    
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}