
# Image Loader Headers
vkb__register_component(
    NAME
        asset_headers
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    LINK_LIBS
        vkb__vfs
        vkb__common
        volk
)

# stb loader
vkb__register_component(
    NAME
        stb_loader
    SRC
        src/images/stb.cpp
    LINK_LIBS
        vkb__asset_headers
        stb
)

# ktx loader
vkb__register_component(
    NAME
        ktx_loader
    SRC
        src/images/ktx.cpp
    LINK_LIBS
        vkb__asset_headers
        ktx
)

# astc Codec
vkb__register_component(
    NAME
        astc_loader
    SRC
        src/images/astc.cpp
    LINK_LIBS
        vkb__asset_headers
        astc
)

# image loaders interface
vkb__register_component(
    NAME
        image_loaders
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    LINK_LIBS
        vkb__asset_headers
        vkb__stb_loader
        vkb__ktx_loader
        vkb__astc_loader
)

vkb__register_tests(
  NAME "image_loader_tests"
  SRC
    tests/stb.test.cpp
    tests/ktx.test.cpp
  LIBS
    vkb__image_loaders
    vkb__vfs
)

# gltf
vkb__register_component(
    NAME
        gltf_loader
    SRC
        src/gltf/filesystem.cpp
        src/gltf/model_loader.cpp
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gltf
    LINK_LIBS
        vkb__asset_headers
        vkb__vfs
        vkb__scene_graph
        vkb__common
        tinygltf
)

vkb__register_tests(
  NAME "gltf_loader_tests"
  SRC
    tests/gltf_loader.test.cpp
  LIBS
    vkb__gltf_loader
)