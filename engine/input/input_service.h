#pragma once

#include "./input_manager.h"
#include "../physics/raycaster.h"
#include "../camera/icamera.h"
#include "../ecs/world.h"
#include <GLFW/glfw3.h> // Para a janela

namespace Engine
{
    namespace Input
    {

        class InputService
        {
        private:
            Engine::Physics::Raycaster m_raycaster;
            Engine::ECS::World &m_world;
            GLFWwindow *m_glfwWindow;

            // Construtor privado para Singleton
            InputService(GLFWwindow *window, Engine::ECS::World &world)
                : m_glfwWindow(window), m_world(world) {}

            // Deleção de construtores de cópia/movimentação
            InputService(const InputService &) = delete;
            InputService &operator=(const InputService &) = delete;

        public:
            // O sistema de ECS deve chamar este método para inicializar o Singleton
            static InputService &Init(GLFWwindow *window, Engine::ECS::World &world);
            static InputService &Get();

            // Novo método que encapsula a lógica do Callback de Clique para CTM
            void registerCallbacks(Engine::Camera::ICamera &cameraRef);

        private:
            // Método privado para lidar com o CTM
            void processLeftClick(Engine::Camera::ICamera &cameraRef);
        };

    } // namespace Input
} // namespace Engine