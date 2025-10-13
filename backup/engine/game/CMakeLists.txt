# engine/game/CMakeLists.txt
# Gerencia as fontes do módulo de game objects e as adiciona ao target principal 'engine'.

target_sources(engine
    PRIVATE

     
    PUBLIC # Headers públicos do módulo Game

   
)

# Adiciona o diretório 'game' como um diretório de inclusão pública para o target 'engine'.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)