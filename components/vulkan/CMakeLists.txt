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


add_library(components__vulkan_test_framework STATIC tests/vulkan_test_functions.cpp)
target_include_directories(components__vulkan_test_framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(components__vulkan_test_framework PUBLIC components__vulkan Catch2::Catch2)

# force CXX linkage
set_target_properties(components__vulkan_test_framework PROPERTIES LINKER_LANGUAGE CXX)

vkb__register_tests(
    NAME "vulkan_tests"
    SRC
        tests/instance.test.cpp
    LIBS
        components__vulkan_test_framework
)