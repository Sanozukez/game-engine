// src/app/scene.cpp
#define GLFW_INCLUDE_NONE
// #include <glad/gl.h>
#include "../../engine/render/opengl_types.h"
#include <GLFW/glfw3.h>

#include "scene.h"
#include "./../../engine/render/shader.h"
#include "./../../engine/core/log.h"
#include "./../../engine/core/path_utils.h"
#include "./../../engine/core/config_manager.h"
#include "./../../engine/camera/free_camera.h"
#include "./../../engine/camera/orbit_camera.h"
#include "./../../engine/core/SceneLoader.h"
#include "./../../shared/mmap_format/SceneFileFormat.h"
#include "./../../engine/asset/obj_loader.h"
#include "./../../engine/asset/asset_manager.h"
#include "./../../engine/render/texture.h"
#include "./../../engine/asset/gltf_loader.h"
#include "./../../engine/game/world_initializer.h"
#include "./../../engine/game/game_object.h"
#include "./../../engine/game/player_character.h"
#include "./../../engine/input/input_manager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <memory>
#include <format>
#include <map>
#include <string>

namespace Engine
{

    Scene::Scene()
        : shader(nullptr), m_playerCharacter(nullptr), m_terrain(nullptr)
    {
        // Pega o tipo de câmera do ConfigManager
        auto &config = ConfigManager::Get();
        std::string cameraType = config.getValue<std::string>("camera.default_type", "orbit");

        if (cameraType == "free")
        {
            m_camera = std::make_unique<Engine::Camera::FreeCamera>();
        }
        else // "orbit" ou qualquer outro valor se torna o padrão
        {
            m_camera = std::make_unique<Engine::Camera::OrbitCamera>();
        }

        Engine::Log::Info(std::format("Engine::Scene::Scene() - Construtor chamado. Câmera inicial a partir do JSON: {}", cameraType));
    }

    Scene::~Scene()
    {
        Engine::Log::Info("Engine::Scene::~Scene() - Destruindo objetos da cena.");
    }

    void Scene::addGameObject(std::unique_ptr<Engine::Game::GameObject> obj)
    {
        m_gameObjects.push_back(std::move(obj));
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

        // 2. CARREGAMENTO DO MUNDO (Chamada Limpa para o WorldInitializer)
        // O WorldInitializer fará todo o MMAP loading e instanciação
        Engine::Game::WorldInitializer::Initialize(*this, m_playerCharacter, m_terrain);

        // Verificar camera dinamicamente

        // Se a câmera for uma OrbitCamera, configura o alvo e os limites.
        if (auto *orbitCam = dynamic_cast<Engine::Camera::OrbitCamera *>(m_camera.get()))
        {
            if (m_playerCharacter)
            {
                orbitCam->setTarget(m_playerCharacter->getPosition());
                orbitCam->setYaw(m_playerCharacter->getRotationYaw());

                auto &config = Engine::ConfigManager::Get();
                orbitCam->setDistanceLimits(
                    config.getValue<float>("camera.orbit.min_distance", 2.0f),
                    config.getValue<float>("camera.orbit.max_distance", 25.0f));
                orbitCam->setPitchLimits(
                    config.getValue<float>("camera.orbit.min_pitch_degrees", -20.0f),
                    config.getValue<float>("camera.orbit.max_pitch_degrees", 85.0f));
                Engine::Log::Info("Limites da OrbitCamera configurados a partir do arquivo JSON.");
            }
        }
        // Se a câmera for uma FreeCamera, define sua posição inicial.
        else if (auto *freeCam = dynamic_cast<Engine::Camera::FreeCamera *>(m_camera.get()))
        {
            freeCam->setPosition(glm::vec3(25.0f, 15.0f, 25.0f));
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