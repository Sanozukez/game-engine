# // tools/dictionary_compiler/CMakeLists.txt (Final Corrigido)

cmake_minimum_required(VERSION 3.10)
project(dictionary_compiler CXX)

set(PROJECT_ROOT_PATH "../../") 

# =========================================================================
# 1. DEFINIÇÃO DO EXECUTÁVEL
# =========================================================================

# O executável compila APENAS o seu próprio arquivo principal.
# A lista de MMAP_SOURCES NÃO é usada aqui.
add_executable(dictionary_compiler dictionary_compiler.cpp) 

# FORÇA o compilador a usar o padrão C++20, necessário para std::format
target_compile_features(dictionary_compiler PRIVATE cxx_std_20)

# 2. INCLUDES (Headers)
include_directories(
    # 2.1. Headers do CORE para path_utils
    ${PROJECT_ROOT_PATH}engine/core 
    
    # 2.2. Headers Compartilhados (SceneFileFormat.h)
    ${PROJECT_ROOT_PATH}shared/mmap_format
    
    # 2.3. JSON (Nlohmann/json)
    ${PROJECT_ROOT_PATH}build/_deps/json-src/include
)

# 3. DEPENDÊNCIAS DE LINKAGEM (Link com o Engine Core)
# O dictionary_compiler precisa do código compilado do path_utils.cpp.
target_link_libraries(dictionary_compiler 
    # NOVO CAMINHO FÍSICO CORRETO: Sobe 3 níveis para a raiz, desce para a build, e remove '.lib'
    ../../../build/engine/Debug/engine
)

# 4. FLAGS E OTIMIZAÇÕES
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

if(MSVC)
    target_compile_options(dictionary_compiler PRIVATE /W3 /Zi /EHsc)
else()
    target_compile_options(dictionary_compiler PRIVATE -Wall -g)
endif()