# game-engine/tools/mmap_compiler/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(mmap_compiler C CXX)

# O caminho é relativo a partir da pasta 'tools/mmap_compiler'
# Sobe dois níveis (../../) para chegar na raiz 'game-engine'
set(PROJECT_ROOT_PATH "../../")

# 1. Headers Compartilhados (Nossa nova API)
include_directories(${PROJECT_ROOT_PATH}shared/mmap_format)

# 2. Headers do cgltf
# Presume-se que cgltf.h está diretamente em deps/cgltf/
include_directories(${PROJECT_ROOT_PATH}engine/deps/cgltf)

# 3. Headers do nlohmann/json (Herdado do FetchContent na raiz)
# Caminho: ../../build/_deps/json-src/include (Ajuste o caminho se for diferente)
# Assumimos que a pasta de build principal está na raiz (game-engine/build)
include_directories(${PROJECT_ROOT_PATH}build/_deps/json-src/include)

# Desativa warnings de funções inseguras do CRT do MSVC (para libs como cgltf)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Adiciona o arquivo fonte do executável
add_executable(mmap_compiler mmap_compiler.cpp)

# Define as opções de compilação (necessário para o pragma pack(1) funcionar corretamente)
if(MSVC)
    # Define as flags de C++ e C separadamente
    target_compile_options(mmap_compiler PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W3 /Zi>)
    target_compile_options(mmap_compiler PRIVATE $<$<COMPILE_LANGUAGE:C>:/W3 /Zi>)
else()
    # Para Compiladores GCC/Clang (Linux/macOS)
    target_compile_options(mmap_compiler PRIVATE -Wall -g)
endif()