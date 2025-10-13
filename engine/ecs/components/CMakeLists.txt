# // engine/ecs/components/CMakeLists.txt
# Este arquivo garante que os headers de matemática sejam visíveis para o restante da Engine.

# Adiciona o diretório atual (exc/componentes/) como um caminho de inclusão (include path).
# O PUBLIC garante que qualquer projeto que use 'engine' também possa ver os headers de math.
target_include_directories(engine 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)