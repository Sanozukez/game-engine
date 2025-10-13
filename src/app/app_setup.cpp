// // engine/core/app_setup.cpp

#include "app_setup.h"
#include "./../../core/log.h"
#include "./../../ecs/systems/player_system.h"
#include "./../../ecs/systems/terrain_tracking_system.h"
#include "./../../ecs/systems/render_system.h"
#include "./../../ecs/world_loader.h" // Incluir o WorldLoader diretamente
#include "./../../input/input_service.h"
#include "./../../ecs/components/all_components.h" // Incluir todos os componentes
#include "./../../camera/orbit_camera.h"           // <--- INCLUSÃO CRÍTICA (OrbitCamera)

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
            Engine::ConfigManager &config, // Corrigido namespace Core:: para Engine::
            Engine::Render::Renderer &rendererRef,
            Engine::Camera::ICamera &cameraRef,
            GLFWwindow *glfwWindow)
        {
            // NOVO: Ponteiro temporário para acessar métodos específicos da OrbitCamera.
            Engine::Camera::OrbitCamera *orbitCameraPtr = nullptr;

            // Tenta obter o OrbitCamera* com cast seguro.
            // (O erro getType foi resolvido, pois vamos fazer o dynamic_cast direto)
            orbitCameraPtr = dynamic_cast<Engine::Camera::OrbitCamera *>(&cameraRef);

            // Lógica para carregar limites da OrbitCamera
            if (orbitCameraPtr)
            {
                // Leitura dos limites da câmera
                float minDist = config.getValue<float>("camera.orbit.min_distance", 3.0f);
                float maxDist = config.getValue<float>("camera.orbit.max_distance", 13.0f);
                float minPitch = config.getValue<float>("camera.orbit.min_pitch_degrees", -55.0f);
                float maxPitch = config.getValue<float>("camera.orbit.max_pitch_degrees", 15.0f);

                orbitCameraPtr->setDistanceLimits(minDist, maxDist);
                orbitCameraPtr->setPitchLimits(minPitch, maxPitch);
                Log::Info(std::format("[AppSetup] Limites da OrbitCamera configurados."));
            }

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

        // -------------------------------------------------------------------------
        // 2. INICIALIZAÇÃO E ORQUESTRAÇÃO DO ECS (Lógica Migrada)
        // -------------------------------------------------------------------------

        bool AppSetup::InitializeECS(
            Engine::ECS::World &world,
            Engine::Camera::ICamera &cameraRef,
            Engine::Render::Renderer &rendererRef,
            GLFWwindow *glfwWindow)
        {
            // A. CRIAÇÃO DAS ENTIDADES ALVO (Do App.cpp)
            Engine::ECS::EntityID playerEntity = world.createEntity();
            const uint32_t PLAYER_ASSET_ID = 614879287;

            // ADIÇÃO DOS COMPONENTES (Mínimos antes do Load)
            world.addMesh(playerEntity, PLAYER_ASSET_ID);
            world.getTransform(playerEntity).position = Engine::Math::Vec3(0.0f, -100.0f, 0.0f);
            world.addComponent<Player>(playerEntity);
            world.addComponent<Movement>(playerEntity);
            world.addComponent<TerrainTracker>(playerEntity);

            // C. CARREGAMENTO DO MUNDO (Do App.cpp - Ponto de Falha de Memória)
            if (!Engine::ECS::WorldLoader::Load(world, playerEntity))
            {
                Log::Critical("[AppSetup] Falha ao carregar World Loader.");
                return false;
            }

            // B. ADIÇÃO DOS SISTEMAS (Para que rodem no Game Loop)
            world.addSystem<PlayerSystem>(cameraRef); // Dependência ICamera para tracking/input
            world.addSystem<TerrainTrackingSystem>();
            world.addSystem<RenderSystem>(rendererRef);

            // === CORREÇÃO DE FRAME 0 (Frame-perfect sync) ===

            Component::Transform &currentTransform = world.getTransform(playerEntity);
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