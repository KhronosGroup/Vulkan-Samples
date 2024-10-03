/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <core/hpp_device.h>

#include <common/hpp_error.h>
#include <core/hpp_command_pool.h>

namespace vkb
{
namespace core
{
HPPDevice::HPPDevice(vkb::core::HPPPhysicalDevice               &gpu,
                     vk::SurfaceKHR                              surface,
                     std::unique_ptr<vkb::core::HPPDebugUtils> &&debug_utils,
                     std::unordered_map<const char *, bool>      requested_extensions) :
    VulkanResource{nullptr, this},        // Recursive, but valid
    debug_utils{std::move(debug_utils)},
    gpu{gpu},
    resource_cache{*this}
{
	LOGI("Selected GPU: {}", gpu.get_properties().deviceName.data());

	// Prepare the device queues
	std::vector<vk::QueueFamilyProperties> queue_family_properties = gpu.get_queue_family_properties();
	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos(queue_family_properties.size());
	std::vector<std::vector<float>>        queue_priorities(queue_family_properties.size());

	for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index)
	{
		vk::QueueFamilyProperties const &queue_family_property = queue_family_properties[queue_family_index];

		if (gpu.has_high_priority_graphics_queue())
		{
			uint32_t graphics_queue_family = get_queue_family_index(vk::QueueFlagBits::eGraphics);
			if (graphics_queue_family == queue_family_index)
			{
				queue_priorities[queue_family_index].reserve(queue_family_property.queueCount);
				queue_priorities[queue_family_index].push_back(1.0f);
				for (uint32_t i = 1; i < queue_family_property.queueCount; i++)
				{
					queue_priorities[queue_family_index].push_back(0.5f);
				}
			}
			else
			{
				queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);
			}
		}
		else
		{
			queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);
		}

		vk::DeviceQueueCreateInfo &queue_create_info = queue_create_infos[queue_family_index];

		queue_create_info.queueFamilyIndex = queue_family_index;
		queue_create_info.queueCount       = queue_family_property.queueCount;
		queue_create_info.pQueuePriorities = queue_priorities[queue_family_index].data();
	}

	// Check extensions to enable Vma Dedicated Allocation
	bool can_get_memory_requirements = is_extension_supported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	bool has_dedicated_allocation    = is_extension_supported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

	if (can_get_memory_requirements && has_dedicated_allocation)
	{
		enabled_extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
		enabled_extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

		LOGI("Dedicated Allocation enabled");
	}

	// For performance queries, we also use host query reset since queryPool resets cannot
	// live in the same command buffer as beginQuery
	if (is_extension_supported(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME) && is_extension_supported(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME))
	{
		auto perf_counter_features     = gpu.get_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>();
		auto host_query_reset_features = gpu.get_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>();

		if (perf_counter_features.performanceCounterQueryPools && host_query_reset_features.hostQueryReset)
		{
			gpu.add_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>().performanceCounterQueryPools = true;
			gpu.add_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>().hostQueryReset                    = true;
			enabled_extensions.push_back(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME);
			enabled_extensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
			LOGI("Performance query enabled");
		}
	}

	// Check that extensions are supported before trying to create the device
	std::vector<const char *> unsupported_extensions{};
	for (auto &extension : requested_extensions)
	{
		if (is_extension_supported(extension.first))
		{
			enabled_extensions.emplace_back(extension.first);
		}
		else
		{
			unsupported_extensions.emplace_back(extension.first);
		}
	}

	if (enabled_extensions.size() > 0)
	{
		LOGI("HPPDevice supports the following requested extensions:");
		for (auto &extension : enabled_extensions)
		{
			LOGI("  \t{}", extension);
		}
	}

	if (unsupported_extensions.size() > 0)
	{
		auto error = false;
		for (auto &extension : unsupported_extensions)
		{
			auto extension_is_optional = requested_extensions[extension];
			if (extension_is_optional)
			{
				LOGW("Optional device extension {} not available, some features may be disabled", extension);
			}
			else
			{
				LOGE("Required device extension {} not available, cannot run", extension);
				error = true;
			}
		}

		if (error)
		{
			throw vkb::common::HPPVulkanException(vk::Result::eErrorExtensionNotPresent, "Extensions not present");
		}
	}

	vk::DeviceCreateInfo create_info({}, queue_create_infos, {}, enabled_extensions, &gpu.get_mutable_requested_features());

	// Latest requested feature will have the pNext's all set up for device creation.
	create_info.pNext = gpu.get_extension_feature_chain();

	set_handle(gpu.get_handle().createDevice(create_info));

	queues.resize(queue_family_properties.size());

	for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index)
	{
		vk::QueueFamilyProperties const &queue_family_property = queue_family_properties[queue_family_index];

		vk::Bool32 present_supported = gpu.get_handle().getSurfaceSupportKHR(queue_family_index, surface);

		for (uint32_t queue_index = 0U; queue_index < queue_family_property.queueCount; ++queue_index)
		{
			queues[queue_family_index].emplace_back(*this, queue_family_index, queue_family_property, present_supported, queue_index);
		}
	}

	vkb::allocated::init(*this);

	command_pool = std::make_unique<vkb::core::HPPCommandPool>(
	    *this, get_queue_by_flags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).get_family_index());
	fence_pool = std::make_unique<vkb::HPPFencePool>(*this);
}

