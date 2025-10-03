// // engine/render/texture.cpp

#include "texture.h"
#include "./../core/log.h"
#include "./../core/path_utils.h" // Para Engine::resolveEnginePath

#include <stb_image.h>
#include <format>
#include <stdexcept>
#include <filesystem>

namespace Engine
{
    namespace Render
    {

        // =========================================================================
        // CUSTOM DELETER (Lógica de Liberação de Recursos OpenGL)
        // =========================================================================

        // Função estática que o shared_ptr chama quando a contagem de referência é ZERO.
        void Texture::OpenGLResourceDeleter(GLuint *id_ptr)
        {
            if (id_ptr && *id_ptr != 0)
            {
                // ESSENCIAL: Garante que a liberação seja feita APENAS UMA VEZ
                glDeleteTextures(1, id_ptr);
                Engine::Log::Trace(std::format("Texture: Recurso OpenGL (ID: {}) liberado via Custom Deleter.", *id_ptr));
            }
            delete id_ptr; // Libera a memória do GLuint*
        }

        // =========================================================================
        // CONSTRUTORES E DESTRUTOR
        // =========================================================================

        // Função auxiliar para inicializar o shared_ptr vazio
        std::shared_ptr<GLuint> createEmptyHandle()
        {
            // Cria um novo GLuint na heap, inicializado como 0, e anexa o Custom Deleter.
            return std::shared_ptr<GLuint>(new GLuint(0), Texture::OpenGLResourceDeleter);
        }

        Texture::Texture()
            : m_id_handle(createEmptyHandle()), m_filePath("")
        {
            Engine::Log::Trace("Texture: Default constructor called (empty handle).");
        }

        Texture::Texture(const std::string &filePath)
            : m_id_handle(createEmptyHandle()), m_filePath(filePath)
        {
            if (!loadTexture(filePath))
            {
                Engine::Log::Error(std::format("Texture: Falha ao carregar textura de '{}'.", filePath));
            }
        }

        // Construtor para carregar textura de dados brutos na memória
        Texture::Texture(int width, int height, int numChannels, const unsigned char *data)
            : m_id_handle(createEmptyHandle()), m_filePath("")
        {
            if (!createTextureFromData(width, height, numChannels, data))
            {
                Engine::Log::Error("Texture: Falha ao carregar textura de dados brutos.");
            }
        }

        // Destrutor é padrão; o shared_ptr faz o cleanup no final da contagem.
        // Texture::~Texture() = default;

        // =========================================================================
        // OPERAÇÕES (BIND, UNBIND, CLEANUP)
        // =========================================================================

        void Texture::bind(GLuint unit) const
        {
            // Usa a interface adaptada para obter o ID (0 se não estiver carregado)
            GLuint id = getID();

            if (id == 0)
            {
                Engine::Log::Warn(std::format("Texture: Tentando vincular textura não carregada ('{}').", m_filePath));
                return;
            }
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, id);
        }

        void Texture::unbind() const
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void Texture::cleanup()
        {
            // A função cleanup agora é essencialmente vazia, pois o shared_ptr gerencia a deleção.
            // Ela pode ser usada para resetar o ponteiro em situações de erro ou move-assignment.
            // Se o ponteiro for resetado, o shared_ptr tentará liberar o recurso.
            // m_id_handle.reset();
        }

        // =========================================================================
        // FUNÇÕES DE CARREGAMENTO
        // =========================================================================

        bool Texture::loadTexture(const std::string &filePath)
        {
            // ... (Lógica de stb_image_load e path_utils)
            int width, height, numChannels;
            unsigned char *data = stbi_load(Engine::resolveEnginePath(filePath).string().c_str(), &width, &height, &numChannels, 0);

            if (!data)
            {
                Engine::Log::Error(std::format("Texture: Falha ao carregar dados da imagem '{}'. Erro: {}.", filePath, stbi_failure_reason()));
                return false;
            }

            bool success = createTextureFromData(width, height, numChannels, data);
            stbi_image_free(data);
            return success;
        }

        bool Texture::createTextureFromData(int width, int height, int numChannels, const unsigned char *data)
        {
            if (!data)
            {
                Engine::Log::Error("Texture: Dados de imagem nulos para criar textura.");
                return false;
            }

            GLenum format = GL_RGB;
            if (numChannels == 4)
                format = GL_RGBA;
            else if (numChannels == 1)
                format = GL_RED;

            // 1. Geração do ID OpenGL (Crucial)
            GLuint new_id;
            glGenTextures(1, &new_id);

            // 2. Criação do Handle Compartilhado
            // Substitui o handle vazio (0) pelo novo handle, com o Custom Deleter
            m_id_handle = std::shared_ptr<GLuint>(new GLuint(new_id), OpenGLResourceDeleter);

            // 3. Configuração da Textura
            glBindTexture(GL_TEXTURE_2D, *m_id_handle); // Usa *m_id_handle

            // ... (Configurações glTexParameteri existentes)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            Engine::Log::Info(std::format("Texture: Textura criada de dados brutos ({}x{}, {} canais). ID: {}.",
                                          width, height, numChannels, *m_id_handle));
            return true;
        }

        // =========================================================================
        // OPERAÇÕES DE MOVIMENTO (Agora usando shared_ptr)
        // =========================================================================
        // Os operadores de move-assignment/constructor são default, e usam move semantics.
        // O C++ move-assignment default é seguro, pois ele move o shared_ptr.
        // Se você quiser manter as implementações verbosas, use a seguinte lógica (sem cleanup explícito):

        /*
        Texture::Texture(Texture &&other) noexcept
            : m_id_handle(std::move(other.m_id_handle)),
              m_filePath(std::move(other.m_filePath))
        {
            Engine::Log::Trace("Texture: Move-constructor chamado.");
        }

        Texture &Texture::operator=(Texture &&other) noexcept
        {
            if (this != &other) {
                m_id_handle = std::move(other.m_id_handle);
                m_filePath = std::move(other.m_filePath);
            }
            return *this;
        }
        */

    } // namespace Render
} // namespace Engine