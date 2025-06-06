# engine/game/CMakeLists.txt
# Gerencia as fontes do módulo de game objects e as adiciona ao target principal 'engine'.

target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/game_object.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/player_character.cpp # NOVO: Adicionar player_character.cpp
    PUBLIC # Headers públicos do módulo Game
        ${CMAKE_CURRENT_SOURCE_DIR}/game_object.h
        ${CMAKE_CURRENT_SOURCE_DIR}/player_character.h # NOVO: Adicionar player_character.h
)

# Adiciona o diretório 'game' como um diretório de inclusão pública para o target 'engine'.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)