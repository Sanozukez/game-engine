// Seu arquivo de implementação do serviço
#include "input_service.h"
#include "../ecs/entity.h"
#include "../core/log.h"
#include "../camera/icamera.h" //Raycaster

using namespace Engine::ECS;
using namespace Engine::ECS::Component;

// Ponteiro estático do Singleton (Global)
static Engine::Input::InputService *s_instance = nullptr;

namespace Engine
{
    namespace Input
    {
        InputService &InputService::Init(GLFWwindow *window, Engine::ECS::World &world)
        {
            if (s_instance == nullptr)
            {
                s_instance = new InputService(window, world);
            }
            return *s_instance;
        }

        InputService &InputService::Get()
        {
            if (s_instance == nullptr)
            {
                // Log de erro crítico se o serviço não foi inicializado antes de ser chamado
                // Log::Critical("InputService não inicializado! Chame Init() no App.");
            }
            return *s_instance;
        }

        void InputService::registerMouseButtonListener(IMouseButtonListener *listener)
        {
            m_mouseButtonListeners.push_back(listener);
        }

        void InputService::dispatchMouseButtonInput(int button, int action)
        {
            for (auto listener : m_mouseButtonListeners)
            {
                listener->handleMouseButtonInput(button, action);
            }
        }

        // --- IMPLEMENTAÇÕES DE REGISTRO E DISPATCH (ISP) ---

        void InputService::registerKeyboardListener(IKeyboardListener *listener)
        {
            m_keyboardListeners.push_back(listener);
        }

        void InputService::registerMouseListener(IMouseListener *listener)
        {
            m_mouseListeners.push_back(listener);
        }

        void InputService::registerScrollListener(IScrollListener *listener)
        {
            m_scrollListeners.push_back(listener);
        }

        void InputService::dispatchKeyInput(int key, int action)
        {
            for (auto listener : m_keyboardListeners)
            {
                listener->handleKeyInput(key, action);
            }
        }

        void InputService::dispatchMouseMovement(double xpos, double ypos)
        {
            for (auto listener : m_mouseListeners)
            {
                listener->handleMouseMovement(xpos, ypos);
            }
        }

        void InputService::dispatchScrollInput(double yOffset)
        {
            for (auto listener : m_scrollListeners)
            {
                listener->handleScrollInput(yOffset);
            }
        }

        // --- NOVO MÉTODO DE SETUP OCP-COMPLIANT (Substitui registerCallbacks) ---
        void InputService::setupAllCallbacks()
        {
            auto &inputManager = InputManager::Get();
            GLFWwindow *window = m_glfwWindow;

            // 1. MOUSE BUTTONS (LMB/RMB Press/Release)
            inputManager.RegisterCallback(InputEvent::MouseButtonPressed, 0,
                                          [window, this](const InputEventData &data)
                                          {
                                              if (data.key == GLFW_MOUSE_BUTTON_RIGHT)
                                              {
                                                  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                                                  Engine::Core::Log::Debug("[SERVICE] RMB PRESS: cursor disabled");
                                              }

                                              // 1) Primeiro, listeners recebem o PRESS (CIS vai dar resetMouseState)
                                              this->dispatchMouseButtonInput(data.key, GLFW_PRESS);
                                              Engine::Core::Log::Debug("[SERVICE] RMB PRESS: dispatched to listeners");

                                              if (data.key == GLFW_MOUSE_BUTTON_RIGHT)
                                              {
                                                  // 2) PRIME garantido (MouseMoved sintético imediatamente após reset)
                                                  double mx, my;
                                                  glfwGetCursorPos(window, &mx, &my);
                                                  Engine::Core::Log::Debug(std::format("[SERVICE] RMB PRESS: PRIME MouseMoved mx={}, my={}", mx, my));
                                                  this->dispatchMouseMovement(mx, my);
                                              }
                                          });

            inputManager.RegisterCallback(InputEvent::MouseButtonReleased, 0,
                                          [window, this](const InputEventData &data)
                                          {
                                              if (data.key == GLFW_MOUSE_BUTTON_RIGHT)
                                              {
                                                  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                                                  Engine::Core::Log::Debug("[SERVICE] RMB RELEASE: cursor normal");
                                              }
                                              this->dispatchMouseButtonInput(data.key, GLFW_RELEASE);
                                          });

            // 2. MOUSE MOVED (CRÍTICO: Habilita a rotação)
            inputManager.RegisterCallback(InputEvent::MouseMoved, 0,
                                          [this](const InputEventData &data)
                                          {
                                              // Envia posição bruta para o Listener.
                                              this->dispatchMouseMovement(data.xpos, data.ypos);
                                          });

            // 3. MOUSE SCROLLED
            inputManager.RegisterCallback(InputEvent::MouseScrolled, 0,
                                          [this](const InputEventData &data)
                                          {
                                              this->dispatchScrollInput(data.yoffset);
                                          });

            // 4. TECLAS DE JOGO
            inputManager.RegisterCallback(InputEvent::KeyPressed, 0,
                                          [this](const InputEventData &data)
                                          {
                                              if (data.key >= GLFW_KEY_SPACE && data.key <= GLFW_KEY_LAST)
                                              {
                                                  this->dispatchKeyInput(data.key, GLFW_PRESS);
                                              }
                                          });

            inputManager.RegisterCallback(InputEvent::KeyReleased, 0,
                                          [this](const InputEventData &data)
                                          {
                                              if (data.key >= GLFW_KEY_SPACE && data.key <= GLFW_KEY_LAST)
                                              {
                                                  this->dispatchKeyInput(data.key, GLFW_RELEASE);
                                              }
                                          });

            // 5. TECLA ESC
            inputManager.RegisterCallback(InputEvent::KeyPressed, GLFW_KEY_ESCAPE,
                                          [window](const InputEventData &data)
                                          {
                                              glfwSetWindowShouldClose(window, true);
                                          });
        }

    } // namespace Input
} // namespace Engine