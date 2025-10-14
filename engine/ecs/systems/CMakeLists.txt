# // engine/ecs/systems/CMakeLists.txt

# Adiciona todos os arquivos .cpp do diretório de sistemas 
# (apenas render_system.cpp por enquanto) 
# à biblioteca mãe 'engine'.
target_sources(engine 
    PRIVATE
    render_system.cpp
    player_system.cpp
    terrain_tracking_system.cpp
    camera_system.cpp
    camera_target_system.cpp
    camera_input_system.cpp
)