
add_executable(shader_reflector main.cpp)

target_link_libraries(shader_reflector
    PRIVATE
        vkb__core
        vkb__shaders
        vkb__filesystem
        CLI11::CLI11
        nlohmann_json::nlohmann_json
)