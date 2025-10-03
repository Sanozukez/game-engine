// // engine/render/texture.h

#pragma once

#include <glad/gl.h>
#include <string>
#include <memory> // Necessário para unique_ptr e shared_ptr
#include <vector> // Para raw pixel data

namespace Engine
{
    namespace Render
    {

        class Texture
        {
        public:
            Texture();                            // Default constructor (cria handle vazio)
            Texture(const std::string &filePath); // Construtor para carregar de arquivo
            // Construtor para carregar textura de dados brutos na memória
            Texture(int width, int height, int numChannels, const unsigned char *data);

            // O destrutor padrão é seguro, pois o shared_ptr chamará o Custom Deleter
            ~Texture() = default;

            // PROPRIEDADE DE CÓPIA: Os construtores de cópia são default,
            // permitindo que o shared_ptr seja copiado e que a contagem de referência aumente.
            Texture(const Texture &other) = default;
            Texture &operator=(const Texture &other) = default;

            // PROPRIEDADE DE MOVIMENTO: Removemos a implementação antiga para usar a padrão
            // Move operations são agora default (ou podemos deixar move como default também)
            Texture(Texture &&other) noexcept = default;
            Texture &operator=(Texture &&other) noexcept = default;

            // Permite a instanciação do Material/Mesh: retorna uma nova cópia que compartilha o ID.
            std::unique_ptr<Texture> clone() const
            {
                return std::make_unique<Texture>(*this); // Usa o construtor de cópia padrão
            }            

            void bind(GLuint unit = 0) const;
            void unbind() const;

            // Interface adaptada para usar o shared_ptr
            bool isLoaded() const { return m_id_handle && (*m_id_handle != 0); }
            GLuint getID() const { return m_id_handle ? *m_id_handle : 0; }

            // NOVO: Custom Deleter (Função que o shared_ptr chama quando a contagem chega a zero)
            static void OpenGLResourceDeleter(GLuint* id_ptr);

        private:
            // **SUBSTITUIÇÃO CRÍTICA**: O handle do OpenGL agora é compartilhado
            // (Padrão de Otimização)
            std::shared_ptr<GLuint> m_id_handle; // OpenGL texture ID Handle (ponteiro compartilhado)
            std::string m_filePath;

            

            // Funções utilitárias (devem ser atualizadas para usar o m_id_handle)
            // cleanup() agora deve ser usada com cautela, pois o deleter faz o trabalho.
            bool loadTexture(const std::string &filePath);
            bool createTextureFromData(int width, int height, int numChannels, const unsigned char *data);

            // NOTA: A função cleanup() pode ser removida ou deixada como um helper interno,
            // mas não deve mais conter glDeleteTextures. Deixaremos a declaração por enquanto.
            void cleanup();
        };

    } // namespace Render
} // namespace Engine