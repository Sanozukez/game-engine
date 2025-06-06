# engine/input/CMakeLists.txt
# Este arquivo é responsável por adicionar as fontes e os diretórios de inclusão
# do módulo 'input' ao target principal da engine.

# Adiciona as fontes do módulo 'input' ao target principal 'engine'.
target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/input_manager.cpp
    PUBLIC # Headers que definem a interface pública deste módulo
        ${CMAKE_CURRENT_SOURCE_DIR}/input_manager.h
)

# Adiciona o diretório 'input' como um diretório de inclusão pública para o target 'engine'.
# Isso permite que outros módulos (como 'app') incluam headers de 'input' usando:
# #include "engine/input/input_manager.h"
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # $<INSTALL_INTERFACE:include/engine/input> # Para instalações futuras do seu motor
)