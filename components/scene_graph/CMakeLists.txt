vkb__register_component(
    NAME scene_graph
    LINK_LIBS
        glm
        EnTT::EnTT
        vkb__common
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    SRC
        src/animations.cpp
        src/cameras.cpp
        src/graph.cpp
)

vkb__register_tests(
    NAME "scene_graph_test"
    SRC
        tests/graph.test.cpp
    LIBS
        vkb__scene_graph
)