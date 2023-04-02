project(shader_compiler)

set(SOURCES
    src/shader_descriptor.cpp
    src/shader_compiler.cpp
    src/header_generator.cpp
)

add_library(shader_compiler_lib STATIC ${SOURCES})
target_include_directories(
    shader_compiler_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(
    shader_compiler_lib
    PUBLIC
        vkb__common
        vkb__platform
        vkb__spirv_reflect
        vkb__glslang
        CLI11
        nlohmann_json
        re2::re2
)

add_executable(shader_compiler WIN32 src/main.cpp)
target_link_libraries(
        shader_compiler
    PUBLIC
        shader_compiler_lib
)

set_target_properties(
    shader_compiler
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)