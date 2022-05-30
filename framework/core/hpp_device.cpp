/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include <common/hpp_error.h>
#include <core/hpp_device.h>

namespace vkb
{
namespace core
{
HPPDevice::HPPDevice(vkb::core::HPPPhysicalDevice &              gpu,
                     vk::SurfaceKHR                              surface,
                     std::unique_ptr<vkb::core::HPPDebugUtils> &&debug_utils,
                     std::unordered_map<const char *, bool>      requested_extensions) :
    HPPVulkanResource{nullptr, this},        // Recursive, but valid
    debug_utils{std::move(debug_utils)},
    gpu{gpu},
    resource_cache{*this}
{
	LOGI("Selected GPU: {}", gpu.get_properties().deviceName);

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
	device_extensions = gpu.get_handle().enumerateDeviceExtensionProperties();

	// Display supported extensions
	if (device_extensions.size() > 0)
	{
		LOGD("HPPDevice supports the following extensions:");
		for (auto &extension : device_extensions)
		{
			LOGD("  \t{}", extension.extensionName);
		}
	}

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
		auto perf_counter_features     = gpu.request_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>();
		auto host_query_reset_features = gpu.request_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>();

		if (perf_counter_features.performanceCounterQueryPools && host_query_reset_features.hostQueryReset)
		{
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

	VmaVulkanFunctions vma_vulkan_func{};
	vma_vulkan_func.vkAllocateMemory                    = reinterpret_cast<PFN_vkAllocateMemory>(get_handle().getProcAddr("vkAllocateMemory"));
	vma_vulkan_func.vkBindBufferMemory                  = reinterpret_cast<PFN_vkBindBufferMemory>(get_handle().getProcAddr("vkBindBufferMemory"));
	vma_vulkan_func.vkBindImageMemory                   = reinterpret_cast<PFN_vkBindImageMemory>(get_handle().getProcAddr("vkBindImageMemory"));
	vma_vulkan_func.vkCreateBuffer                      = reinterpret_cast<PFN_vkCreateBuffer>(get_handle().getProcAddr("vkCreateBuffer"));
	vma_vulkan_func.vkCreateImage                       = reinterpret_cast<PFN_vkCreateImage>(get_handle().getProcAddr("vkCreateImage"));
	vma_vulkan_func.vkDestroyBuffer                     = reinterpret_cast<PFN_vkDestroyBuffer>(get_handle().getProcAddr("vkDestroyBuffer"));
	vma_vulkan_func.vkDestroyImage                      = reinterpret_cast<PFN_vkDestroyImage>(get_handle().getProcAddr("vkDestroyImage"));
	vma_vulkan_func.vkFlushMappedMemoryRanges           = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(get_handle().getProcAddr("vkFlushMappedMemoryRanges"));
	vma_vulkan_func.vkFreeMemory                        = reinterpret_cast<PFN_vkFreeMemory>(get_handle().getProcAddr("vkFreeMemory"));
	vma_vulkan_func.vkGetBufferMemoryRequirements       = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(get_handle().getProcAddr("vkGetBufferMemoryRequirements"));
	vma_vulkan_func.vkGetImageMemoryRequirements        = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(get_handle().getProcAddr("vkGetImageMemoryRequirements"));
	vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(get_handle().getProcAddr("vkGetPhysicalDeviceMemoryProperties"));
	vma_vulkan_func.vkGetPhysicalDeviceProperties       = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(get_handle().getProcAddr("vkGetPhysicalDeviceProperties"));
	vma_vulkan_func.vkInvalidateMappedMemoryRanges      = reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(get_handle().getProcAddr("vkInvalidateMappedMemoryRanges"));
	vma_vulkan_func.vkMapMemory                         = reinterpret_cast<PFN_vkMapMemory>(get_handle().getProcAddr("vkMapMemory"));
	vma_vulkan_func.vkUnmapMemory                       = reinterpret_cast<PFN_vkUnmapMemory>(get_handle().getProcAddr("vkUnmapMemory"));
	vma_vulkan_func.vkCmdCopyBuffer                     = reinterpret_cast<PFN_vkCmdCopyBuffer>(get_handle().getProcAddr("vkCmdCopyBuffer"));

	VmaAllocatorCreateInfo allocator_info{};
	allocator_info.physicalDevice = static_cast<VkPhysicalDevice>(gpu.get_handle());
	allocator_info.device         = static_cast<VkDevice>(get_handle());
	allocator_info.instance       = static_cast<VkInstance>(gpu.get_instance().get_handle());

	if (can_get_memory_requirements && has_dedicated_allocation)
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
		vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
		vma_vulkan_func.vkGetImageMemoryRequirements2KHR  = vkGetImageMemoryRequirements2KHR;
	}

	if (is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) && is_enabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
	{
		allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}

	allocator_info.pVulkanFunctions = &vma_vulkan_func;

	VkResult result = vmaCreateAllocator(&allocator_info, &memory_allocator);

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create allocator"};
	}

