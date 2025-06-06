# engine/render/CMakeLists.txt
# Adiciona o subdiretório da câmera
add_subdirectory(camera)

target_sources(engine
  PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/shader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/texture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp # Seu renderer principal
        ${CMAKE_CURRENT_SOURCE_DIR}/opengl_renderer.cpp # Se for uma implementação separada
        # NOVO: Adicione material.cpp aqui
        ${CMAKE_CURRENT_SOURCE_DIR}/material.cpp
  PUBLIC # Headers públicos do módulo Render
        ${CMAKE_CURRENT_SOURCE_DIR}/shader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/texture.h
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/opengl_renderer.h
        # NOVO: Adicione material.h aqui
        ${CMAKE_CURRENT_SOURCE_DIR}/material.h
        ${CMAKE_CURRENT_SOURCE_DIR}/camera/icamera.h # Assumindo que icamera.h está aqui
)

# Adiciona o diretório 'render' como um diretório de inclusão pública para o target 'engine'.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)