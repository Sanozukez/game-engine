// // src/app/app.cpp

#include "app.h"
#include "./../../engine/window/window.h"
#include "./../../engine/core/log.h"
#include "./../../engine/ecs/world.h"

#include "./../../engine/ecs/systems/render_system.h"
#include "./../../engine/ecs/systems/camera_system.h"
#include "./../../engine/ecs/systems/player_system.h"
#include "./../../engine/ecs/world_loader.h"
#include "./../../engine/math/vec3.h"
#include "./../../engine/camera/free_camera.h"
#include "./../../engine/camera/orbit_camera.h"
#include "./../../engine/render/renderer.h"
#include "./../../engine/core/config_manager.h"
#include "./../../engine/input/input_manager.h"
#include "./../../engine/input/input_service.h"
#include "./../../engine/physics/raycaster.h"
// #include "./../../engine/ecs/systems/camera_target_system.h" // NOVO: Target System
#include "./../../engine/ecs/systems/camera_system.h"
#include "./../../engine/ecs/components/player_component.h"
#include "./../../engine/ecs/components/movement_component.h"
#include "./../../engine/ecs/components/transform_component.h"
#include "./../../engine/ecs/systems/terrain_tracking_system.h"

#include <glm/gtx/string_cast.hpp>
#include <thread>
#include <chrono>
#include <glm/glm.hpp>
#include <iostream>
#include <format>
#include <limits>
#include <GLFW/glfw3.h> // Necessário para as funções GLFW no APP

using namespace Engine::ECS;
using namespace Engine::ECS::System;
using namespace Engine::ECS::Component;

// ----------------------------------------------------------------------------------
// Construtor e Destrutor
// ----------------------------------------------------------------------------------

App::App() : m_window(nullptr), m_renderer(nullptr), m_gameWorld(std::make_unique<World>())
{
    Engine::Log::Info("[App] Construtor chamado");
}

App::~App() = default;

