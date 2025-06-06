# engine/render/camera/CMakeLists.txt
# Adiciona as fontes do módulo de câmera ao target principal 'engine'
target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/free_camera.cpp
        ${CMAKE_CURRENT_LIST_DIR}/orbit_camera.cpp
    PUBLIC # Headers que definem as interfaces das câmeras
        ${CMAKE_CURRENT_LIST_DIR}/icamera.h # NOVO: Incluir a interface da câmera
        ${CMAKE_CURRENT_LIST_DIR}/free_camera.h
        ${CMAKE_CURRENT_LIST_DIR}/orbit_camera.h
)

# Adiciona o diretório 'camera' como um diretório de inclusão pública para o target 'engine'.
# Isso permite que outros módulos incluam headers como #include "engine/render/camera/free_camera.h"
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # $<INSTALL_INTERFACE:include/engine/render/camera> # Para instalações futuras
)