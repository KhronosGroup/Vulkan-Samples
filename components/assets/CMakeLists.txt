
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

vkb__register_tests(
  NAME "image_loader_tests"
  SRC
    tests/stb.test.cpp
    tests/ktx.test.cpp
  LIBS
    components__images
    components__vfs
)