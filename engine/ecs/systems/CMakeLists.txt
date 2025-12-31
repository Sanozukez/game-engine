# // engine/ecs/systems/CMakeLists.txt

# APENAS animation_system fica aqui (core, não-gráfico)
# Outros sistemas movidos para src/client/systems/
target_sources(engine 
    PRIVATE
    animation_system.cpp
)