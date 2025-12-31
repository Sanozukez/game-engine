# client/CMakeLists.txt
# Graphics/UI layer - Used by both Game Client and Editor

cmake_minimum_required(VERSION 3.25)

# ============================================================================
# CLIENT LIBRARY - Graphics, Input, UI (shared by game and editor)
# ============================================================================

add_library(engine_client STATIC
    # Window Management
    window/window.cpp
    window/window.h
    
    # Input System
    input/input_manager.cpp
    input/input_manager.h
    input/input_service.cpp
    input/input_service.h
    input/i_keyboard_listener.h
    input/i_mouse_listener.h
    input/i_scroll_listener.h
    input/i_mouse_button_listener.h
    
    # Camera System
    camera/icamera.h
    camera/camera.h
    camera/camera.cpp
    camera/free_camera.cpp
    camera/free_camera.h
    camera/orbit_camera.cpp
    camera/orbit_camera.h
    camera/camera_math.h
    
    # Rendering
    render/renderer.cpp
    render/renderer.h
    render/shader.cpp
    render/shader.h
    render/shader_manager.cpp
    render/shader_manager.h
    render/texture.cpp
    render/texture.h
    render/material.cpp
    render/material.h
    render/opengl_types.h
    render/grid_renderer.cpp
    render/grid_renderer.h
    render/armature_renderer.cpp
    render/armature_renderer.h
    render/safety_guards.h
    
    # UI Layer (ImGui)
    ui/imgui_layer.cpp
    ui/imgui_layer.h
)

target_include_directories(engine_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/engine
)

target_link_libraries(engine_client PUBLIC
    engine           # Core engine (ECS, Asset, Animation, etc)
    glfw
    glad
    glm::glm
    imgui
)

# Compiler settings
target_compile_features(engine_client PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(engine_client PRIVATE /W4)
else()
    target_compile_options(engine_client PRIVATE -Wall -Wextra -Wpedantic)
endif()