	command_pool = std::make_unique<vkb::core::HPPCommandPool>(
	    *this, get_queue_by_flags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).get_family_index());
	fence_pool = std::make_unique<vkb::HPPFencePool>(*this);
}

HPPDevice::~HPPDevice()
{
	resource_cache.clear();

	command_pool.reset();
	fence_pool.reset();

	if (memory_allocator != VK_NULL_HANDLE)
	{
		VmaStats stats;
		vmaCalculateStats(memory_allocator, &stats);

		LOGI("Total device memory leaked: {} bytes.", stats.total.usedBytes);

		vmaDestroyAllocator(memory_allocator);
	}

	if (get_handle())
	{
		get_handle().destroy();
	}
}

bool HPPDevice::is_extension_supported(std::string const &requested_extension) const
{
	return std::find_if(device_extensions.begin(),
	                    device_extensions.end(),
	                    [requested_extension](auto &device_extension) { return std::strcmp(device_extension.extensionName, requested_extension.c_str()) == 0; }) != device_extensions.end();
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

VmaAllocator const &HPPDevice::get_memory_allocator() const
{
	return memory_allocator;
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

std::pair<vk::Buffer, vk::DeviceMemory> HPPDevice::create_buffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::DeviceSize size, void *data) const
{
	// Create the buffer handle
	vk::BufferCreateInfo buffer_create_info({}, size, usage, vk::SharingMode::eExclusive);
	vk::Buffer           buffer = get_handle().createBuffer(buffer_create_info);

	// Create the memory backing up the buffer handle
	vk::MemoryRequirements memory_requirements = get_handle().getBufferMemoryRequirements(buffer);
	vk::MemoryAllocateInfo memory_allocation(memory_requirements.size, get_gpu().get_memory_type(memory_requirements.memoryTypeBits, properties));
	vk::DeviceMemory       memory = get_handle().allocateMemory(memory_allocation);

	// If a pointer to the buffer data has been passed, map the buffer and copy over the
	if (data != nullptr)
	{
		void *mapped = get_handle().mapMemory(memory, 0, size);
		memcpy(mapped, data, static_cast<size_t>(size));
		// If host coherency hasn't been requested, do a manual flush to make writes visible
		if (!(properties & vk::MemoryPropertyFlagBits::eHostCoherent))
		{
			vk::MappedMemoryRange mapped_range(memory, 0, size);
			get_handle().flushMappedMemoryRanges(mapped_range);
		}
		get_handle().unmapMemory(memory);
	}

	// Attach the memory to the buffer object
	get_handle().bindBufferMemory(buffer, memory, 0);

	return std::make_pair(buffer, memory);
}

void HPPDevice::copy_buffer(vkb::core::HPPBuffer &src, vkb::core::HPPBuffer &dst, vk::Queue queue, vk::BufferCopy *copy_region) const
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

vkb::core::HPPCommandPool const &HPPDevice::get_command_pool() const
{
	return *command_pool;
}

vkb::HPPFencePool const &HPPDevice::get_fence_pool() const
{
	return *fence_pool;
}

vkb::HPPResourceCache const &HPPDevice::get_resource_cache() const
{
	return resource_cache;
}
}        // namespace core
}        // namespace vkb
