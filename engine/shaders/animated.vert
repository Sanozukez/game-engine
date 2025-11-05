// src/engine/shaders/animated.vert
// (arquivo inteiro, atualizado com uNode)
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

// Animação
layout (location = 4) in ivec4 aBoneIDs;
layout (location = 5) in vec4  aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

// NEW: transform do nó da malha (onde o primitive está no glTF)
uniform mat4 uNode;

uniform bool uIsAnimated;
const int MAX_BONES = 100;
uniform mat4 uBoneTransforms[MAX_BONES];

void main()
{
    mat4 skin = mat4(1.0);
    if (uIsAnimated) {
        skin  = aWeights.x * uBoneTransforms[aBoneIDs.x];
        skin += aWeights.y * uBoneTransforms[aBoneIDs.y];
        skin += aWeights.z * uBoneTransforms[aBoneIDs.z];
        skin += aWeights.w * uBoneTransforms[aBoneIDs.w];
    }

    // A matriz da malha é uModel * uNode (instância * nó da cena)
    mat4 M = uModel * uNode * skin;

    vec4 worldPos = M * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // normal: usar inverse-transpose de M sem translação
    mat3 N = mat3(transpose(inverse(M)));
    Normal = normalize(N * aNormal);

    TexCoords = aTexCoords;

    gl_Position = uProjection * uView * worldPos;
}
