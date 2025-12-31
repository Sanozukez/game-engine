# engine/render/CMakeLists.txt
# Adiciona o subdiretório da câmera

target_sources(engine 
  PRIVATE
    ${RENDER_SOURCES}
    renderer.cpp
    shader.cpp
    texture.cpp
    material.cpp
    shader_manager.cpp
    armature_renderer.cpp
)

# Adiciona o diretório 'render' como um diretório de inclusão pública para o target 'engine'.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)