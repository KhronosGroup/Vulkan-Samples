/* Copyright (c) 2019-2024, Arm Limited and Contributors
 * Copyright (c) 2019-2024, Sascha Willems
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

#include "device.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace vkb
{
Device::Device(PhysicalDevice                        &gpu,
               VkSurfaceKHR                           surface,
               std::unique_ptr<DebugUtils>          &&debug_utils,
               std::unordered_map<const char *, bool> requested_extensions) :
    VulkanResource{VK_NULL_HANDLE, this},        // Recursive, but valid
    debug_utils{std::move(debug_utils)},
    gpu{gpu},
    resource_cache{*this}
{
	LOGI("Selected GPU: {}", gpu.get_properties().deviceName);

	// Prepare the device queues
	uint32_t                             queue_family_properties_count = to_u32(gpu.get_queue_family_properties().size());
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_family_properties_count, {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO});
	std::vector<std::vector<float>>      queue_priorities(queue_family_properties_count);

	for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties_count; ++queue_family_index)
	{
		const VkQueueFamilyProperties &queue_family_property = gpu.get_queue_family_properties()[queue_family_index];

		if (gpu.has_high_priority_graphics_queue())
		{
			uint32_t graphics_queue_family = get_queue_family_index(VK_QUEUE_GRAPHICS_BIT);
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

		VkDeviceQueueCreateInfo &queue_create_info = queue_create_infos[queue_family_index];

		queue_create_info.queueFamilyIndex = queue_family_index;
		queue_create_info.queueCount       = queue_family_property.queueCount;
		queue_create_info.pQueuePriorities = queue_priorities[queue_family_index].data();
	}

	// Check extensions to enable Vma Dedicated Allocation
	bool can_get_memory_requirements = is_extension_supported("VK_KHR_get_memory_requirements2");
	bool has_dedicated_allocation    = is_extension_supported("VK_KHR_dedicated_allocation");

	if (can_get_memory_requirements && has_dedicated_allocation)
	{
		enabled_extensions.push_back("VK_KHR_get_memory_requirements2");
		enabled_extensions.push_back("VK_KHR_dedicated_allocation");

		LOGI("Dedicated Allocation enabled");
	}

	// For performance queries, we also use host query reset since queryPool resets cannot
	// live in the same command buffer as beginQuery
	if (is_extension_supported("VK_KHR_performance_query") &&
	    is_extension_supported("VK_EXT_host_query_reset"))
	{
		auto perf_counter_features     = gpu.request_extension_features<VkPhysicalDevicePerformanceQueryFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR);
		auto host_query_reset_features = gpu.request_extension_features<VkPhysicalDeviceHostQueryResetFeatures>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES);

		if (perf_counter_features.performanceCounterQueryPools && host_query_reset_features.hostQueryReset)
		{
			enabled_extensions.push_back("VK_KHR_performance_query");
			enabled_extensions.push_back("VK_EXT_host_query_reset");
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
		LOGI("Device supports the following requested extensions:");
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
			throw VulkanException(VK_ERROR_EXTENSION_NOT_PRESENT, "Extensions not present");
		}
	}

	VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

	// Latest requested feature will have the pNext's all set up for device creation.
	create_info.pNext = gpu.get_extension_feature_chain();

	create_info.pQueueCreateInfos       = queue_create_infos.data();
	create_info.queueCreateInfoCount    = to_u32(queue_create_infos.size());
	create_info.enabledExtensionCount   = to_u32(enabled_extensions.size());
	create_info.ppEnabledExtensionNames = enabled_extensions.data();

	const auto requested_gpu_features = gpu.get_requested_features();
	create_info.pEnabledFeatures      = &requested_gpu_features;

	VkResult result = vkCreateDevice(gpu.get_handle(), &create_info, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create device"};
	}

	queues.resize(queue_family_properties_count);

	for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties_count; ++queue_family_index)
	{
		const VkQueueFamilyProperties &queue_family_property = gpu.get_queue_family_properties()[queue_family_index];

		VkBool32 present_supported = gpu.is_present_supported(surface, queue_family_index);

		for (uint32_t queue_index = 0U; queue_index < queue_family_property.queueCount; ++queue_index)
		{
			queues[queue_family_index].emplace_back(*this, queue_family_index, queue_family_property, present_supported, queue_index);
		}
	}

	prepare_memory_allocator();

	command_pool = std::make_unique<CommandPool>(*this, get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0).get_family_index());
	fence_pool   = std::make_unique<FencePool>(*this);
}

Device::Device(PhysicalDevice &gpu, VkDevice &vulkan_device, VkSurfaceKHR surface) :
    gpu{gpu},
    resource_cache{*this}
{
	this->handle = vulkan_device;
	debug_utils  = std::make_unique<DummyDebugUtils>();
}

Device::~Device()
{
	resource_cache.clear();

	command_pool.reset();
	fence_pool.reset();

	vkb::allocated::shutdown();

	if (handle != VK_NULL_HANDLE)
	{
		vkDestroyDevice(handle, nullptr);
	}
}

bool Device::is_extension_supported(const std::string &requested_extension) const
{
	return gpu.is_extension_supported(requested_extension);
}

bool Device::is_enabled(const char *extension) const
{
	return std::find_if(enabled_extensions.begin(), enabled_extensions.end(), [extension](const char *enabled_extension) { return strcmp(extension, enabled_extension) == 0; }) != enabled_extensions.end();
}

const PhysicalDevice &Device::get_gpu() const
{
	return gpu;
}

DriverVersion Device::get_driver_version() const
{
	DriverVersion version;

	switch (gpu.get_properties().vendorID)
	{
		case 0x10DE:
		{
			// Nvidia
			version.major = (gpu.get_properties().driverVersion >> 22) & 0x3ff;
			version.minor = (gpu.get_properties().driverVersion >> 14) & 0x0ff;
			version.patch = (gpu.get_properties().driverVersion >> 6) & 0x0ff;
			// Ignoring optional tertiary info in lower 6 bits
			break;
		}
		default:
		{
			version.major = VK_VERSION_MAJOR(gpu.get_properties().driverVersion);
			version.minor = VK_VERSION_MINOR(gpu.get_properties().driverVersion);
			version.patch = VK_VERSION_PATCH(gpu.get_properties().driverVersion);
		}
	}

	return version;
}

bool Device::is_image_format_supported(VkFormat format) const
{
	VkImageFormatProperties format_properties;

	auto result = vkGetPhysicalDeviceImageFormatProperties(gpu.get_handle(),
	                                                       format,
	                                                       VK_IMAGE_TYPE_2D,
	                                                       VK_IMAGE_TILING_OPTIMAL,
	                                                       VK_IMAGE_USAGE_SAMPLED_BIT,
	                                                       0,        // no create flags
	                                                       &format_properties);
	return result != VK_ERROR_FORMAT_NOT_SUPPORTED;
}

uint32_t Device::get_memory_type(uint32_t bits, VkMemoryPropertyFlags properties, VkBool32 *memory_type_found) const
{
	for (uint32_t i = 0; i < gpu.get_memory_properties().memoryTypeCount; i++)
	{
		if ((bits & 1) == 1)
		{
			if ((gpu.get_memory_properties().memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memory_type_found)
				{
					*memory_type_found = true;
				}
				return i;
			}
		}
		bits >>= 1;
	}

	if (memory_type_found)
	{
		*memory_type_found = false;
		return 0;
	}
	else
	{
		throw std::runtime_error("Could not find a matching memory type");
	}
}

const Queue &Device::get_queue(uint32_t queue_family_index, uint32_t queue_index)
{
	return queues[queue_family_index][queue_index];
}

const Queue &Device::get_queue_by_flags(VkQueueFlags required_queue_flags, uint32_t queue_index) const
{
	for (uint32_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index)
	{
		Queue const &first_queue = queues[queue_family_index][0];

		VkQueueFlags queue_flags = first_queue.get_properties().queueFlags;
		uint32_t     queue_count = first_queue.get_properties().queueCount;

		if (((queue_flags & required_queue_flags) == required_queue_flags) && queue_index < queue_count)
		{
			return queues[queue_family_index][queue_index];
		}
	}

	throw std::runtime_error("Queue not found");
}

const Queue &Device::get_queue_by_present(uint32_t queue_index) const
{
	for (uint32_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index)
	{
		Queue const &first_queue = queues[queue_family_index][0];

		uint32_t queue_count = first_queue.get_properties().queueCount;

		if (first_queue.support_present() && queue_index < queue_count)
		{
			return queues[queue_family_index][queue_index];
		}
	}

	throw std::runtime_error("Queue not found");
}

void Device::add_queue(size_t global_index, uint32_t family_index, VkQueueFamilyProperties properties, VkBool32 can_present)
{
	if (queues.size() < global_index + 1)
	{
		queues.resize(global_index + 1);
	}
	queues[global_index].emplace_back(*this, family_index, properties, can_present, 0);
}

uint32_t Device::get_num_queues_for_queue_family(uint32_t queue_family_index)
{
	const auto &queue_family_properties = gpu.get_queue_family_properties();
	return queue_family_properties[queue_family_index].queueCount;
}

uint32_t Device::get_queue_family_index(VkQueueFlagBits queue_flag)
{
	const auto &queue_family_properties = gpu.get_queue_family_properties();

	// Dedicated queue for compute
	// Try to find a queue family index that supports compute but not graphics
	if (queue_flag & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++)
		{
			if ((queue_family_properties[i].queueFlags & queue_flag) && !(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				return i;
				break;
			}
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if (queue_flag & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++)
		{
			if ((queue_family_properties[i].queueFlags & queue_flag) && !(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				return i;
				break;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
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

const Queue &Device::get_suitable_graphics_queue() const
{
	for (uint32_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index)
	{
		Queue const &first_queue = queues[queue_family_index][0];

		uint32_t queue_count = first_queue.get_properties().queueCount;

		if (first_queue.support_present() && 0 < queue_count)
		{
			return queues[queue_family_index][0];
		}
	}

	return get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
}

void Device::copy_buffer(vkb::core::Buffer &src, vkb::core::Buffer &dst, VkQueue queue, VkBufferCopy *copy_region)
{
	assert(dst.get_size() <= src.get_size());
	assert(src.get_handle());

	VkCommandBuffer command_buffer = create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy buffer_copy{};
	if (copy_region == nullptr)
	{
		buffer_copy.size = src.get_size();
	}
	else
	{
		buffer_copy = *copy_region;
	}

	vkCmdCopyBuffer(command_buffer, src.get_handle(), dst.get_handle(), 1, &buffer_copy);

	flush_command_buffer(command_buffer, queue);
}

VkCommandPool Device::create_command_pool(uint32_t queue_index, VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo command_pool_info = {};
	command_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.queueFamilyIndex        = queue_index;
	command_pool_info.flags                   = flags;
	VkCommandPool command_pool;
	VK_CHECK(vkCreateCommandPool(handle, &command_pool_info, nullptr, &command_pool));
	return command_pool;
}

VkCommandBuffer Device::create_command_buffer(VkCommandBufferLevel level, bool begin) const
{
	assert(command_pool && "No command pool exists in the device");

	VkCommandBufferAllocateInfo cmd_buf_allocate_info{};
	cmd_buf_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buf_allocate_info.commandPool        = command_pool->get_handle();
	cmd_buf_allocate_info.level              = level;
	cmd_buf_allocate_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	VK_CHECK(vkAllocateCommandBuffers(handle, &cmd_buf_allocate_info, &command_buffer));

	// If requested, also start recording for the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo command_buffer_info{};
		command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_info));
	}

	return command_buffer;
}

void Device::flush_command_buffer(VkCommandBuffer command_buffer, VkQueue queue, bool free, VkSemaphore signalSemaphore) const
{
	if (command_buffer == VK_NULL_HANDLE)
	{
		return;
	}

	VK_CHECK(vkEndCommandBuffer(command_buffer));

	VkSubmitInfo submit_info{};
	submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &command_buffer;
	if (signalSemaphore)
	{
		submit_info.pSignalSemaphores    = &signalSemaphore;
		submit_info.signalSemaphoreCount = 1;
	}

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FLAGS_NONE;

	VkFence fence;
	VK_CHECK(vkCreateFence(handle, &fence_info, nullptr, &fence));

	// Submit to the queue
	VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence);
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK(vkWaitForFences(handle, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(handle, fence, nullptr);

	if (command_pool && free)
	{
		vkFreeCommandBuffers(handle, command_pool->get_handle(), 1, &command_buffer);
	}
}

CommandPool &Device::get_command_pool() const
{
	return *command_pool;
}

FencePool &Device::get_fence_pool() const
{
	return *fence_pool;
}

void Device::create_internal_fence_pool()
{
	fence_pool = std::make_unique<FencePool>(*this);
}

void Device::create_internal_command_pool()
{
	command_pool = std::make_unique<CommandPool>(*this, get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0).get_family_index());
}

void Device::prepare_memory_allocator()
{
	vkb::allocated::init(*this);
}

CommandBuffer &Device::request_command_buffer() const
{
	return command_pool->request_command_buffer();
}

VkFence Device::request_fence() const
{
	return fence_pool->request_fence();
}

VkResult Device::wait_idle() const
{
	return vkDeviceWaitIdle(handle);
}

ResourceCache &Device::get_resource_cache()
{
	return resource_cache;
}
}        // namespace vkb
