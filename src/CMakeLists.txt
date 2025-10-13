# src/CMakeLists.txt
# Este arquivo define o executável principal do jogo.

# file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.h)

# Define o executável principal do jogo.
add_executable(game-engine 
    main.cpp            # Está em src/
    app/app.cpp         # Está em src/app/
    app/app_setup.cpp  # <--- NOVO ARQUIVO DE SETUP
)

# Adiciona o diretório raiz do projeto (${CMAKE_SOURCE_DIR}) como um diretório de inclusão público
target_include_directories(game-engine PUBLIC ${CMAKE_SOURCE_DIR})

# Linka o executável com a sua biblioteca 'engine'.
target_link_libraries(game-engine PRIVATE engine)
# # Define o executável principal do jogo.
# add_executable(game-engine ${SOURCES})

