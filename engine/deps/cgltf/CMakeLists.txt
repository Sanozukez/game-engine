# engine/deps/cgltf/CMakeLists.txt
# Gerencia a dependência cgltf.

# Adiciona o diretório de cgltf como um diretório de inclusão pública para o target 'engine'.
# Isso permite que a engine inclua #include <cgltf.h>.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)