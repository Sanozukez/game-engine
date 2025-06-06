// engine/geometry/sphere.h
#pragma once

#include <glad/gl.h> // Incluir aqui para GLuint

namespace Engine { // NOVO: Namespace Engine
namespace Geometry { // NOVO: Namespace Geometry

class Sphere {
public:
    Sphere(float radius = 0.25f, int stacks = 20, int slices = 20);
    ~Sphere();

    void draw() const;

private:
    unsigned int vao_, vbo_, ebo_;
    int indexCount_;
};

} // namespace Geometry
} // namespace Engine