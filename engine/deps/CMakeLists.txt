# engine/deps/CMakeLists.txt
# Este é o CMakeLists.txt orquestrador para as dependências de terceiros.
# Ele inclui os CMakeLists.txt de cada subpasta de dependência.

add_subdirectory(cgltf) # NOVO: Inclui o CMakeLists.txt da pasta cgltf
add_subdirectory(stb)   # NOVO: Inclui o CMakeLists.txt da pasta stb

# NÃO adicione `target_include_directories(engine PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)` aqui.
# Cada subpasta vai adicionar seus próprios includes ao target 'engine'.
# Isso evita que a pasta 'deps' em si se torne uma include path, forçando caminhos mais específicos como <cgltf.h>.