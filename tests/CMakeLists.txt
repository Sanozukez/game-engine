# Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.25)

# Test: Struct Sizes Validation (Fase 1.1)
add_executable(test_struct_sizes
    test_struct_sizes.cpp
)

target_include_directories(test_struct_sizes PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link com engine se necessário (não precisa por enquanto, só usa structs)
# target_link_libraries(test_struct_sizes engine)
