#version 450 core

out vec4 FragColor;

in vec3 FragPos;   
in vec3 Normal;    
in vec2 TexCoords; 
in vec3 Tangent;    
in vec3 Bitangent;  

// Material PBR (uniforms)
struct Material {
    sampler2D baseColorMap;
    sampler2D normalMap;
    sampler2D roughnessMap;
    sampler2D metallicMap;
    sampler2D occlusionMap;
    sampler2D emissiveMap;

    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor; 
    vec3 emissiveFactor;
    float normalScale;
    float occlusionStrength;

    int hasBaseColorMap;
    int hasNormalMap;
    int hasRoughnessMap;
    int hasMetallicMap;
    int hasOcclusionMap;
    int hasEmissiveMap;
};
uniform Material uMaterial; 

uniform vec3 uLightPos;            
uniform vec3 uViewPos;             

void main() {
    // 1. Texturas e Fatores Base
    vec3 baseColor = uMaterial.baseColorFactor.rgb;
    if (uMaterial.hasBaseColorMap == 1) {
        baseColor *= texture(uMaterial.baseColorMap, TexCoords).rgb;
    }

    // 2. Normal Map
    vec3 normal = Normal; 
    if (uMaterial.hasNormalMap == 1) {
        vec3 normalMapTangentSpace = texture(uMaterial.normalMap, TexCoords).rgb;
        normalMapTangentSpace = normalize(normalMapTangentSpace * 2.0 - 1.0); 

        mat3 tbn = mat3(normalize(Tangent), normalize(Bitangent), normalize(Normal)); 
        normal = normalize(tbn * normalMapTangentSpace);
        normal *= uMaterial.normalScale; 
    }
    
    // 3. Roughness e Metallic (GLTF PBR Metallic/Roughness Workflow)
    float metallic = uMaterial.metallicFactor; 
    float roughness = uMaterial.roughnessFactor; 

    if (uMaterial.hasRoughnessMap == 1) { 
        vec4 metallicRoughnessMap = texture(uMaterial.roughnessMap, TexCoords);
        roughness *= metallicRoughnessMap.g; 
        metallic *= metallicRoughnessMap.b; 
    }
    if (uMaterial.hasMetallicMap == 1) { 
        metallic *= texture(uMaterial.metallicMap, TexCoords).r; 
    }
    
    float occlusion = 1.0;
    if (uMaterial.hasOcclusionMap == 1) {
        occlusion = texture(uMaterial.occlusionMap, TexCoords).r; 
        occlusion = mix(1.0, occlusion, uMaterial.occlusionStrength); 
    }

    vec3 emissive = uMaterial.emissiveFactor;
    if (uMaterial.hasEmissiveMap == 1) {
        emissive += texture(uMaterial.emissiveMap, TexCoords).rgb;
    }

    // --- PBR Lighting Model (Cook-Torrance) ---
    vec3 lightColor = vec3(1.0); 
    float lightIntensity = 30.0; 

    vec3 N = normal; 
    vec3 L = normalize(uLightPos); 
    vec3 V = normalize(uViewPos - FragPos); 
    vec3 H = normalize(L + V); 

    float NdotH = max(dot(N, H), 0.0);
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float NdotH_sq = NdotH * NdotH;
    float denomD = (NdotH_sq * (alphaSq - 1.0) + 1.0);
    float D = alphaSq / (3.14159265359 * denomD * denomD);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float k_schlick = alphaSq / 2.0; 
    float G_schlick_L = NdotL / (NdotL * (1.0 - k_schlick) + k_schlick);
    float G_schlick_V = NdotV / (NdotV * (1.0 - k_schlick) + k_schlick);
    float G = G_schlick_L * G_schlick_V;

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, baseColor, metallic); 
    vec3 F = F0 + (vec3(1.0) - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);

    vec3 specularPBR = (D * G * F) / max(4.0 * NdotL * NdotV, 0.001); 

    vec3 diffusePBR = (vec3(1.0) - F) * (1.0 - metallic) * baseColor / 3.14159265359;

    // **** MUDANÇA: Aumentar AINDA MAIS a luz ambiente para 0.5 (ou mais) ****
    vec3 ambient_light_color = vec3(0.7); // Era 0.15, aumentar para 0.5
    vec3 ambient_contribution = ambient_light_color * baseColor; 

    vec3 direct_light_contribution = lightColor * lightIntensity * (diffusePBR + specularPBR) * NdotL;

    vec3 finalColor = ambient_contribution + direct_light_contribution + emissive; 
    finalColor *= occlusion; 

    FragColor = vec4(finalColor, 1.0);
}
