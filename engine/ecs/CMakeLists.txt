# // engine/ecs/CMakeLists.txt

add_subdirectory(systems) # Onde camera_system e player_system estão

# Adiciona o World Loader diretamente ao target 'engine'
target_sources(engine
    PRIVATE
    world_loader.cpp # <--- ADICIONAR ESTA LINHA
)