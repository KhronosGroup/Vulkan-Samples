# volk
set(VOLK_DIR "${CMAKE_SOURCE_DIR}/third_party/volk")
set(VOLK_SRC ${VOLK_DIR}/volk.c)

set(SRC
    ${VOLK_SRC}
    src/instance.cpp
)

add_library(components__vulkan STATIC ${SRC})
target_include_directories(components__vulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${VOLK_DIR})
target_link_libraries(components__vulkan PUBLIC
    vulkan
    ${CMAKE_DL_LIBS}
    components__common)

if (VKB_WSI_SELECTION STREQUAL XCB)
    target_include_directories(components__vulkan PUBLIC ${XCB_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL XLIB)
    target_include_directories(components__vulkan PUBLIC ${X11_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL WAYLAND)
    target_include_directories(components__vulkan PUBLIC ${WAYLAND_INCLUDE_DIRS})
endif()



## Vulkan unit tests overrite the Volk vulkan function definitions with custom written ones
## In the custom vulkan function implementations we can add test logic to verify that our code is using Vulkan as expected
## This does not remove the need to out of box test on specific platforms as drivers can contain bugs

## This library must contain all source, libraries and include dirs as components__vulkan. It must be a single compile target to avoid linker errors when compiling on Mac

vkb__register_tests(
    NAME "vulkan_tests"
    SRC
        ${SRC}
        tests/vulkan_test_functions.cpp
        tests/instance.test.cpp
    LINK_LIBS
        vulkan
        ${CMAKE_DL_LIBS}
        components__common
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${VOLK_DIR}
)