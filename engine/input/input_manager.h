// engine/input/input_manager.h
#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <map>
#include <vector>

namespace Engine
{
    namespace Input
    { // **** MUDANÇA AQUI: INCLUIR NAMESPACE INPUT ****

        // Enumeração para representar diferentes eventos de input
        enum class InputEvent
        {
            KeyPressed,
            KeyReleased,
            MouseButtonPressed,
            MouseButtonReleased,
            MouseMoved,
            MouseScrolled
        };

        // Estrutura para armazenar informações sobre um evento de input
        struct InputEventData
        {
            int key;                 // GLFW Key code ou botão do mouse
            double xpos, ypos;       // Posição do mouse
            double xoffset, yoffset; // Offset do scroll
            bool rightMousePressed;  // Novo: estado do botão direito do mouse
        };

        // Definição de tipo para funções callback de input
        using InputCallback = std::function<void(const InputEventData &)>;

        class InputManager
        {
        public:
            // Singleton - Garante que apenas uma instância exista
            static InputManager &Get();

            // Registra uma função callback para um determinado evento de input
            void RegisterCallback(InputEvent event, int key, const InputCallback &callback);

            // Remove uma função callback registrada
            void UnregisterCallback(InputEvent event, int key, const InputCallback &callback);

            // Processa eventos de input do GLFW (configura os callbacks de baixo nível do GLFW)
            void ProcessInput(GLFWwindow *window);

            // Método para consultar o estado atual de uma tecla
            bool IsKeyPressed(int key) const;

            // Novos: Métodos para consultar o estado do mouse
            bool IsRightMouseButtonPressed() const { return m_rightMousePressed; }
            double GetMouseX() const { return m_lastMouseX; }
            double GetMouseY() const { return m_lastMouseY; }

            // Método para obter o offset do mouse desde o último frame,
            // gerenciando a lógica de "first mouse".
            // Esta função não é mais usada diretamente pela OrbitCamera,
            // mas pode ser útil para outras partes do código.
            void GetMouseDelta(double &xoffset, double &yoffset);

        private:
            InputManager() = default;
            ~InputManager() = default;
            InputManager(const InputManager &) = delete;
            InputManager &operator=(const InputManager &) = delete;

            // Mapa para armazenar as funções callback registradas
            std::map<InputEvent, std::map<int, std::vector<InputCallback>>> callbacks_;

            // Array para armazenar o estado atual de cada tecla
            bool m_keyStates[1024] = {false};

            // Membros para armazenar o estado do mouse
            bool m_rightMousePressed = false;
            double m_lastMouseX = 0.0;
            double m_lastMouseY = 0.0;
            bool m_firstMouse = true; // Flag para o primeiro movimento do mouse

            // Funções auxiliares para processar eventos específicos (chamadas pelos callbacks do GLFW)
            void processKeyEvent(int key, int action);
            void processMouseButtonEvent(int button, int action);
            void processMousePositionEvent(double xpos, double ypos);
            void processMouseScrollEvent(double xoffset, double yoffset);
        };

    } // namespace Input
} // namespace Engine