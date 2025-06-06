// engine/input/input_manager.cpp
#include "input_manager.h"
#include "./../core/log.h"
#include <algorithm>
#include <format>

namespace Engine
{
    namespace Input
    { // **** MUDANÇA AQUI: INCLUIR NAMESPACE INPUT ****

        InputManager &InputManager::Get()
        {
            static InputManager instance;
            return instance;
        }

        void InputManager::RegisterCallback(InputEvent event, int key, const InputCallback &callback)
        {
            callbacks_[event][key].push_back(callback);
        }

        void InputManager::UnregisterCallback(InputEvent event, int key, const InputCallback &callback)
        {
            auto &keyCallbacks = callbacks_[event][key];
            keyCallbacks.erase(std::remove_if(keyCallbacks.begin(), keyCallbacks.end(),
                                              [&](const InputCallback &cb)
                                              {
                                                  return cb.target<void(const InputEventData &)>() == callback.target<void(const InputEventData &)>();
                                              }),
                               keyCallbacks.end());
        }

        bool InputManager::IsKeyPressed(int key) const
        {
            if (key >= 0 && key < 1024)
            {
                return m_keyStates[key];
            }
            return false;
        }

        void InputManager::GetMouseDelta(double &xoffset, double &yoffset)
        {
            xoffset = m_lastMouseX;
            yoffset = m_lastMouseY;
        }

        void InputManager::ProcessInput(GLFWwindow *window)
        {
            glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                               { InputManager::Get().processKeyEvent(key, action); });

            glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods)
                                       { InputManager::Get().processMouseButtonEvent(button, action); });

            glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos)
                                     { InputManager::Get().processMousePositionEvent(xpos, ypos); });

            glfwSetScrollCallback(window, [](GLFWwindow *window, double xoffset, double yoffset)
                                  { InputManager::Get().processMouseScrollEvent(xoffset, yoffset); });
        }

        void InputManager::processKeyEvent(int key, int action)
        {
            if (key >= 0 && key < 1024)
            {
                m_keyStates[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
            }

            InputEvent event = (action == GLFW_PRESS || action == GLFW_REPEAT) ? InputEvent::KeyPressed : InputEvent::KeyReleased;
            InputEventData data = {key, 0.0, 0.0, 0.0, 0.0, m_rightMousePressed};
            if (callbacks_.count(event) && callbacks_[event].count(key))
            {
                for (const auto &callback : callbacks_[event][key])
                {
                    callback(data);
                }
            }
        }

        void InputManager::processMouseButtonEvent(int button, int action)
        {
            if (button == GLFW_MOUSE_BUTTON_RIGHT)
            {
                m_rightMousePressed = (action == GLFW_PRESS);
                if (action == GLFW_PRESS)
                {
                    m_firstMouse = true;
                    Engine::Log::Debug("InputManager: Botão direito do mouse pressionado. Resetando firstMouse.");
                }
            }

            InputEvent event = (action == GLFW_PRESS) ? InputEvent::MouseButtonPressed : InputEvent::MouseButtonReleased;
            InputEventData data = {button, 0.0, 0.0, 0.0, 0.0, m_rightMousePressed};
            if (callbacks_.count(event) && callbacks_[event].count(button))
            {
                for (const auto &callback : callbacks_[event][button])
                {
                    callback(data);
                }
            }
        }

        void InputManager::processMousePositionEvent(double xpos, double ypos)
        {
            m_lastMouseX = xpos;
            m_lastMouseY = ypos;

            InputEventData data = {0, xpos, ypos, 0.0, 0.0, m_rightMousePressed};
            if (callbacks_.count(InputEvent::MouseMoved) && callbacks_[InputEvent::MouseMoved].count(0))
            {
                for (const auto &callback : callbacks_[InputEvent::MouseMoved][0])
                {
                    callback(data);
                }
            }
        }

        void InputManager::processMouseScrollEvent(double xoffset, double yoffset)
        {
            InputEventData data = {0, 0.0, 0.0, xoffset, yoffset, m_rightMousePressed};
            if (callbacks_.count(InputEvent::MouseScrolled) && callbacks_[InputEvent::MouseScrolled].count(0))
            {
                for (const auto &callback : callbacks_[InputEvent::MouseScrolled][0])
                {
                    callback(data);
                }
            }
        }

    } // namespace Input
} // namespace Engine