void App::run()
{
    Engine::Log::Info("[App] Iniciando aplicação");

    if (!Engine::ConfigManager::Get().load("config/engine_settings.json"))
    {
        Engine::Log::Critical("[App] Falha ao carregar configurações essenciais. Encerrando.");
        return;
    }

    // --- VARIÁVEIS DE INICIALIZAÇÃO ---
    auto &config = Engine::ConfigManager::Get();
    Engine::WindowConfig winConfig;
    winConfig.width = config.getValue<int>("window.width", 1280);
    winConfig.height = config.getValue<int>("window.height", 720);
    winConfig.title = config.getValue<std::string>("window.title", "Default Title");
    winConfig.resizable = config.getValue<bool>("window.resizable", false);
    winConfig.fullscreen = config.getValue<bool>("window.fullscreen", false);
    winConfig.maximized = false;

    // --- INICIALIZAÇÃO DA JANELA ---
    try
    {
        m_window = std::make_unique<Engine::Window>(winConfig);
    }
    catch (const std::exception &e)
    {
        Engine::Log::Critical(std::format("[App] Erro fatal na inicialização da janela: {}", e.what()));
        return;
    }

    GLFWwindow *glfwWindow = m_window->getGLFWWindow();
    if (!glfwWindow)
    {
        Engine::Log::Critical("[App] Ponteiro GLFWwindow inválido após criação da janela.");
        return;
    }
    Engine::Input::InputManager::Get().ProcessInput(glfwWindow); // Configura Callbacks GLFW

    // --- INICIALIZAÇÃO DA CÂMERA E RENDERER ---
    std::string cameraType = config.getValue<std::string>("camera.default_type", "orbit");

    // NOVO: Ponteiro temporário para acessar métodos específicos da OrbitCamera, se aplicável
    Engine::Camera::OrbitCamera *orbitCameraPtr = nullptr;

    if (cameraType == "free")
    {
        m_mainCamera = std::make_unique<Engine::Camera::FreeCamera>();
    }
    else // Assumimos "orbit"
    {
        auto orbitCam = std::make_unique<Engine::Camera::OrbitCamera>();

        // Armazena o ponteiro bruto ANTES de mover o unique_ptr
        orbitCameraPtr = orbitCam.get();

        m_mainCamera = std::move(orbitCam);
    }
    Engine::Log::Info(std::format("[App] Câmera inicializada a partir do JSON: {}", cameraType));

    // NOVO: APLICAÇÃO DOS LIMITES DA CÂMERA DE ÓRBITA
    if (orbitCameraPtr)
    {
        // Leitura dos valores do JSON (usando o valor padrão como fallback)
        float minDist = config.getValue<float>("camera.orbit.min_distance", 3.0f);
        float maxDist = config.getValue<float>("camera.orbit.max_distance", 13.0f);
        float minPitch = config.getValue<float>("camera.orbit.min_pitch_degrees", -55.0f);
        float maxPitch = config.getValue<float>("camera.orbit.max_pitch_degrees", 15.0f);

        // Configura a distância (Zoom) e os ângulos (Pitch) na instância da câmera
        orbitCameraPtr->setDistanceLimits(minDist, maxDist);
        orbitCameraPtr->setPitchLimits(minPitch, maxPitch);

        Engine::Log::Info(std::format("[App] Limites da OrbitCamera configurados. Distância ({}, {}), Pitch ({}, {})", minDist, maxDist, minPitch, maxPitch));
    }

    m_renderer = std::make_unique<Engine::Render::Renderer>(*m_window, *m_mainCamera.get());
    Engine::Render::Renderer &rendererRef = *m_renderer;

    // === NOVO BLOCO: CARREGAR E INJETAR LUZ GLOBAL ===

    // Acessa o nó raiz da configuração para navegação manual
    const auto &rootNode = config.getRootNode();
    const nlohmann::json *lightConfig = nullptr;

    // Tenta obter a subseção de luz com verificação de ponteiro
    if (rootNode.contains("world") && rootNode["world"].contains("global_light"))
    {
        lightConfig = &rootNode["world"]["global_light"];
    }

    if (lightConfig)
    {
        // Leitura dos valores JSON (Navegação segura por índice de array)
        glm::vec3 lightPos(
            lightConfig->value("position", nlohmann::json::array({50.0f, 50.0f, 50.0f}))[0].get<float>(),
            lightConfig->value("position", nlohmann::json::array({50.0f, 50.0f, 50.0f}))[1].get<float>(),
            lightConfig->value("position", nlohmann::json::array({50.0f, 50.0f, 50.0f}))[2].get<float>());
        glm::vec3 lightColor(
            lightConfig->value("color", nlohmann::json::array({1.0f, 1.0f, 1.0f}))[0].get<float>(),
            lightConfig->value("color", nlohmann::json::array({1.0f, 1.0f, 1.0f}))[1].get<float>(),
            lightConfig->value("color", nlohmann::json::array({1.0f, 1.0f, 1.0f}))[2].get<float>());
        float lightIntensity = lightConfig->value("intensity", 30.0f);

        // Chamadas aos setters implementados no Renderer
        rendererRef.setGlobalLightPos(lightPos);
        rendererRef.setGlobalLightColor(lightColor);
        rendererRef.setGlobalLightIntensity(lightIntensity);

        Engine::Log::Info(std::format("[App] Luz Global carregada de {} com intensidade {}.",
                                      glm::to_string(lightPos), lightIntensity));
    }
    else
    {
        Engine::Log::Warn("[App] Nenhuma seção 'world.global_light' encontrada. Usando defaults.");
        // NOTA: Se os setters de luz foram definidos, você pode querer chamar os setters com os valores padrão aqui.
    }

    rendererRef.setClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    Engine::Log::Info("[App] Renderer inicializado e configurado.");

    // =========================================================================
    // 3. CONFIGURAÇÃO DO ECS (Sistemas, Entities e Carregamento)
    // =========================================================================

    // A. CRIAÇÃO DAS ENTIDADES ALVO (Mínimo de Memória antes do Load)
    Engine::ECS::EntityID playerEntity = m_gameWorld->createEntity();
    const uint32_t PLAYER_ASSET_ID = 614879287; // character_placeholder.glb

    m_gameWorld->addMesh(playerEntity, PLAYER_ASSET_ID);
    m_gameWorld->getTransform(playerEntity).position = Engine::Math::Vec3(0.0f, -100.0f, 0.0f);
    m_gameWorld->addComponent<Player>(playerEntity);
    m_gameWorld->addComponent<Movement>(playerEntity);
    m_gameWorld->addComponent<TerrainTracker>(playerEntity);
    // Componente CameraTarget FOI REMOVIDO DAQUI para simplicidade.

    Component::Movement &movementComponent = m_gameWorld->getComponent<Component::Movement>(playerEntity);
    movementComponent.isCameraOrbitModeActive = false;

    // C. CARREGAMENTO DO MUNDO (Ponto de Falha - Chamada Instável)
    if (!Engine::ECS::WorldLoader::Load(*m_gameWorld, playerEntity))
    {
        Engine::Log::Critical("[App] Falha ao carregar World Loader. Encerrando.");
        return;
    }

    // B. ADIÇÃO DOS SISTEMAS (Ordem de Execução do Jogo)
    Engine::Camera::ICamera &cameraRef = *m_mainCamera.get();

    // 1. PlayerSystem: Move X/Z/ROT e faz o tracking da Câmera (Onde a lógica funciona)
    // NOTA: A dependência ICamera DEVE SER REINTRODUZIDA NO PlayerSystem AGORA
    m_gameWorld->addSystem<PlayerSystem>(cameraRef); // <--- REINTRODUZINDO DEPENDÊNCIA

    // 2. TerrainTrackingSystem: Corrige a altura (Y).
    m_gameWorld->addSystem<TerrainTrackingSystem>();

    // 3. CameraTargetSystem NÃO É MAIS NECESSÁRIO
    // m_gameWorld->addSystem<CameraTargetSystem>(cameraRef); // REMOVIDO

    // 4. RenderSystem: Renderiza.
    m_gameWorld->addSystem<RenderSystem>(rendererRef);

    // === INICIALIZAÇÃO E TARGET (SIMPLES E FUNCIONAL) ===

    // 1. Obtém a Transform FINAL do Player
    Component::Transform &currentTransform = m_gameWorld->getTransform(playerEntity);
    Component::Movement &playerMovement = m_gameWorld->getComponent<Component::Movement>(playerEntity);

    // 2. CORREÇÃO DE FRAME 0 (Target/Yaw)
    // Fazemos o cálculo e o setTarget aqui. O TerrainTrackingSystem será chamado
    // no Game Loop para fazer a correção de Y.
    glm::quat playerQuat = currentTransform.rotation;
    float playerYawDegrees = glm::degrees(glm::yaw(playerQuat));
    glm::vec3 spawnPosGLM = currentTransform.position.toGLM();

    // Use o valor 1.0f para o focusHeight, pois o CameraTargetComponent foi removido do setup
    // Component::CameraTarget &camTarget = m_gameWorld->getComponent<CameraTarget>(playerEntity); // REMOVIDO
    float focusHeight = playerMovement.cameraFocusHeight; // Usamos o valor do MovementComponent

    glm::vec3 initialTargetPoint = spawnPosGLM + glm::vec3(0.0f, focusHeight, 0.0f);

    m_mainCamera->setYaw(playerYawDegrees);
    m_mainCamera->setTarget(initialTargetPoint);
    m_mainCamera->resetMouseState();

    // D. REGISTRO DE EVENTOS (Permanece igual)
    Engine::Input::InputService::Init(glfwWindow, *m_gameWorld.get());
    Engine::Input::InputService::Get().registerCallbacks(cameraRef);

    Engine::Log::Info("[App] ECS e Sistemas de Input configurados.");

    // === LOOP DE RENDERIZAÇÃO ===
    float lastFrame = 0.0f;
    float deltaTime = 0.0f;

    while (!m_window->shouldClose())
    {
        float currentFrame = m_window->getTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        auto &inputManager = Engine::Input::InputManager::Get();

        // Lógica de Teclas de Controle (F11, F10) - Permanece no App
        if (inputManager.IsKeyPressed(GLFW_KEY_F11))
        {
            m_window->isMaximized() ? m_window->restore() : m_window->maximize();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        if (inputManager.IsKeyPressed(GLFW_KEY_F10))
        {
            int defaultWidth = config.getValue<int>("window.width", 1280);
            int defaultHeight = config.getValue<int>("window.height", 720);

            m_window->setResolutionAndMode(defaultWidth, defaultHeight, !m_window->isFullscreen());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        // MUNDO ECS: Chama a lógica de todos os sistemas (Render, Input, etc.)
        m_gameWorld->update(deltaTime);

        m_window->swapBuffersAndPollEvents();
    }

    Engine::Log::Info("[App] Encerrando aplicação.");
    glfwTerminate();
}
