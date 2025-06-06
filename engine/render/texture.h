// engine/render/texture.h
#pragma once

#include <glad/gl.h> // For GLuint
#include <string>    // For the file path
#include <memory>    // For unique_ptr
#include <vector>    // For raw pixel data if needed (optional for texture class)

namespace Engine {
namespace Render {

class Texture {
public:
    Texture(); // Default constructor (creates empty texture)
    Texture(const std::string& filePath); // Constructor for loading from file path
    // **** NOVO: Construtor para carregar textura de dados brutos na memória ****
    Texture(int width, int height, int numChannels, const unsigned char* data); 

    ~Texture();

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(GLuint unit = 0) const; 
    void unbind() const;

    bool isLoaded() const { return m_id != 0; }
    GLuint getID() const { return m_id; }

private:
    GLuint m_id; // OpenGL texture ID
    std::string m_filePath; // Optional, only for file-loaded textures

    void cleanup();
    bool loadTexture(const std::string& filePath); // Loads from file path
    // **** NOVO: Helper para criar textura OpenGL de dados brutos ****
    bool createTextureFromData(int width, int height, int numChannels, const unsigned char* data);
};

} // namespace Render
} // namespace Engine