# src/CMakeLists.txt
# Este arquivo define o executável principal do jogo.

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.h)

# Exemplo de como remover arquivos da lista, se necessário:
# list(FILTER SOURCES EXCLUDE REGEX ".*/input\\.cpp$")

# Define o executável principal do jogo.
add_executable(game-engine ${SOURCES})

# **** MUDANÇA CRUCIAL AQUI: ****
# Adiciona o diretório raiz do projeto (${CMAKE_SOURCE_DIR}) como um diretório de inclusão público
# para o executável 'game-engine'. Isso permite inclusões como
# #include "engine/render/camera/free_camera.h" a partir de arquivos em 'src/'.
target_include_directories(game-engine PUBLIC ${CMAKE_SOURCE_DIR})


# Linka o executável com a sua biblioteca 'engine'.
# A sua biblioteca 'engine' já está configurada para linkar PUBLICAMENTE
# com as libs de terceiros (glad, glm, imgui, glfw).
# Portanto, o executável 'game-engine' herdará automaticamente as includes e links dessas libs
# através da sua própria 'engine'. Não é necessário listá-las aqui novamente.
target_link_libraries(game-engine PRIVATE engine)