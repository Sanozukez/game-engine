// engine/geometry/grid.cpp
#include "grid.h"
#include <vector>
#include <glad/gl.h> // Incluir aqui para as chamadas GL

namespace Engine { // NOVO: Namespace Engine
namespace Geometry { // NOVO: Namespace Geometry

Grid::Grid(int size, float spacing) {
    std::vector<float> vertices;

    // Linhas paralelas ao eixo Z
    for (int i = -size; i <= size; ++i) {
        vertices.push_back(i * spacing);
        vertices.push_back(0.0f);
        vertices.push_back(-size * spacing);
        vertices.push_back(i * spacing);
        vertices.push_back(0.0f);
        vertices.push_back(size * spacing);
    }

    // Linhas paralelas ao eixo X
    for (int i = -size; i <= size; ++i) {
        vertices.push_back(-size * spacing);
        vertices.push_back(0.0f);
        vertices.push_back(i * spacing);
        vertices.push_back(size * spacing);
        vertices.push_back(0.0f);
        vertices.push_back(i * spacing);
    }

    vertexCount_ = static_cast<int>(vertices.size() / 3);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glBindVertexArray(0);
}

Grid::~Grid() {
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

void Grid::draw() const {
    glBindVertexArray(vao_);
    glDrawArrays(GL_LINES, 0, vertexCount_);
    glBindVertexArray(0);
}

} // namespace Geometry
} // namespace Engine