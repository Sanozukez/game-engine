// src/app/input.h
#pragma once

#include <GLFW/glfw3.h> 
#include "./../../engine/render/camera/icamera.h" 
#include "scene.h" 

// Forward declarations para as classes necessárias
namespace Engine { namespace Camera { class ICamera; } }
namespace Engine { namespace Game { class GameObject; } } 
// **** MUDANÇA AQUI: Forward declaration para InputManager (agora no namespace correto) ****
namespace Engine { namespace Input { class InputManager; } } 

// Função para configurar os callbacks de input da aplicação
void setup_application_input(GLFWwindow* window, Engine::Camera::ICamera& camera, Engine::Scene& scene, float deltaTime); 

// Função para processar input contínuo (chamada no loop principal)
void process_continuous_input(Engine::Game::GameObject* playerCharacter, float deltaTime);