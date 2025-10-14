#pragma once

#include "./input_manager.h"
#include "../physics/raycaster.h"

#include "../ecs/world.h"
#include <GLFW/glfw3.h> // Para a janela

#include "i_keyboard_listener.h"
#include "i_mouse_listener.h"
#include "i_scroll_listener.h"
#include "i_mouse_button_listener.h"

namespace Engine
{
    namespace Input
    {
        class InputService
        {
        private:
        // Manter temporariamente, mas deve ser movido para o ClickToMoveSystem futuro.
            Engine::Physics::Raycaster m_raycaster;
            Engine::ECS::World &m_world;
            GLFWwindow *m_glfwWindow;

            // --- NOVOS MEMBROS: LISTA DE LISTENERS (ISP) ---
            std::vector<IKeyboardListener*> m_keyboardListeners;
            std::vector<IMouseListener*> m_mouseListeners;
            std::vector<IScrollListener*> m_scrollListeners;
             std::vector<IMouseButtonListener*> m_mouseButtonListeners;

            // Construtor privado para Singleton
            InputService(GLFWwindow *window, Engine::ECS::World &world)
                : m_glfwWindow(window), m_world(world) {}

            // Deleção de construtores de cópia/movimentação
            InputService(const InputService &) = delete;
            InputService &operator=(const InputService &) = delete;

            // MÉTODOS DE DISPATCH (Para serem chamados pelos Callbacks do InputManager)
            void dispatchKeyInput(int key, int action);
            void dispatchMouseMovement(double xpos, double ypos);
            void dispatchScrollInput(double yOffset);
            void dispatchMouseButtonInput(int button, int action);

        public:
            // O sistema de ECS deve chamar este método para inicializar o Singleton
            static InputService &Init(GLFWwindow *window, Engine::ECS::World &world);
            static InputService &Get();

            // Novo método que encapsula a lógica do Callback de Clique para CTM
            // void registerCallbacks(Engine::Camera::ICamera &cameraRef);

             // NOVO: Métodos para registro de Listeners (OCP)
            void registerKeyboardListener(IKeyboardListener* listener);
            void registerMouseListener(IMouseListener* listener);
            void registerScrollListener(IScrollListener* listener);
            void registerMouseButtonListener(IMouseButtonListener* listener);
            
            // NOVO: Método para configurar todos os Callbacks GLFW
            void setupAllCallbacks();

             // EXPOR MÉTODOS NECESSÁRIOS PARA O CTM (A ser usado pelo novo CTM System/Listener)
            GLFWwindow* getWindowPtr() const { return m_glfwWindow; }
            Engine::Physics::Raycaster& getRaycaster() { return m_raycaster; }
            Engine::ECS::World& getWorld() { return m_world; }

        private:
            // // Método privado para lidar com o CTM
            // void processLeftClick(Engine::Camera::ICamera &cameraRef);
        };

    } // namespace Input
} // namespace Engine