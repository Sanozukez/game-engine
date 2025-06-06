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

set(IGG_COMPONENTS "IMGUI;GLFW;GLAD;GLM" CACHE STRING "The components to import and build")
foreach (comp IN ITEMS ${IGG_COMPONENTS})
    set(IMPORT_${comp} ON)
endforeach()

include(cmake/dependencies.cmake)

# Primeiro, adicione o diretório do motor.
# Isso deve vir antes de 'src' porque 'src' linka com 'engine'.
add_subdirectory(engine)

# Depois, adicione o diretório da aplicação/executável.
add_subdirectory(src)