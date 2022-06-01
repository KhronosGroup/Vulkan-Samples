set(SRC
    src/instance.cpp
)

add_library(components__vulkan STATIC ${SRC})
target_include_directories(components__vulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(components__vulkan PUBLIC
    components__common
    volk)


add_library(components__vulkan_test_framework STATIC tests/vulkan_test_functions.cpp)
target_include_directories(components__vulkan_test_framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(components__vulkan_test_framework PUBLIC volk Catch2::Catch2)

vkb__register_tests(
    NAME "vulkan_tests"
    SRC
        tests/instance.test.cpp
    LIBS
        components__vulkan
        components__vulkan_test_framework
)