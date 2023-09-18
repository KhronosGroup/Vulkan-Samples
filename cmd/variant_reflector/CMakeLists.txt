
add_executable(variant_reflector main.cpp)

target_link_libraries(variant_reflector
    PRIVATE
        vkb__core
        vkb__filesystem
        CLI11::CLI11
        nlohmann_json::nlohmann_json
        tinygltf
)