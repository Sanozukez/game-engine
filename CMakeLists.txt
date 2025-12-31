# game-engine/CMakeLists.txt
# Este é o arquivo CMake principal do projeto game-engine.

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.25)
project(game-engine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# **** NOVO: Definir GLM_ENABLE_EXPERIMENTAL globalmente para o projeto ****
# Isso garante que todas as unidades de compilação que usam GLM o vejam.
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

# **** PROFILING: Habilitar profiling em Debug builds ****
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(ENABLE_PROFILING)
    message(STATUS "Profiling ENABLED (Debug build)")
else()
    message(STATUS "Profiling DISABLED (Release build)")
endif()

set(IGG_COMPONENTS "IMGUI;GLFW;GLAD;GLM" CACHE STRING "The components to import and build")
foreach (comp IN ITEMS ${IGG_COMPONENTS})
    set(IMPORT_${comp} ON)
endforeach()

include(cmake/dependencies.cmake)

# ============================================================================
# BUILD ORDER - Dependencies matter!
# ============================================================================

# 1. Core Engine (shared: ECS, Asset, Animation, Physics)
add_subdirectory(engine)

# 2. Client Library (graphics: Render, Input, UI, Camera)
add_subdirectory(client)

# 3. Executables (game client, editor, server)
add_subdirectory(src)

# 4. Tests (optional: struct validation, unit tests)
add_subdirectory(tests)