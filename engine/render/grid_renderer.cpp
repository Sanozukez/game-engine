#include "grid_renderer.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

static unsigned int gridVAO = 0, gridVBO = 0;
static int linesCount = 0;

void GridRenderer::initialize() {
    std::vector<float> lines;

    const int gridSize = 10;
    const float spacing = 1.0f;

    for (int i = -gridSize; i <= gridSize; ++i) {
        lines.push_back(i * spacing);
        lines.push_back(0);
        lines.push_back(-gridSize * spacing);
        lines.push_back(i * spacing);
        lines.push_back(0);
        lines.push_back(gridSize * spacing);

        lines.push_back(-gridSize * spacing);
        lines.push_back(0);
        lines.push_back(i * spacing);
        lines.push_back(gridSize * spacing);
        lines.push_back(0);
        lines.push_back(i * spacing);
    }

    linesCount = lines.size() / 3;

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void GridRenderer::render() {
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, linesCount);
}

void GridRenderer::cleanup() {
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
}
