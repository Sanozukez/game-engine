# // engine/math/CMakeLists.txt
# Este arquivo garante que os headers de matemática sejam visíveis para o restante da Engine.

# Adiciona o diretório atual (engine/math/) como um caminho de inclusão (include path).
# O PUBLIC garante que qualquer projeto que use 'engine' também possa ver os headers de math.
target_include_directories(engine 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# NOVO: Adiciona a implementação .cpp e os headers relevantes ao target da biblioteca 'engine'
target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/quat.cpp # O novo arquivo de implementação

    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/quat.h
        ${CMAKE_CURRENT_SOURCE_DIR}/vec3.h
        ${CMAKE_CURRENT_SOURCE_DIR}/transform_utils.h
        ${CMAKE_CURRENT_SOURCE_DIR}/vec3_json.h
)