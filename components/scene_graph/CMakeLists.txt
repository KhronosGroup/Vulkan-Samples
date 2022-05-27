set(SRC
    src/node_animation.cpp
)

add_library(components__scene_graph STATIC ${SRC})
target_include_directories(components__scene_graph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(components__scene_graph PUBLIC glm EnTT::EnTT)

add_library(Components::SceneGraph ALIAS components__scene_graph)

vkb__register_tests(
    NAME "scene_graph_test"
    SRC
        tests/graph.test.cpp
    LIBS
        components__scene_graph
)