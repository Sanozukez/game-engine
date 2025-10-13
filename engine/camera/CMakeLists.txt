# engine/camera/CMakeLists.txt
# Adiciona as fontes do módulo de câmera ao target principal 'engine'
target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/free_camera.cpp
        ${CMAKE_CURRENT_LIST_DIR}/orbit_camera.cpp   
)

# Adiciona o diretório 'camera' como um diretório de inclusão pública para o target 'engine'.
# Isso permite que outros módulos incluam headers como #include "engine/camera/free_camera.h"
# Adiciona o diretório 'camera' como um diretório de inclusão pública.
target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR} # Inclui a pasta 'engine/camera/'
)