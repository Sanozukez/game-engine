// engine/render/texture.cpp
#include "texture.h"
#include "./../core/log.h"
#include "./../core/path_utils.h" // For Engine::loadFileFromEngineAssets

#include <stb_image.h> 


namespace Engine {
namespace Render {

Texture::Texture() : m_id(0) {
    Engine::Log::Trace("Texture: Default constructor called (empty texture).");
}

Texture::Texture(const std::string& filePath) : m_id(0), m_filePath(filePath) {
    if (!loadTexture(filePath)) {
        Engine::Log::Error(std::format("Texture: Falha ao carregar textura de '{}'.", filePath));
    }
}

// **** NOVO: Construtor para carregar textura de dados brutos na memória ****
Texture::Texture(int width, int height, int numChannels, const unsigned char* data) : m_id(0) {
    if (!createTextureFromData(width, height, numChannels, data)) {
        Engine::Log::Error("Texture: Falha ao carregar textura de dados brutos.");
    }
}

Texture::~Texture() {
    cleanup();
    Engine::Log::Trace(std::format("Texture: Destrutor chamado. Recurso OpenGL da textura '{}' liberado.", m_filePath));
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_filePath(std::move(other.m_filePath)) {
    other.m_id = 0; 
    Engine::Log::Trace("Texture: Move-constructor chamado.");
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        cleanup(); 
        m_id = other.m_id;
        m_filePath = std::move(other.m_filePath);

        other.m_id = 0; 
        Engine::Log::Trace("Texture: Move-assignment chamado.");
    }
    return *this;
}

void Texture::bind(GLuint unit) const {
    if (m_id == 0) {
        Engine::Log::Warn(std::format("Texture: Tentando vincular textura não carregada ('{}').", m_filePath));
        return;
    }
    glActiveTexture(GL_TEXTURE0 + unit); 
    glBindTexture(GL_TEXTURE_2D, m_id); 
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0); 
}

// Já existe loadTexture(filePath)
bool Texture::loadTexture(const std::string& filePath) {
    int width, height, numChannels;
    unsigned char* data = stbi_load(Engine::resolveEnginePath(filePath).string().c_str(), &width, &height, &numChannels, 0);

    if (!data) {
        Engine::Log::Error(std::format("Texture: Falha ao carregar dados da imagem '{}'. Erro: {}.", filePath, stbi_failure_reason()));
        return false;
    }

    bool success = createTextureFromData(width, height, numChannels, data);
    stbi_image_free(data); 
    return success;
}

// **** NOVO: Implementação para criar textura OpenGL de dados brutos ****
bool Texture::createTextureFromData(int width, int height, int numChannels, const unsigned char* data) {
    if (!data) {
        Engine::Log::Error("Texture: Dados de imagem nulos para criar textura.");
        return false;
    }
    
    GLenum format = GL_RGB; 
    if (numChannels == 4) { format = GL_RGBA; }
    else if (numChannels == 1) { format = GL_RED; }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D); 

    Engine::Log::Info(std::format("Texture: Textura criada de dados brutos ({}x{}, {} canais). ID: {}.", width, height, numChannels, m_id));
    return true;
}


void Texture::cleanup() {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

} // namespace Render
} // namespace Engine