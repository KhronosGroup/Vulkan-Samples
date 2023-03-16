

vkb__register_gpu_tests(
  NAME "vulkan_context_test"
  LIBS
    vkb__vulkan
  SRC
    gpu/context.test.cpp
)

vkb__register_gpu_tests(
  NAME "vulkan_memory_pool_test"
  LIBS
    vkb__vulkan
  SRC
    gpu/memory_pool.test.cpp
)

vkb__register_gpu_tests(
  NAME "vulkan_command_pool_test"
  LIBS
    vkb__vulkan
  SRC
    gpu/command_pool.test.cpp
)