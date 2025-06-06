// src/app/scene.cpp
#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "scene.h"
#include "./../../engine/render/shader.h"
#include "./../../engine/core/log.h"
#include "./../../engine/core/path_utils.h"

#include "./../../engine/render/camera/free_camera.h"
#include "./../../engine/render/camera/orbit_camera.h"

#include "./../../engine/asset/obj_loader.h"
#include "./../../engine/render/texture.h"

#include "./../../engine/asset/gltf_loader.h"
#include "./../../engine/game/game_object.h"
#include "./../../engine/game/player_character.h"
#include "./../../engine/input/input_manager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <memory>

#include "./../../engine/core/config.h"

namespace Engine
{

  Scene::Scene()
      : shader(nullptr), m_playerCharacter(nullptr)
  {
    if (Engine::CAMERA_DEFAULT_IS_FREE)
    {
      m_camera = std::make_unique<Engine::Camera::FreeCamera>();
    }
    else
    {
      m_camera = std::make_unique<Engine::Camera::OrbitCamera>();
    }

    Engine::Log::Info(std::format("Engine::Scene::Scene() - Construtor chamado. Câmera inicial: {}",
                                  Engine::CAMERA_DEFAULT_IS_FREE ? "FreeCamera" : "OrbitCamera"));
  }

  Scene::~Scene()
  {
    Engine::Log::Info("Engine::Scene::~Scene() - Destruindo objetos da cena.");
  }

  void Scene::initialize()
  {
    Engine::Log::Info("Engine::Scene::initialize() - início");
    try
    {
      shader = std::make_unique<Engine::Render::Shader>("engine/shaders/basic.vert", "engine/shaders/basic.frag");
      Engine::Log::Info("Shader carregado com sucesso!");
    }
    catch (const std::exception &e)
    {
      Engine::Log::Error(std::format("Erro ao carregar shader: {}", e.what()));
      return;
    }

    // 1. GameObject do Terreno
    try
    {
      auto terrainModel = Engine::Asset::GLTFLoader::loadGLTF("assets/models/map_test.glb");
      if (terrainModel)
      {
        auto terrainObject = std::make_unique<Engine::Game::GameObject>(std::move(terrainModel));
        terrainObject->name = "Terrain";
        terrainObject->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        m_gameObjects.push_back(std::move(terrainObject));
        Engine::Log::Info("GameObject Terreno carregado e adicionado à cena!");
      }
      else
      {
        Engine::Log::Error("Falha ao criar GameObject do Terreno: Modelo nulo.");
      }
    }
    catch (const std::exception &e)
    {
      Engine::Log::Error(std::format("Erro ao carregar GameObject do Terreno: {}", e.what()));
    }

    // 2. GameObject do Personagem (Cubo Simulado)
    try
    {
      auto characterModel = Engine::Asset::GLTFLoader::loadGLTF("assets/models/character_placeholder.glb");
      if (characterModel)
      {
        auto characterObject = std::make_unique<Engine::Game::PlayerCharacter>(std::move(characterModel));
        characterObject->name = "PlayerCharacter";
        characterObject->setPosition(glm::vec3(0.0f, 0.9f, 0.0f));

        m_playerCharacter = characterObject.get();
        m_gameObjects.push_back(std::move(characterObject));
        Engine::Log::Info("GameObject Personagem (cubo) carregado e adicionado à cena!");

        if (!Engine::CAMERA_DEFAULT_IS_FREE && m_playerCharacter)
        {
          m_camera->setTarget(m_playerCharacter->getPosition());
          // NOVO: Sincronizar yaw da câmera com o yaw inicial do personagem
          m_camera->setYaw(m_playerCharacter->getRotationYaw());
          Engine::Log::Debug(std::format("OrbitCamera target set to PlayerCharacter at {}. Initial Yaw: {}.",
                                         glm::to_string(m_playerCharacter->getPosition()), m_playerCharacter->getRotationYaw()));
        }
      }
      else
      {
        Engine::Log::Error("Falha ao criar GameObject do Personagem: Modelo nulo.");
      }
    }
    catch (const std::exception &e)
    {
      Engine::Log::Error(std::format("Erro ao carregar GameObject do Personagem (cubo): {}", e.what()));
    }

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (Engine::CAMERA_DEFAULT_IS_FREE)
    {
      m_camera->setPosition(glm::vec3(25.0f, 15.0f, 25.0f));
    }
    else
    {
      // Para OrbitCamera, o target já foi definido para o personagem
      // (m_camera->setTarget e setYaw feito acima).
    }
    Engine::Log::Info(std::format("Camera posicionada em {}", glm::to_string(m_camera->getPosition())));

    Engine::Log::Info("Engine::Scene::initialize() - fim");
  }

  // src/app/scene.cpp (relevant Scene::update() method)
  void Scene::update(float deltaTime, const Input::InputManager &inputManager)
  {
    if (m_playerCharacter)
    {
      // **** MUDANÇA: Passar a câmera para o PlayerCharacter::update ****
      m_playerCharacter->update(deltaTime, inputManager, *m_camera);
      // Se a câmera é OrbitCamera, ela deve seguir o personagem após o update dele.
      if (!Engine::CAMERA_DEFAULT_IS_FREE)
      {
        m_camera->setTarget(m_playerCharacter->getPosition());
        // **** REMOVIDO: Sincronização de yaw aqui (feito em PlayerCharacter::update) ****
        // m_camera->setYaw(m_playerCharacter->getRotationYaw());
      }
    }
  }

  void Scene::render(const glm::mat4 &projection, const glm::mat4 &view) const
  {
    if (!shader)
    {
      Engine::Log::Error("Shader não inicializado. Pulando renderização da Engine::Scene.");
      return;
    }

    shader->use();

    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    Engine::Log::Debug(std::format("[DEBUG] Shader ativo OpenGL: {} | Shader ID: {}", currentProgram, shader->getID()));

    Engine::Log::Debug(std::format("Camera pos: {}", glm::to_string(m_camera->getPosition())));
    Engine::Log::Debug(std::format("View matrix:\n{}", glm::to_string(view)));
    Engine::Log::Debug(std::format("Projection matrix:\n{}", glm::to_string(projection)));

    shader->setMat4("uProjection", projection);
    shader->setMat4("uView", view);

    shader->setVec3("uLightPos", glm::vec3(50.0f, 50.0f, 50.0f));
    shader->setVec3("uViewPos", m_camera->getPosition());

    // Desenhar todos os GameObjects da cena
    for (const auto &gameObject_ptr : m_gameObjects)
    {
      if (gameObject_ptr)
      {
        gameObject_ptr->draw(*shader);
      }
    }
  }

  Engine::Camera::ICamera &Scene::getCamera()
  {
    return *m_camera;
  }

  void Scene::setCamera(std::unique_ptr<Engine::Camera::ICamera> camera)
  {
    m_camera = std::move(camera);
    Engine::Log::Info("Scene: Câmera da cena alterada.");
  }

} // namespace Engine