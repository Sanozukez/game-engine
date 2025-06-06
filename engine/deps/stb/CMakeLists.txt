# engine/deps/stb/CMakeLists.txt
# Gerencia a dependência stb.

# Adiciona o diretório de stb como um diretório de inclusão pública para o target 'engine'.
# Isso permite que a engine inclua #include <stb_image.h>.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)