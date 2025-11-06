# engine/animation/CMakeLists.txt
target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/keyframe_sampler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/skeleton_hierarchy.cpp
    
    PUBLIC # Public headers of the Animation module
        ${CMAKE_CURRENT_SOURCE_DIR}/keyframe_sampler.h
        ${CMAKE_CURRENT_SOURCE_DIR}/skeleton_hierarchy.h
)

# Adiciona o diretório 'animation' como um diretório de inclusão pública para o target 'engine'.
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)