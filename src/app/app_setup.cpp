// // engine/core/app_setup.cpp

#include "app_setup.h"
#include "./../../core/log.h"
#include "./../../ecs/systems/player_system.h"
#include "./../../ecs/systems/terrain_tracking_system.h"
#include "./../../ecs/systems/render_system.h"
#include "./../../ecs/world_loader.h" // Incluir o WorldLoader diretamente
#include "./../../input/input_service.h"
#include "./../../ecs/components/all_components.h" // Incluir todos os componentes
#include "./../../ecs/components/component_signature.h"
#include "./../../camera/orbit_camera.h" // <--- INCLUSÃO CRÍTICA (OrbitCamera)

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

            // Tenta obter o OrbitCamera* com cast seguro.
            // (O erro getType foi resolvido, pois vamos fazer o dynamic_cast direto)
            // orbitCameraPtr = dynamic_cast<Engine::Camera::OrbitCamera *>(&cameraRef);

            // // Lógica para carregar limites da OrbitCamera
            // if (orbitCameraPtr)
            // {
            //     // Leitura dos limites da câmera
            //     float minDist = config.getValue<float>("camera.orbit.min_distance", 3.0f);
            //     float maxDist = config.getValue<float>("camera.orbit.max_distance", 13.0f);
            //     float minPitch = config.getValue<float>("camera.orbit.min_pitch_degrees", -55.0f);
            //     float maxPitch = config.getValue<float>("camera.orbit.max_pitch_degrees", 15.0f);

            //     orbitCameraPtr->setDistanceLimits(minDist, maxDist);
            //     orbitCameraPtr->setPitchLimits(minPitch, maxPitch);
            //     Log::Info(std::format("[AppSetup] Limites da OrbitCamera configurados."));
            // }
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

            // === 1. REGISTRO DE COMPONENTES E ENTIDADES (INALTERADO) ===
            world.registerComponent<Transform>();
            world.registerComponent<Mesh>();
            world.registerComponent<Player>();
            world.registerComponent<Movement>();
            world.registerComponent<Terrain>();
            world.registerComponent<TerrainTracker>();
            world.registerComponent<CameraTarget>();
            Log::Info("[AppSetup] Todos os Componentes ECS registrados com sucesso.");

            // A. CRIAÇÃO DA ENTIDADE PLAYER (Continua adicionando Transform explicitamente)
            Engine::ECS::EntityID playerEntity = world.createEntity();
            const uint32_t PLAYER_ASSET_ID = 614879287;
            world.addComponent<Transform>(playerEntity);
            world.addComponent<Mesh>(playerEntity, PLAYER_ASSET_ID);
            world.getComponent<Transform>(playerEntity).position = Engine::Math::Vec3(0.0f, -100.0f, 0.0f);
            world.addComponent<Player>(playerEntity);
            world.addComponent<Movement>(playerEntity);
            world.addComponent<TerrainTracker>(playerEntity);

            // C. CARREGAMENTO DO MUNDO (Que dispara addComponent, atualizando signatures)
            if (!Engine::ECS::WorldLoader::Load(world, playerEntity))
            {
                Log::Critical("[AppSetup] Falha ao carregar World Loader.");
                return false;
            }

            // === 2. ADIÇÃO DOS SISTEMAS E REGISTRO DAS ASSINATURAS (CRÍTICO) ===
            // CORREÇÃO: ADICIONAR ESTA DECLARAÇÃO
            Engine::ECS::ComponentTypeManager &typeManager = Engine::ECS::ComponentTypeManager::Get();

            // -- RENDER SYSTEM --
            // Requer: Transform e Mesh
            ComponentSignature renderSignature;
            renderSignature.set(typeManager.getTypeID<Transform>());
            renderSignature.set(typeManager.getTypeID<Mesh>());
            world.addSystem<RenderSystem>(rendererRef);
            world.registerSystemSignature<RenderSystem>(renderSignature); // REGISTRO!

            // -- PLAYER SYSTEM --
            // Requer: Transform, Movement e Player
            ComponentSignature playerSignature;
            playerSignature.set(typeManager.getTypeID<Transform>());
            playerSignature.set(typeManager.getTypeID<Movement>());
            playerSignature.set(typeManager.getTypeID<Player>());
            world.addSystem<PlayerSystem>(cameraRef);
            world.registerSystemSignature<PlayerSystem>(playerSignature); // REGISTRO!

            // -- TERRAIN TRACKING SYSTEM --
            // Requer: Transform e TerrainTracker
            ComponentSignature terrainTrackingSignature;
            terrainTrackingSignature.set(typeManager.getTypeID<Transform>());
            terrainTrackingSignature.set(typeManager.getTypeID<TerrainTracker>());
            world.addSystem<TerrainTrackingSystem>();
            world.registerSystemSignature<TerrainTrackingSystem>(terrainTrackingSignature); // REGISTRO!

            // === CORREÇÃO DE FRAME 0 (Frame-perfect sync) ===

            Component::Transform &currentTransform = world.getComponent<Component::Transform>(playerEntity);
            Component::Movement &playerMovement = world.getComponent<Component::Movement>(playerEntity);

            // 1. Força a correção do Y
            TerrainTrackingSystem *terrainSystem = world.getSystem<TerrainTrackingSystem>();
            if (terrainSystem)
            {
                terrainSystem->update(world, 0.0f);
            }

            // 2. APLICAÇÃO DE YAW E TARGET (Configuração da Câmera)
            glm::quat playerQuat = currentTransform.rotation;
            float playerYawDegrees = glm::degrees(glm::yaw(playerQuat));
            glm::vec3 spawnPosGLM = currentTransform.position.toGLM();
            float focusHeight = playerMovement.cameraFocusHeight;
            glm::vec3 initialTargetPoint = spawnPosGLM + glm::vec3(0.0f, focusHeight, 0.0f);

            cameraRef.setYaw(playerYawDegrees);
            cameraRef.setTarget(initialTargetPoint);
            cameraRef.resetMouseState();

            // REGISTRO DE CALLBACKS BÁSICOS
            Engine::Input::InputService::Init(glfwWindow, world);
            Engine::Input::InputService::Get().registerCallbacks(cameraRef);

            return true;
        }

    } // namespace Core
} // namespace Engine