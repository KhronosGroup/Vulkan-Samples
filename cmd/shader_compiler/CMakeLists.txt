project(shader_compiler)

set(SOURCES
    src/main.cpp
)

add_executable(shader_compiler ${SOURCES})
target_link_libraries(
        shader_compiler
    PRIVATE
        vkb__common
        vkb__platform
        CLI11
        nlohmann_json
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