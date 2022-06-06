add_library(components__events INTERFACE)
target_include_directories(components__events INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

vkb__register_tests(
  NAME "events_tests"
  SRC
    tests/channel.test.cpp
  LIBS
    components__events
)