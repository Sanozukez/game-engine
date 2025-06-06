# engine/core/CMakeLists.txt
# Este arquivo é responsável por adicionar as fontes e os diretórios de inclusão
# do módulo 'core' ao target principal da engine.

target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/path_utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/path_utils.h
        ${CMAKE_CURRENT_SOURCE_DIR}/log.h
        ${CMAKE_CURRENT_SOURCE_DIR}/config.h # NOVO: Adicionar o arquivo de configuração
)

# Adiciona o diretório 'core' como uma diretiva de inclusão pública para o target 'engine'.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # $<INSTALL_INTERFACE:include/engine/core> 
)