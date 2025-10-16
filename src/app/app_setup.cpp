// // engine/core/app_setup.cpp

#include "app_setup.h"
#include "./../../core/log.h"
#include "./../../ecs/systems/player_system.h"
#include "./../../ecs/systems/animation_system.h"
#include "./../../ecs/systems/terrain_tracking_system.h"
#include "./../../ecs/systems/render_system.h"
#include "./../../ecs/world_loader.h" // Incluir o WorldLoader diretamente
#include "./../../input/input_service.h"
#include "./../../ecs/components/all_components.h" // Incluir todos os componentes
#include "./../../ecs/components/component_signature.h"
#include "./../../camera/orbit_camera.h" // <--- INCLUSÃO CRÍTICA (OrbitCamera)
#include "./../../ecs/components/camera_input_component.h"
#include "./../../engine/core/config_manager.h" // Importar para acessar o JSON
#include "./../../engine/asset/asset_manager.h"

#include "./../../input/i_keyboard_listener.h"
#include "./../../input/i_mouse_listener.h"
#include "./../../input/i_scroll_listener.h"
#include "./../../ecs/systems/camera_input_system.h"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <format>
#include <nlohmann/json.hpp>

namespace Engine
{
    namespace Core
    {

        using namespace Engine::ECS;
        using namespace Engine::ECS::System;
        using namespace Engine::ECS::Component;
        using nlohmann::json; // Atalho para o nlohmann::json

        // -------------------------------------------------------------------------
        // 1. CONFIGURAÇÃO (LIMITES DA CÂMERA E LUZ GLOBAL)
        // -------------------------------------------------------------------------

        bool AppSetup::InitializeConfiguration(
            Engine::Core::ConfigManager &config, // Corrigido namespace Core:: para Engine::
            Engine::Render::Renderer &rendererRef,
            Engine::Camera::ICamera &cameraRef,
            GLFWwindow *glfwWindow)
        {
            // NOVO: Ponteiro temporário para acessar métodos específicos da OrbitCamera.
            Engine::Camera::OrbitCamera *orbitCameraPtr = nullptr;

            cameraRef.applyExternalConfig(config);

            // --- CARREGAR LUZ GLOBAL (CORRIGIDO PARA USO DE JSON SEGURO) ---
            const auto &rootNode = config.getRootNode();
            const json *lightConfig = nullptr;

            if (rootNode.contains("world") && rootNode["world"].contains("global_light"))
            {
                lightConfig = &rootNode["world"]["global_light"];
            }

            if (lightConfig)
            {
                // Leitura dos valores JSON
                glm::vec3 lightPos(
                    lightConfig->value("position", json::array({50.0f, 50.0f, 50.0f}))[0].get<float>(),
                    lightConfig->value("position", json::array({50.0f, 50.0f, 50.0f}))[1].get<float>(),
                    lightConfig->value("position", json::array({50.0f, 50.0f, 50.0f}))[2].get<float>());
                glm::vec3 lightColor(
                    lightConfig->value("color", json::array({1.0f, 1.0f, 1.0f}))[0].get<float>(),
                    lightConfig->value("color", json::array({1.0f, 1.0f, 1.0f}))[1].get<float>(),
                    lightConfig->value("color", json::array({1.0f, 1.0f, 1.0f}))[2].get<float>());
                float lightIntensity = lightConfig->value("intensity", 30.0f);

                rendererRef.setGlobalLightPos(lightPos);
                rendererRef.setGlobalLightColor(lightColor);
                rendererRef.setGlobalLightIntensity(lightIntensity);
                Log::Info(std::format("[AppSetup] Luz Global configurada."));
            }

            return true;
        }

