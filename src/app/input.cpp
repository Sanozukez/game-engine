// src/app/input.cpp
#define GLFW_INCLUDE_NONE

// Este arquivo é responsável por configurar os callbacks de input para a aplicação.

#include "input.h"
#include "app.h"

#include <GLFW/glfw3.h> // Incluir GLFW DEPOIS de GLFW_INCLUDE_NONE e antes de qualquer GLAD
// **** MUDANÇA AQUI: Incluir InputManager (agora no namespace correto) ****
#include "./../../engine/input/input_manager.h"
#include "./../../engine/render/camera/icamera.h"
#include "scene.h"
#include "./../../engine/core/log.h"

// Incluir GameObject para acessar seu update/processKeyboard
#include "./../../engine/game/game_object.h"

// Função para configurar todos os callbacks de input da aplicação
void setup_application_input(GLFWwindow *window, Engine::Camera::ICamera &camera, Engine::Scene &scene, float deltaTime)
{
    auto &inputManager = Engine::Input::InputManager::Get();

    inputManager.ProcessInput(window);

    inputManager.RegisterCallback(Engine::Input::InputEvent::MouseButtonPressed, GLFW_MOUSE_BUTTON_RIGHT,
                                  [window, &camera](const Engine::Input::InputEventData &data) { // **** MUDANÇA AQUI ****
                                      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                                      camera.resetMouseState();
                                      Engine::Log::Debug("src/app/input.cpp: Botão direito do mouse pressionado. Cursor desabilitado e estado da câmera resetado.");
                                  });
    inputManager.RegisterCallback(Engine::Input::InputEvent::MouseButtonReleased, GLFW_MOUSE_BUTTON_RIGHT,
                                  [window](const Engine::Input::InputEventData &data) { // **** MUDANÇA AQUI ****
                                      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                                      Engine::Log::Debug("src/app/input.cpp: Botão direito do mouse solto. Cursor normal.");
                                  });

    inputManager.RegisterCallback(Engine::Input::InputEvent::MouseMoved, 0,
                                  [&camera](const Engine::Input::InputEventData &data) { // **** MUDANÇA AQUI ****
                                      auto &manager = Engine::Input::InputManager::Get();
                                      if (!manager.IsRightMouseButtonPressed())
                                      {
                                          return;
                                      }

                                      camera.processMouseMovement(data.xpos, data.ypos);
                                  });

    inputManager.RegisterCallback(Engine::Input::InputEvent::MouseScrolled, 0,
                                  [&camera](const Engine::Input::InputEventData &data) { // **** MUDANÇA AQUI ****
                                      camera.processScroll(data.yoffset);
                                      Engine::Log::Debug(std::format("src/app/input.cpp: Scroll Y: {}", data.yoffset));
                                  });

    inputManager.RegisterCallback(Engine::Input::InputEvent::KeyPressed, GLFW_KEY_ESCAPE,
                                  [window](const Engine::Input::InputEventData &data) { // **** MUDANÇA AQUI ****
                                      glfwSetWindowShouldClose(window, true);
                                      Engine::Log::Info("ESC pressionado. Fechando janela.");
                                  });
}

// Adaptação da função process_continuous_input para usar o GameObject do personagem
void process_continuous_input(Engine::Game::GameObject *playerCharacter, float deltaTime)
{
    Engine::Log::Trace("process_continuous_input: Chamado. Lógica de input do personagem movida para GameObject::update().");
}