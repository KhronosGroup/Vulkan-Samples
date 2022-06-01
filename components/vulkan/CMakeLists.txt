set(SRC
    src/instance.cpp
)

add_library(components__vulkan STATIC ${SRC})
target_include_directories(components__vulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(components__vulkan PUBLIC volk)

vkb__register_tests(
    NAME "vulkan_tests"
    SRC
        tests/instance.test.cpp
    LIBS
        components__vulkan
)