        bool AppSetup::InitializeECS(
            Engine::ECS::World &world,
            Engine::Camera::ICamera &cameraRef,
            Engine::Render::Renderer &rendererRef,
            GLFWwindow *glfwWindow)
        {
            using namespace Engine::ECS::Component;
            using namespace Engine::ECS;

            // --- 1. CONFIGURAÇÃO BASE (Componentes, Entidades, Carregamento) ---
            world.registerComponent<Transform>();
            world.registerComponent<Mesh>();
            world.registerComponent<Player>();
            world.registerComponent<Movement>();
            world.registerComponent<Terrain>();
            world.registerComponent<TerrainTracker>();
            world.registerComponent<CameraTarget>();
            world.registerComponent<CameraInput>();
            world.registerComponent<Animation>();
            Log::Info("[AppSetup] Todos os Componentes ECS registrados com sucesso.");

            // [AppSetup] Lendo configurações do JSON
            auto &config = ConfigManager::Get();
            auto &assetManager = Engine::Asset::AssetManager::Get(); // Acesso ao AssetManager

            // --------------------------------------------------------------------------------
            // 1. OBTENDO CONFIGURAÇÕES DO JSON (Usando as variáveis já declaradas no topo)
            // --------------------------------------------------------------------------------
            // config e assetManager foram lidos no topo, usaremos as variáveis
            // que estão no escopo (linhas 106-114 do seu código)

            std::string playerModelName = config.getValue<std::string>("character.player.model_name", "character_placeholder.glb");
            glm::vec3 startPos = config.getValue<glm::vec3>("character.player.start_position", glm::vec3(0.0f, 50.0f, 0.0f));
            const uint32_t playerAssetID = assetManager.getAssetIDByName(playerModelName);

            if (playerAssetID == 0)
            {
                Log::Critical(std::format("[Setup] Falha ao resolver Asset ID para: {}. Player não será criado.", playerModelName));
                return false;
            }

            // --------------------------------------------------------------------------------
            // 2. CRIAÇÃO E CONSTRUÇÃO EXPLÍCITA DOS COMPONENTES (RESOLUÇÃO DO C2672)
            // --------------------------------------------------------------------------------
            Engine::ECS::EntityID playerEntity = world.createEntity();

            // Transform Componente
            world.addComponent<Transform>(playerEntity);
            world.getComponent<Transform>(playerEntity).position = Engine::Math::Vec3(startPos);

            // Mesh Componente (CORRIGIDO: Construção explícita)
            Component::Mesh playerMeshConfig{playerAssetID};
            world.addComponent<Mesh>(playerEntity, playerMeshConfig);

            // Player Componente (vazio, Construção explícita)
            Component::Player playerConfig{};
            world.addComponent<Player>(playerEntity, playerConfig);

            // Inicializa o asset de animação com o mesmo assetID da Mesh.
            Component::Animation playerAnimConfig;
            playerAnimConfig.animationAssetID = playerAssetID;
            // Define o ID da animação inicial (ex: "IDLE"). Precisaremos do hash em runtime.
            // Por enquanto, usaremos um ID de fallback ou o hash de "IDLE" (hash("IDLE")).
            // Vamos definir uma função de hash temporária ou usar o AssetManager se ele for adequado.
            // Assumindo que você tem uma forma de fazer hash ou que o asset manager tem essa função:
            playerAnimConfig.currentAnimationID = 0; // Temporário, será o hash de "IDLE"
            world.addComponent<Animation>(playerEntity, playerAnimConfig);

            // Movement Componente (Configurado pelo JSON)
            Component::Movement playerMovementConfig;
            playerMovementConfig.movementSpeed = config.getValue<float>("character.player.movement_speed", 5.5f);
            playerMovementConfig.rotationSpeed = config.getValue<float>("character.player.rotation_speed_degrees", 80.0f);
            playerMovementConfig.cameraFocusHeight = config.getValue<float>("character.player.camera_focus_height", 1.2f);
            world.addComponent<Movement>(playerEntity, playerMovementConfig);

            // Componentes vazios (CORRIGIDO: Construção explícita)
            Component::TerrainTracker terrainTrackerConfig{};
            world.addComponent<TerrainTracker>(playerEntity, terrainTrackerConfig);

            Component::CameraInput cameraInputConfig{};
            world.addComponent<CameraInput>(playerEntity, cameraInputConfig);

            if (!Engine::ECS::WorldLoader::Load(world, playerEntity))
            {
                Log::Critical("[AppSetup] Falha ao carregar World Loader.");
                return false;
            }

            // D. INICIALIZAÇÃO DO INPUT (ANTES DE TUDO)
            Engine::Input::InputService::Init(glfwWindow, world);
            Engine::Input::InputManager &inputManagerRef = Engine::Input::InputManager::Get();
            Engine::ECS::ComponentTypeManager &typeManager = Engine::ECS::ComponentTypeManager::Get();
            Engine::Input::InputService &inputService = Engine::Input::InputService::Get(); // Referência única

            // --- 2. ADIÇÃO DOS SYSTEMS E REGISTRO DAS ASSINATURAS (FLUXO ÚNICO) ---

            // 1. CAMERA INPUT SYSTEM (Lida com o input da Câmera)
            // CRÍTICO: World::addSystem constrói o objeto no heap e o gerencia.
            // world.addSystem<Engine::ECS::System::CameraInputSystem>(cameraRef, inputManagerRef, inputService);
            world.addSystem<Engine::ECS::System::CameraInputSystem>(world, cameraRef, inputManagerRef, inputService);

            Engine::ECS::System::CameraInputSystem *camInputSystemPtr =
                world.getSystem<Engine::ECS::System::CameraInputSystem>();

            if (camInputSystemPtr == nullptr)
            {
                Log::Critical("Falha ao inicializar CameraInputSystem.");
                return false;
            }

            // REGISTRO DE LISTENERS NO INPUT SERVICE (ISP)
            inputService.registerKeyboardListener(camInputSystemPtr);
            inputService.registerMouseListener(camInputSystemPtr);
            inputService.registerScrollListener(camInputSystemPtr);
            inputService.registerMouseButtonListener(camInputSystemPtr);

            // REGISTRO DA ASSINATURA (Não requer Componentes)
            world.registerSystemSignature<Engine::ECS::System::CameraInputSystem>(ComponentSignature());

            // 2. REGISTRO DOS OUTROS SYSTEMS
            // -- RENDER SYSTEM --
            ComponentSignature renderSignature;
            renderSignature.set(typeManager.getTypeID<Transform>());
            renderSignature.set(typeManager.getTypeID<Mesh>());
            world.addSystem<RenderSystem>(rendererRef);
            world.registerSystemSignature<RenderSystem>(renderSignature);

            // -- PLAYER SYSTEM --
            ComponentSignature playerSignature;
            playerSignature.set(typeManager.getTypeID<Transform>());
            playerSignature.set(typeManager.getTypeID<Movement>());
            playerSignature.set(typeManager.getTypeID<Player>());
            world.addSystem<PlayerSystem>(cameraRef);
            world.registerSystemSignature<PlayerSystem>(playerSignature);

            // 2. -- TERRAIN TRACKING SYSTEM --

            // 1. Ler a altura do raycast do JSON
            float terrainRaycastHeight = config.getValue<float>("world.terrain_raycast_height", 150.0f);

            // -- TERRAIN TRACKING SYSTEM --
            ComponentSignature terrainTrackingSignature;
            terrainTrackingSignature.set(typeManager.getTypeID<Transform>());
            terrainTrackingSignature.set(typeManager.getTypeID<TerrainTracker>());
            // Injeta a altura do raycast no System
            world.addSystem<TerrainTrackingSystem>(terrainRaycastHeight);
            world.registerSystemSignature<TerrainTrackingSystem>(terrainTrackingSignature);

            // --- 3. EXECUÇÃO DE FRAME 0 E SETUP DA CÂMERA (SYNC CRÍTICO) ---

            // 1. Força a correção do Y (TerrainTrackingSystem)
            TerrainTrackingSystem *terrainSystem = world.getSystem<TerrainTrackingSystem>();
            if (terrainSystem)
            {
                terrainSystem->update(world, 0.0f);
            }

            // --- ANIMATION SYSTEM ---
            ComponentSignature animationSignature;
            animationSignature.set(typeManager.getTypeID<Transform>());
            animationSignature.set(typeManager.getTypeID<Movement>());
            animationSignature.set(typeManager.getTypeID<Animation>());

            // Injetamos o AssetManager para que o sistema possa acessar os dados GLTF/Animation
            world.addSystem<AnimationSystem>(assetManager);
            world.registerSystemSignature<AnimationSystem>(animationSignature);

            // 2. APLICAÇÃO DE YAW E TARGET (Configuração da Câmera)
            Component::Transform &currentTransform = world.getComponent<Component::Transform>(playerEntity);
            Component::Movement &playerMovement = world.getComponent<Component::Movement>(playerEntity);

            glm::quat playerQuat = currentTransform.rotation;
            float playerYawDegrees = glm::degrees(glm::yaw(playerQuat));
            glm::vec3 spawnPosGLM = currentTransform.position.toGLM();
            float focusHeight = playerMovement.cameraFocusHeight;
            glm::vec3 initialTargetPoint = spawnPosGLM + glm::vec3(0.0f, focusHeight, 0.0f);

            cameraRef.setYaw(playerYawDegrees);
            cameraRef.setTarget(initialTargetPoint);

            // 3. LIGA OS CALLBACKS GLFW (DEPOIS DE CONFIGURAR TUDO)
            inputService.setupAllCallbacks();

            return true;
        }

    } // namespace Core
} // namespace Engine