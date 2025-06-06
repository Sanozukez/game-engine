# engine/asset/CMakeLists.txt
# Gerencia as fontes do módulo de assets e as adiciona ao target principal 'engine'.

target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/model.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/obj_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gltf_loader_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gltf_loader.cpp
    PUBLIC # Public headers of the Asset module
        ${CMAKE_CURRENT_SOURCE_DIR}/model.h
        ${CMAKE_CURRENT_SOURCE_DIR}/obj_loader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/gltf_loader.h
)

# Adiciona o diretório 'asset' como um diretório de inclusão pública para o target 'engine'.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # $<INSTALL_INTERFACE:include/engine/asset> 
)