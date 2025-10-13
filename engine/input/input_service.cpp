// Seu arquivo de implementação do serviço
#include "input_service.h"
#include "../ecs/components/movement_component.h"
#include "../ecs/components/player_component.h"
#include "../ecs/entity.h" // <--- NECESSÁRIO para EntityID e INVALID_ENTITY_ID
#include "../core/log.h" // Adicionado para uso futuro

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

        void InputService::processLeftClick(Engine::Camera::ICamera &cameraRef)
        {
            const EntityID playerID = m_world.getSingleEntityWith<Component::Player>();

            if (playerID != INVALID_ENTITY_ID)
            {
                double mouseX, mouseY;
                glfwGetCursorPos(m_glfwWindow, &mouseX, &mouseY);
                int screenWidth, screenHeight;
                glfwGetWindowSize(m_glfwWindow, &screenWidth, &screenHeight);

                m_raycaster.update(mouseX, mouseY, screenWidth, screenHeight, cameraRef);
                glm::vec3 intersectionPoint;

                // Se encontrou o chão (Y=0.0f)
                if (m_raycaster.getIntersectionWithPlane(0.0f, intersectionPoint))
                {
                    Component::Movement &movementComp = m_world.getComponent<Component::Movement>(playerID);
                    movementComp.targetDestination = Engine::Math::Vec3(intersectionPoint);
                    movementComp.isMovingToDestination = true;
                    movementComp.isCameraOrbitModeActive = true;
                }
            }
        }

        void InputService::registerCallbacks(Engine::Camera::ICamera &cameraRef)
        {
            auto &inputManager = InputManager::Get();
            GLFWwindow *window = m_glfwWindow; // Para simplificar o uso do ponteiro da janela

            // 1. MOUSE DOWN (LIGA A ROTAÇÃO)
            inputManager.RegisterCallback(InputEvent::MouseButtonPressed, GLFW_MOUSE_BUTTON_RIGHT,
                                          [window, &cameraRef](const InputEventData &data)
                                          {
                                              glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                                              cameraRef.resetMouseState();
                                          });

            // 2. MOUSE UP (DESLIGA A ROTAÇÃO)
            inputManager.RegisterCallback(InputEvent::MouseButtonReleased, GLFW_MOUSE_BUTTON_RIGHT,
                                          [window](const InputEventData &data)
                                          {
                                              glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                                          });

            // 3. MOUSE MOVED (ROTAÇÃO DE FATO)
            inputManager.RegisterCallback(InputEvent::MouseMoved, 0,
                                          [this, &inputManager, &cameraRef](const InputEventData &data)
                                          {
                                              // Lógica do Mouse Moved: Gira a Câmera se RMB estiver pressionado
                                              if (inputManager.IsRightMouseButtonPressed())
                                              {
                                                  cameraRef.processMouseMovement(data.xpos, data.ypos);
                                              }
                                          });

            // 4. MOUSE SCROLLED (ZOOM)
            inputManager.RegisterCallback(InputEvent::MouseScrolled, 0,
                                          [&cameraRef](const InputEventData &data)
                                          {
                                              cameraRef.processScroll(data.yoffset);
                                          });

            // 5. RAYCASTING (CLICK-TO-MOVE) - Callback LMB
            // A lógica complexa agora é encapsulada em processLeftClick (que também é um método do Service)
            inputManager.RegisterCallback(InputEvent::MouseButtonPressed, GLFW_MOUSE_BUTTON_LEFT,
                                          [this, &cameraRef](const InputEventData &data)
                                          {
                                              this->processLeftClick(cameraRef);
                                          });

            // 6. TECLA ESC (FECHAR JANELA)
            inputManager.RegisterCallback(InputEvent::KeyPressed, GLFW_KEY_ESCAPE,
                                          [window](const InputEventData &data)
                                          {
                                              glfwSetWindowShouldClose(window, true);
                                          });
        }

    } // namespace Input
} // namespace Engine