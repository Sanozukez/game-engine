# engine/window/CMakeLists.txt
# Este arquivo é responsável por adicionar as fontes e os diretórios de inclusão
# do módulo 'window' ao target principal da engine.

target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/window.cpp # Garanta que esta linha esteja presente
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/window.h   # Garanta que esta linha esteja presente
)

target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # $<INSTALL_INTERFACE:include/engine/window> # Para instalações futuras do seu motor
)