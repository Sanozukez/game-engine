// src/app/scene.cpp
#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "scene.h"
#include "./../../engine/render/shader.h"
#include "./../../engine/core/log.h"
#include "./../../engine/core/path_utils.h"
#include "./../../engine/core/config_manager.h"
#include "./../../engine/render/camera/free_camera.h"
#include "./../../engine/render/camera/orbit_camera.h"
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

    void Scene::addGameObject(std::unique_ptr<Engine::Game::GameObject> obj) {
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

        // // ====================================================================
        // // 1. NOVO FLUXO: CARREGAMENTO BINÁRIO (MMAP)
        // // ====================================================================
        // Engine::Log::Info("Engine::Scene::initialize() - Iniciando carregamento MMAP.");

        // Engine::SceneLoader sceneLoader;
        // // O nome do arquivo final do mapa é o que foi gerado pelo nosso compiler
        // bool loadSuccess = sceneLoader.loadMapBinary("assets/models/test_scene_1.mmap");

        // if (!loadSuccess)
        // {
        //     Engine::Log::Error("Falha crítica ao carregar arquivo de cena (.mmap).");
        //     return;
        // }

        // const auto &loaded_nodes = sceneLoader.nodes;
        // const SceneNode *terrainNode = nullptr;

        // // ====================================================================
        // // 1. LÓGICA DO TERRENO PRINCIPAL (Procura o TYPE_TERRAIN_BASE)
        // // ====================================================================
        // for (const auto &node : loaded_nodes)
        // {
        //     if (node.type == EntityType::TYPE_TERRAIN_BASE)
        //     { // <-- NOVO FILTRO
        //         terrainNode = &node;
        //         break;
        //     }
        // }

        // if (terrainNode)
        // {
        //     // Carrega a geometria do Terreno UMA VEZ, usando a posição do primeiro node estático
        //     auto terrainModel = Engine::Asset::GLTFLoader::loadGLTF("assets/models/test_scene_1.glb");
        //     auto terrainObject = std::make_unique<Engine::Game::GameObject>(std::move(terrainModel));
        //     terrainObject->setPosition(glm::vec3(terrainNode->position[0], terrainNode->position[1], terrainNode->position[2]));

        //     m_terrain = terrainObject.get();
        //     m_gameObjects.push_back(std::move(terrainObject));

        //     Engine::Log::Info("GameObject Terreno BASE carregado via MMAP!");
        // }
        // else
        // {
        //     Engine::Log::Error("Nenhum Node Estático encontrado para ser o Terreno Base.");
        // }

        // // ====================================================================
        // // 2. ITERAÇÃO E INSTANCIAÇÃO DOS OBJETOS SECUNDÁRIOS (DATA-DRIVEN)
        // // ====================================================================

        // // Acessa o Asset Manager global (Singleton)
        // Engine::Asset::AssetManager &assetManager = Engine::Asset::AssetManager::Get();

        // // --- LÓGICA DE INSTANCIAÇÃO (ARRAY E ESTÁTICOS) ---

        // for (const auto &node : sceneLoader.nodes)
        // {
        //     // Ignoramos os nodes de controle (TERRAIN_BASE) e os originais ARRAY_START/END
        //     if (node.type == EntityType::TYPE_TERRAIN_BASE || node.type == EntityType::TYPE_ARRAY_START)
        //     {
        //         continue;
        //     }

        //     // Se o node for do tipo STATIC_MESH (os módulos gerados pelo Array)
        //     if (node.type == EntityType::TYPE_STATIC_MESH)
        //     {
        //         // 1. Otimização: Elimina a string hardcoded e usa o ID numérico salvo no MMAP
        //         uint32_t asset_id = node.asset_reference_id;

        //         // 2. A Engine AGORA só precisa saber o ID. O Asset Manager faz o resto.
        //         std::shared_ptr<Engine::Asset::Model> baseModel = assetManager.getModel(asset_id); // <-- CHAMADA FINAL!

        //         if (!baseModel)
        //         {
        //             // NOVO: Traduz o ID para o nome (string) APENAS para o log de erro!
        //             std::string asset_name_for_log = assetManager.getAssetPathByID(asset_id);

        //             Engine::Log::Error(std::format("Asset Manager: Falha ao obter modelo '{}' (ID: {}). Pulando instanciação.",
        //                                            asset_name_for_log,
        //                                            asset_id));
        //             continue;
        //         }
        //         // 2. Instanciação: Cria o GameObject, aplica Posição e Rotação

        //         // Cria uma cópia profunda (Deep Copy) da ESTRUTURA Model para a nova instância
        //         auto moduleModelCopy = baseModel->clone();

        //         auto moduleObject = std::make_unique<Engine::Game::GameObject>(std::move(moduleModelCopy));

        //         // Posição e Rotação (Conversão do Quatérnio MMAP)
        //         glm::quat rotationQuat(
        //             node.rotation_quat[0], // W
        //             node.rotation_quat[1], // X
        //             node.rotation_quat[2], // Y
        //             node.rotation_quat[3]  // Z
        //         );

        //         moduleObject->setPosition(glm::vec3(node.position[0], node.position[1], node.position[2]));
        //         moduleObject->setRotation(rotationQuat);

        //         m_gameObjects.push_back(std::move(moduleObject));
        //     }

        //     // ... (Lógica futura para NPCs e Triggers)
        // }

        // // 2. GameObject do Personagem
        // try
        // {
        //     auto characterModel = Engine::Asset::GLTFLoader::loadGLTF("assets/models/character_placeholder.glb");
        //     if (characterModel)
        //     {
        //         auto characterObject = std::make_unique<Engine::Game::PlayerCharacter>(std::move(characterModel));
        //         characterObject->name = "PlayerCharacter";
        //         characterObject->setPosition(glm::vec3(0.0f, 0.9f, 0.0f)); // Posição fixa por enquanto

        //         auto &config = Engine::ConfigManager::Get();
        //         characterObject->setCameraFocusHeight(config.getValue<float>("character.player.camera_focus_height", 1.0f));
        //         Engine::Log::Info("Altura de foco da câmera configurada a partir do JSON.");

        //         characterObject->setMovementSpeed(config.getValue<float>("character.player.movement_speed", 5.0f));
        //         characterObject->setRotationSpeed(config.getValue<float>("character.player.rotation_speed_degrees", 75.0f));
        //         Engine::Log::Info("Velocidades do PlayerCharacter configuradas a partir do arquivo JSON.");

        //         m_playerCharacter = characterObject.get();
        //         m_gameObjects.push_back(std::move(characterObject));
        //         Engine::Log::Info("GameObject Personagem (cubo) carregado e adicionado à cena!");
        //     }
        //     else
        //     {
        //         Engine::Log::Error("Falha ao criar GameObject do Personagem: Modelo nulo.");
        //     }
        // }
        // catch (const std::exception &e)
        // {
        //     Engine::Log::Error(std::format("Erro ao carregar GameObject do Personagem (cubo): {}", e.what()));
        // }

        // glEnable(GL_DEPTH_TEST);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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