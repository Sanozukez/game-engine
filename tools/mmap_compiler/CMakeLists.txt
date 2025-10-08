# // game-engine/tools/mmap_compiler/CMakeLists.txt (Versão Final e Corrigida)

cmake_minimum_required(VERSION 3.10)
project(mmap_compiler C CXX)

set(PROJECT_ROOT_PATH "../../") 

# =========================================================================
# 1. DEFINIÇÃO DOS ARQUIVOS DE CÓDIGO FONTE (CORRETO)
# =========================================================================

set(MMAP_SOURCES
    mmap_compiler.cpp
    asset_processor.cpp
    array_processor.cpp
    spawn_processor.cpp
    mmap_writer.cpp
)

# 2. CONFIGURAÇÃO DE INCLUDES (Ajustado)
# Adicionar a pasta atual para includes locais e APIs do compilador.
include_directories(. 
    ${PROJECT_ROOT_PATH}shared/mmap_format
    ${PROJECT_ROOT_PATH}engine/deps/cgltf
    ${PROJECT_ROOT_PATH}build/_deps/json-src/include
)

# 3. DEFINIÇÃO DO EXECUTÁVEL
add_executable(mmap_compiler ${MMAP_SOURCES})

# 4. DEPENDÊNCIAS DE LINKAGEM
# Não linkamos o cgltf.a pois usamos a implementação header-only.
# Não linkamos o nlohmann/json pois ele é header-only.
# Mas precisamos que o CMake saiba que essas bibliotecas existem e devem ser resolvidas.

# (REMOVIDO: target_link_libraries, pois o executável é autocontido e não precisa de linkage externo)

# 5. FLAGS E OTIMIZAÇÕES

# Desativa warnings de funções inseguras do CRT do MSVC
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Define as opções de compilação
if(MSVC)
    # Garante que as opções CXX e C sejam aplicadas para todos os arquivos .cpp
    target_compile_options(mmap_compiler PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W3 /Zi>)
    target_compile_options(mmap_compiler PRIVATE $<$<COMPILE_LANGUAGE:C>:/W3 /Zi>)
else()
    target_compile_options(mmap_compiler PRIVATE -Wall -g)
endif()