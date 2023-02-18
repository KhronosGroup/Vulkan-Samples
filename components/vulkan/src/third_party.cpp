
// A singular place to implement third party dependency header only libraries

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#define VOLK_IMPLEMENTATION
#include <volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
VKBP_ENABLE_WARNINGS()