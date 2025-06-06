// engine/render/shader.cpp
#include "./../render/shader.h" 
#include <fstream>
#include <sstream>
#include "./../core/path_utils.h" 
#include "./../core/log.h" 
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp> 

namespace Engine { 
namespace Render { 

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath)
{
    std::string vertexCode = loadShaderSource(vertexPath);
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode);
    std::string fragmentCode = loadShaderSource(fragmentPath); 
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode); 

    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);

    GLint numUniforms = 0;
    glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &numUniforms);
    Engine::Log::Debug(std::format("Active uniforms in shader program (ID: {}):\n", ID));
    for (GLint i = 0; i < numUniforms; ++i)
    {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveUniform(ID, i, sizeof(name), &length, &size, &type, name);
        Engine::Log::Debug(std::format(" - {}", name));
    }

    int success;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(ID, 512, nullptr, infoLog);
        Engine::Log::Error(std::format("Error linking shader program:\n{}", infoLog));
        throw std::runtime_error("Failed to link shader program.");
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(ID);
}

void Shader::use() const
{
    glUseProgram(ID);
}

GLuint Shader::getID() const
{
    return ID;
}

std::string Shader::loadShaderSource(const std::string &path)
{
    return Engine::loadFileFromEngineAssets(path);
}

GLuint Shader::compileShader(GLenum type, const std::string &source)
{
    GLuint shader = glCreateShader(type);
    const char *src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        Engine::Log::Error(std::format("Error compiling shader:\n{}", infoLog));
        throw std::runtime_error("Failed to compile shader.");
    }

    return shader;
}

GLint Shader::getUniformLocation(const std::string& name) const {
    GLint location = glGetUniformLocation(ID, name.c_str());
    if (location == -1) {
        Engine::Log::Warn(std::format("[Shader] Uniform '{}' not found (ID: {}).", name, ID));
    }
    return location;
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const
{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3f(getUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}
void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4f(getUniformLocation(name), value.x, value.y, value.z, value.w);
}

} // namespace Render
} // namespace Engine