HPPDevice::~HPPDevice()
{
	resource_cache.clear();

	command_pool.reset();
	fence_pool.reset();

	vkb::allocated::shutdown();

	if (get_handle())
	{
		get_handle().destroy();
	}
}

bool HPPDevice::is_extension_supported(std::string const &requested_extension) const
{
	return gpu.is_extension_supported(requested_extension);
}

bool HPPDevice::is_enabled(std::string const &extension) const
{
	return std::find_if(enabled_extensions.begin(),
	                    enabled_extensions.end(),
	                    [extension](const char *enabled_extension) { return extension == enabled_extension; }) != enabled_extensions.end();
}

vkb::core::HPPPhysicalDevice const &HPPDevice::get_gpu() const
{
	return gpu;
}

vkb::core::HPPDebugUtils const &HPPDevice::get_debug_utils() const
{
	return *debug_utils;
}

vkb::core::HPPQueue const &HPPDevice::get_queue(uint32_t queue_family_index, uint32_t queue_index) const
{
	return queues[queue_family_index][queue_index];
}

vkb::core::HPPQueue const &HPPDevice::get_queue_by_flags(vk::QueueFlags required_queue_flags, uint32_t queue_index) const
{
	for (size_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index)
	{
		vkb::core::HPPQueue const &first_queue = queues[queue_family_index][0];

		vk::QueueFlags queue_flags = first_queue.get_properties().queueFlags;
		uint32_t       queue_count = first_queue.get_properties().queueCount;

		if (((queue_flags & required_queue_flags) == required_queue_flags) && queue_index < queue_count)
		{
			return queues[queue_family_index][queue_index];
		}
	}

	throw std::runtime_error("Queue not found");
}

vkb::core::HPPQueue const &HPPDevice::get_queue_by_present(uint32_t queue_index) const
{
	for (uint32_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index)
	{
		vkb::core::HPPQueue const &first_queue = queues[queue_family_index][0];

		uint32_t queue_count = first_queue.get_properties().queueCount;

		if (first_queue.support_present() && queue_index < queue_count)
		{
			return queues[queue_family_index][queue_index];
		}
	}

	throw std::runtime_error("Queue not found");
}

uint32_t HPPDevice::get_queue_family_index(vk::QueueFlagBits queue_flag) const
{
	const auto &queue_family_properties = gpu.get_queue_family_properties();

	// Dedicated queue for compute
	// Try to find a queue family index that supports compute but not graphics
	if (queue_flag & vk::QueueFlagBits::eCompute)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++)
		{
			if ((queue_family_properties[i].queueFlags & queue_flag) && !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics))
			{
				return i;
				break;
			}
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if (queue_flag & vk::QueueFlagBits::eTransfer)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++)
		{
			if ((queue_family_properties[i].queueFlags & queue_flag) && !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
			    !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eCompute))
			{
				return i;
				break;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested
	// flags
	for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++)
	{
		if (queue_family_properties[i].queueFlags & queue_flag)
		{
			return i;
			break;
		}
	}

	throw std::runtime_error("Could not find a matching queue family index");
}

