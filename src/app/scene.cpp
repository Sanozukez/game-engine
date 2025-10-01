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
#include <format>

#include "./../../engine/core/config.h"

namespace Engine
{

    Scene::Scene()
        : shader(nullptr), m_playerCharacter(nullptr), m_terrain(nullptr) // Inicializa o novo ponteiro
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

                // --- MUDANÇA AQUI ---
                // Armazena um ponteiro bruto para o terreno para acesso rápido.
                m_terrain = terrainObject.get();

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
        // (Nenhuma mudança nesta seção)
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

        Engine::Log::Info(std::format("Camera posicionada em {}", glm::to_string(m_camera->getPosition())));
        Engine::Log::Info("Engine::Scene::initialize() - fim");
    }

    void Scene::update(float deltaTime, const Input::InputManager &inputManager)
    {
        for (auto &obj : m_gameObjects)
        {
            if (obj)
            {
                // --- MUDANÇA AQUI ---
                // Passa a própria cena (*this) para o método update dos GameObjects.
                obj->update(deltaTime, inputManager, *this, *m_camera);
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

        // Define as matrizes de projeção e visão no shader
        shader->setMat4("uProjection", projection);
        shader->setMat4("uView", view);

        // Define uniformes globais para iluminação e posição da câmera
        shader->setVec3("uLightPos", glm::vec3(50.0f, 50.0f, 50.0f));
        shader->setVec3("uViewPos", m_camera->getPosition());

        // Itera e desenha todos os GameObjects da cena
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

    Engine::Game::PlayerCharacter *Scene::getPlayer()
    {
        return m_playerCharacter;
    }

    // --- NOVA IMPLEMENTAÇÃO ADICIONADA ---
    Engine::Game::GameObject *Scene::getTerrain()
    {
        return m_terrain;
    }

} // namespace Engine