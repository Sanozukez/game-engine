// engine/ecs/systems/camera_input_system.h
#pragma once

#include "base_system.h"
#include "../../ecs/world.h"
#include "../../camera/icamera.h"
#include "../../input/input_manager.h"
#include "../../input/input_service.h" // declara IKeyboardListener/IMouseListener/...
#include "../../input/i_mouse_look.h"  // Engine::Input::MouseLook
#include "../../input/i_keyboard_listener.h"
#include "../../input/i_mouse_listener.h"
#include "../../input/i_scroll_listener.h"
#include "../../input/i_mouse_button_listener.h"


namespace Engine
{
    namespace ECS
    {
        namespace System
        {

            class CameraInputSystem
                : public BaseSystem,
                  public Engine::Input::IKeyboardListener,
                  public Engine::Input::IMouseListener,
                  public Engine::Input::IScrollListener,
                  public Engine::Input::IMouseButtonListener
            {
            public:
                CameraInputSystem(Engine::ECS::World &world,
                                  Engine::Camera::ICamera &cameraRef,
                                  Engine::Input::InputManager &inputManager,
                                  Engine::Input::InputService &inputService);

                // Eventos de input vindos do App/dispatcher (mantidos para compatibilidade)
                void handleKeyInput(int key, int action) override;
                void handleMouseMovement(double xpos, double ypos) override;
                void handleScrollInput(double yOffset) override;
                void handleMouseButtonInput(int button, int action) override;

                // Callbacks exigidos pelo InputService (assinam as interfaces herdadas)

                // Tick por frame
                // void update(Engine::ECS::World &world, float dt);
                void update(ECS::World& world, float dt) override;


            private:
                Engine::ECS::World &m_world;
                Engine::Camera::ICamera &m_camera;
                Engine::Input::InputManager &m_inputManager;
                Engine::Input::InputService &m_inputService;

                // Para logs/diagnóstico
                uint64_t m_orbitSessionId{0};

                // Mouse-look (cursor travado no centro, com “debounce” do primeiro frame)
                Engine::Input::MouseLook m_mouseLook;
            };

        } // namespace System
    } // namespace ECS
} // namespace Engine
