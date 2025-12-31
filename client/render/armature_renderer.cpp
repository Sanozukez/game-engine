// // engine/render/armature_renderer.cpp
#include "armature_renderer.h"
#include "opengl_types.h"
#include "../../engine/core/log.h"
#include <format>

namespace Engine {
namespace Render {

ArmatureRenderer::ArmatureRenderer() {
    // A inicialização é feita sob demanda na primeira chamada a draw().
}

ArmatureRenderer::~ArmatureRenderer() {
    if (m_armatureVAO != 0) {
        glDeleteVertexArrays(1, &m_armatureVAO);
    }
    if (m_armatureVBO != 0) {
        glDeleteBuffers(1, &m_armatureVBO);
    }
}

void ArmatureRenderer::setupBuffers() const {
    glGenVertexArrays(1, &m_armatureVAO);
    glGenBuffers(1, &m_armatureVBO);
    
    glBindVertexArray(m_armatureVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_armatureVBO);
    
    // Apenas a posição (vec3) é o nosso atributo (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    Engine::Core::Log::Info("ArmatureRenderer: Buffers (VAO/VBO) para visualização de esqueleto criados.");
}

void ArmatureRenderer::updateBuffers(const std::vector<glm::vec3>& debugLines) const {
    if (m_armatureVAO == 0) {
        setupBuffers();
    }
    
    if (debugLines.empty()) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_armatureVBO);
    // Usa GL_DYNAMIC_DRAW, pois a pose do esqueleto muda a cada frame.
    glBufferData(GL_ARRAY_BUFFER, debugLines.size() * sizeof(glm::vec3), debugLines.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void ArmatureRenderer::draw(const Shader& shader, 
                          const std::vector<glm::vec3>& debugLines,
                          const glm::mat4& viewMatrix,
                          const glm::mat4& projectionMatrix,
                          const glm::mat4& modelMatrix) const // <-- NOVO ARGUMENTO
{
    if (debugLines.empty()) {
        // --- NOVO LOG DE DEBUG ---
        // Se vermos este log, significa que o RenderSystem chamou o draw,
        // mas o Model::getSkeletonDebugLines falhou em gerar as linhas.
        Engine::Core::Log::Warn("[DEBUG_RENDER] ArmatureRenderer::draw foi chamado, mas debugLines estava vazio. Nada para desenhar.");
        // --- FIM DO LOG ---
        return;
    }
    // --- NOVO LOG DE DEBUG ---
    // Se vermos este log, TUDO está funcionando e o bug é
    // visual (ex: matriz modelMatrix errada).
    // Engine::Core::Log::Info(std::format("[DEBUG_RENDER] ArmatureRenderer::draw: Desenhando {} linhas.", debugLines.size() / 2));
    // --- FIM DO LOG ---

    // 1. Atualiza e Binda os buffers
    updateBuffers(debugLines);
    
    shader.use();

    // 2. Envia as matrizes
    // A matriz Model é Identity, pois os vértices de debugLines já estão no espaço do mundo.
    shader.setMat4("uProjection", projectionMatrix);
    shader.setMat4("uView", viewMatrix);
    shader.setMat4("uModel", modelMatrix);
    
    // 3. Configurações de desenho
    glDisable(GL_DEPTH_TEST); // Para ver os bones através da malha
    glLineWidth(3.0f); 
    
    // Cor de debug (verde)
    shader.setVec3("uColor", glm::vec3(0.0f, 1.0f, 0.0f)); 

    // 4. Desenha
    glBindVertexArray(m_armatureVAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(debugLines.size())); 
    glBindVertexArray(0);

    // 5. Restaura o estado GL
    glEnable(GL_DEPTH_TEST);
    shader.unuse();
}

} // namespace Render
} // namespace Engine