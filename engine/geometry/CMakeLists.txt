# engine/geometry/CMakeLists.txt
# Adiciona fontes do módulo 'geometry' ao target principal 'engine'
target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/grid.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sphere.cpp
    PUBLIC  # Os headers que o mundo externo (src/app, outros módulos da engine) precisa incluir
        ${CMAKE_CURRENT_SOURCE_DIR}/grid.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sphere.h
)

# Adiciona o diretório 'geometry' como uma diretiva de inclusão pública para o target 'engine'.
# Isso permite que outros módulos incluam headers como #include "engine/geometry/grid.h"
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # $<INSTALL_INTERFACE:include/engine/geometry> # Se você planeja instalar a engine
)