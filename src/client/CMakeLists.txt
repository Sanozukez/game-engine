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
)

target_include_directories(game-engine PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/client
)

target_link_libraries(game-engine PRIVATE
    engine           # Core (ECS, Asset, Animation)
    engine_client    # Graphics, Input, UI
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
# EDITOR - Visual editor (Unity/Unreal-like) - FUTURE
# ============================================================================

# Uncomment when ready to implement editor
# add_executable(game-editor
#     editor/editor_main.cpp
#     editor/editor_app.cpp
#     editor/panels/scene_hierarchy_panel.cpp
#     editor/panels/inspector_panel.cpp
#     editor/panels/asset_browser_panel.cpp
#     editor/panels/viewport_panel.cpp
#     editor/tools/gizmo_system.cpp
# )
# 
# target_link_libraries(game-editor PRIVATE
#     engine
#     engine_client
#     engine_editor  # Editor-specific systems
# )
