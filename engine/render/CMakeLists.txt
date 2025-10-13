# engine/render/CMakeLists.txt
# Adiciona o subdiretório da câmera

target_sources(engine
  PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/shader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/texture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/material.cpp
#   PUBLIC # Headers públicos do módulo Render
#         ${CMAKE_CURRENT_SOURCE_DIR}/shader.h
#         ${CMAKE_CURRENT_SOURCE_DIR}/texture.h
#         ${CMAKE_CURRENT_SOURCE_DIR}/renderer.h
#         ${CMAKE_CURRENT_SOURCE_DIR}/opengl_types.h
#         ${CMAKE_CURRENT_SOURCE_DIR}/material.h
#         ${CMAKE_CURRENT_SOURCE_DIR}/camera/icamera.h # Assumindo que icamera.h está aqui
)

# Adiciona o diretório 'render' como um diretório de inclusão pública para o target 'engine'.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)