vkb::core::HPPQueue const &HPPDevice::get_suitable_graphics_queue() const
{
	for (size_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index)
	{
		vkb::core::HPPQueue const &first_queue = queues[queue_family_index][0];

		uint32_t queue_count = first_queue.get_properties().queueCount;

		if (first_queue.support_present() && 0 < queue_count)
		{
			return queues[queue_family_index][0];
		}
	}

	return get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
}

std::pair<vk::Image, vk::DeviceMemory> HPPDevice::create_image(vk::Format format, vk::Extent2D const &extent, uint32_t mip_levels, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) const
{
	vk::Device device = get_handle();

	vk::ImageCreateInfo image_create_info({}, vk::ImageType::e2D, format, vk::Extent3D(extent, 1), mip_levels, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage);
	vk::Image           image = device.createImage(image_create_info);

	vk::MemoryRequirements memory_requirements = device.getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo memory_allocation(memory_requirements.size, get_gpu().get_memory_type(memory_requirements.memoryTypeBits, properties));
	vk::DeviceMemory       memory = device.allocateMemory(memory_allocation);
	device.bindImageMemory(image, memory, 0);

	return std::make_pair(image, memory);
}

void HPPDevice::copy_buffer(vkb::core::BufferCpp &src, vkb::core::BufferCpp &dst, vk::Queue queue, vk::BufferCopy *copy_region) const
{
	assert(dst.get_size() <= src.get_size());
	assert(src.get_handle());

	vk::CommandBuffer command_buffer = create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

	vk::BufferCopy buffer_copy{};
	if (copy_region)
	{
		buffer_copy = *copy_region;
	}
	else
	{
		buffer_copy.size = src.get_size();
	}

	command_buffer.copyBuffer(src.get_handle(), dst.get_handle(), buffer_copy);

	flush_command_buffer(command_buffer, queue);
}

vk::CommandBuffer HPPDevice::create_command_buffer(vk::CommandBufferLevel level, bool begin) const
{
	assert(command_pool && "No command pool exists in the device");

	vk::CommandBuffer command_buffer = get_handle().allocateCommandBuffers({command_pool->get_handle(), level, 1}).front();

	// If requested, also start recording for the new command buffer
	if (begin)
	{
		command_buffer.begin(vk::CommandBufferBeginInfo());
	}

	return command_buffer;
}

void HPPDevice::flush_command_buffer(vk::CommandBuffer command_buffer, vk::Queue queue, bool free, vk::Semaphore signalSemaphore) const
{
	if (!command_buffer)
	{
		return;
	}

	command_buffer.end();

	vk::SubmitInfo submit_info({}, {}, command_buffer);
	if (signalSemaphore)
	{
		submit_info.setSignalSemaphores(signalSemaphore);
	}

	// Create fence to ensure that the command buffer has finished executing
	vk::Fence fence = get_handle().createFence({});

	// Submit to the queue
	queue.submit(submit_info, fence);

	// Wait for the fence to signal that command buffer has finished executing
	vk::Result result = get_handle().waitForFences(fence, true, DEFAULT_FENCE_TIMEOUT);
	if (result != vk::Result::eSuccess)
	{
		LOGE("Detected Vulkan error: {}", vkb::to_string(result));
		abort();
	}

	get_handle().destroyFence(fence);

	if (command_pool && free)
	{
		get_handle().freeCommandBuffers(command_pool->get_handle(), command_buffer);
	}
}

vkb::core::HPPCommandPool &HPPDevice::get_command_pool()
{
	return *command_pool;
}

vkb::HPPFencePool &HPPDevice::get_fence_pool()
{
	return *fence_pool;
}

vkb::HPPResourceCache &HPPDevice::get_resource_cache()
{
	return resource_cache;
}
}        // namespace core
}        // namespace vkb
