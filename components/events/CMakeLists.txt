set(SRC
  src/input_manager.cpp
)

add_library(components__events STATIC ${SRC})
target_include_directories(components__events PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

vkb__register_tests(
  NAME "events_tests"
  SRC
    tests/channel.test.cpp
    tests/input_manager.test.cpp
  LIBS
    components__events
)