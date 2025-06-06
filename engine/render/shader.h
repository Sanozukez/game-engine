// engine/render/shader.h
#pragma once

#include <string>
#include <glad/gl.h>
#include <glm/glm.hpp>

namespace Engine { 
namespace Render { 

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void use() const;
    GLuint getID() const;

    void setMat4(const std::string& name, const glm::mat4& value) const; 
    void setInt(const std::string& name, int value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value) const; 
    void setVec4(const std::string& name, const glm::vec4& value) const;

private:
    GLuint ID;
    std::string loadShaderSource(const std::string& path);
    GLuint compileShader(GLenum type, const std::string& source);
    GLint getUniformLocation(const std::string& name) const;
};

} // namespace Render
} // namespace Engine