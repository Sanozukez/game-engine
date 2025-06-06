// engine/geometry/grid.h
#pragma once

#include <glad/gl.h>

namespace Engine { // NOVO: Namespace Engine
namespace Geometry { // NOVO: Namespace Geometry

class Grid {
public:
    Grid(int size = 10, float spacing = 1.0f);
    ~Grid();

    void draw() const;

private:
    GLuint vao_, vbo_;
    int vertexCount_;
};

} // namespace Geometry
} // namespace Engine