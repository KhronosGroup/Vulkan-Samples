#pragma once

#include <functional>
#include <memory>

#include <vulkan/vulkan.hpp>

#include "buffer_pool.hpp"
#include "image_pool.hpp"

namespace vkb
{
class Context
{
  public:
	virtual ~Context() = default;

	vk::Instance       instance;
	vk::Device         device;
	vk::PhysicalDevice physical_device;

	virtual BufferPool *buffer_pool() const = 0;
	virtual ImagePool  *image_pool() const  = 0;

	virtual void one_time_command(vk::QueueFlags submit_queue_type, std::function<void(vk::CommandBuffer)> &&cmd_func, std::function<void()> completion_fun) = 0;
};

using ContextPtr = std::shared_ptr<Context>;

ContextPtr create_context(vk::Instance       instance,
                          vk::Device         device,
                          vk::PhysicalDevice physical_device);
}        // namespace vkb