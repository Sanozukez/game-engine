# src/client/CMakeLists.txt
# Game Client and Editor executables

# ============================================================================
# GAME CLIENT - Play the game
# ============================================================================

add_executable(game-engine
    ../main.cpp
    app/app.cpp
    app/app.h
    app/app_setup.cpp
    app/app_setup.h
    
    # Game-specific Systems (depend on graphics)
    systems/render_system.cpp
    systems/render_system.h
    systems/player_system.cpp
    systems/player_system.h
    systems/camera_system.cpp
    systems/camera_system.h
    systems/camera_input_system.cpp
    systems/camera_input_system.h
    systems/camera_target_system.h
    systems/terrain_tracking_system.cpp
    systems/terrain_tracking_system.h
)

target_include_directories(game-engine PRIVATE
    ${CMAKE_SOURCE_DIR}               # Root: allows engine/..., client/..., src/...
    ${CMAKE_SOURCE_DIR}/engine        # Allows ecs/..., core/..., etc
    ${CMAKE_SOURCE_DIR}/client        # Allows render/..., camera/..., etc
    ${CMAKE_SOURCE_DIR}/src/client    # Allows systems/..., app/...
)

target_link_libraries(game-engine PRIVATE
    engine_client    # Graphics, Input, UI (depends on engine)
    engine           # Core (ECS, Asset, Animation) - MUST BE AFTER client
)

target_compile_features(game-engine PRIVATE cxx_std_20)

# Copy assets to build directory
add_custom_command(TARGET game-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:game-engine>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/config
        $<TARGET_FILE_DIR:game-engine>/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:game-engine>/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/shaders
        $<TARGET_FILE_DIR:game-engine>/shaders
)

# ============================================================================
# EDITOR - Visual editor (Unity/Unreal-like)
# ============================================================================

add_executable(game-engine-editor
    ../editor/editor_main.cpp
    ../editor/editor_app.cpp
    ../editor/editor_ui_panels.cpp
    ../editor/editor_viewport_panel.cpp
    ../editor/editor_side_panels.cpp
    ../editor/editor_app.h
    ../editor/editor_theme.cpp
    ../editor/editor_theme.h
    ../editor/editor_layout.cpp
    ../editor/editor_layout.h
)

target_include_directories(game-engine-editor PRIVATE
    ${CMAKE_SOURCE_DIR}               # Root
    ${CMAKE_SOURCE_DIR}/engine        # Permite ecs/..., core/..., etc
    ${CMAKE_SOURCE_DIR}/client        # Permite render/..., camera/..., etc
    ${CMAKE_SOURCE_DIR}/src/editor    # Permite includes do editor
)

target_link_libraries(game-engine-editor PRIVATE
    engine_client    # Graphics, Input, UI
    engine           # Core (ECS, Asset, Animation)
)

target_compile_features(game-engine-editor PRIVATE cxx_std_20)

# Copy assets to editor build directory
add_custom_command(TARGET game-engine-editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/config
        $<TARGET_FILE_DIR:game-engine-editor>/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:game-engine-editor>/data
    COMMENT "Copiando configurações para o editor..."
)
