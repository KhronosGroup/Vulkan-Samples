
# Headers
add_library(components__asset_headers INTERFACE)
target_include_directories(components__asset_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(components__asset_headers INTERFACE
    Components::VFS
    Components::Common
    volk)

# stb loader
add_library(components__stb STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/images/stb.cpp)
target_link_libraries(components__stb PUBLIC
    components__asset_headers
    stb)

# ktx loader
add_library(components__ktx STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/images/ktx.cpp)
target_link_libraries(components__ktx PUBLIC
    components__asset_headers
    ktx)

# astc Codec
add_library(components__astc STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/images/astc.cpp)
target_link_libraries(components__astc PUBLIC
    components__asset_headers
    astc)

# images
add_library(components__images INTERFACE)
target_link_libraries(components__images INTERFACE
    components__stb
    components__ktx
    components__astc)

add_library(Components::Images ALIAS components__images)

# glTF Loader
add_library(components__gltf STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/models/gltf.cpp)
target_link_libraries(components__gltf PUBLIC
    components__asset_headers
    components__stb
    components__ktx
    Components::SceneGraph
    tinygltf)

# models
add_library(components__models INTERFACE)
target_link_libraries(components__models INTERFACE
    components__gltf)

add_library(Components::Models ALIAS components__models)


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND (BUILD_TESTING OR VKB_BUILD_TESTS))
    set(TEST_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/gltf.torusknot.test.cpp
    )

    add_executable(tests__assets ${TEST_SRC})
    target_link_libraries(tests__assets PUBLIC
        components__images
        components__models
        Components::VFS
        Catch2::Catch2)

    vkb_register_test(tests__assets)
endif()