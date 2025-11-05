// // engine/render/armature_renderer.h
#pragma once

#include "shader.h"
#include <glm/glm.hpp>
#include <vector>

namespace Engine {
namespace Render {

class ArmatureRenderer {
public:
    ArmatureRenderer();
    ~ArmatureRenderer();

    // Responsável por atualizar o VBO e desenhar as linhas na tela.
    void draw(const Shader& shader, 
              const std::vector<glm::vec3>& debugLines,
              const glm::mat4& viewMatrix,
              const glm::mat4& projectionMatrix,
              const glm::mat4& modelMatrix) const;

private:
    // IDs de buffer (VAO/VBO) para renderização de linhas
    // Usaremos 'mutable' para permitir que setupBuffers e updateBuffers modifiquem
    // esses membros em um método 'const draw', que é uma convenção comum em renderização.
    mutable unsigned int m_armatureVAO = 0;
    mutable unsigned int m_armatureVBO = 0;
    
    // Função para configurar o VAO/VBO (chamada apenas na primeira vez)
    void setupBuffers() const; 

    // Função para atualizar o VBO com novos dados (chamada a cada frame)
    void updateBuffers(const std::vector<glm::vec3>& debugLines) const;
};

} // namespace Render
} // namespace Engine