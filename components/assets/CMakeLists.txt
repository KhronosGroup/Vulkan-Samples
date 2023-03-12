
# Image Loader Headers
vkb__register_component(
    NAME
        image_loader_headers
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
        vkb__image_loader_headers
        stb
)

# ktx loader
vkb__register_component(
    NAME
        ktx_loader
    SRC
        src/images/ktx.cpp
    LINK_LIBS
        vkb__image_loader_headers
        ktx
)

# astc Codec
vkb__register_component(
    NAME
        astc_loader
    SRC
        src/images/astc.cpp
    LINK_LIBS
        vkb__image_loader_headers
        astc
)

# image loaders interface
vkb__register_component(
    NAME
        image_loaders
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    LINK_LIBS
        vkb__image_loader_headers
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