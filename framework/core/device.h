/* Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#pragma once

#include "common/hpp_vk_common.h"
#include "common/vk_common.h"
#include "core/buffer.h"
#include "core/physical_device.h"
#include "hpp_debug.h"
#include "hpp_fence_pool.h"
#include "hpp_queue.h"
#include "hpp_resource_cache.h"
#include "queue.h"
#include <utility>
#include <vulkan/vulkan.hpp>

namespace vkb
{
class FencePool;
class DebugUtils;
class PhysicalDevice;
class ResourceCache;

namespace core
{
template <vkb::BindingType bindingType>
class CommandPool;
using CommandPoolC   = CommandPool<vkb::BindingType::C>;
using CommandPoolCpp = CommandPool<vkb::BindingType::Cpp>;

template <vkb::BindingType bindingType>
class Device
    : public vkb::core::VulkanResource<bindingType, typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Device, VkDevice>::type>
{
  public:
	using Bool32Type                 = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Bool32, VkBool32>::type;
	using BufferCopyType             = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::BufferCopy, VkBufferCopy>::type;
	using CommandBufferLevelType     = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBufferLevel, VkCommandBufferLevel>::type;
	using CommandBufferType          = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandBuffer, VkCommandBuffer>::type;
	using CommandPoolCreateFlagsType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandPoolCreateFlags, VkCommandPoolCreateFlags>::type;
	using CommandPoolType            = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::CommandPool, VkCommandPool>::type;
	using DeviceMemoryType           = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::DeviceMemory, VkDeviceMemory>::type;
	using DeviceType                 = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Device, VkDevice>::type;
	using Extent2DType               = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Extent2D, VkExtent2D>::type;
	using FenceType                  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Fence, VkFence>::type;
	using FormatType                 = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Format, VkFormat>::type;
	using ImageType                  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Image, VkImage>::type;
	using ImageUsageFlagsType        = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ImageUsageFlags, VkImageUsageFlags>::type;
	using MemoryPropertyFlagsType    = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::MemoryPropertyFlags, VkMemoryPropertyFlags>::type;
	using QueueFamilyPropertiesType  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::QueueFamilyProperties, VkQueueFamilyProperties>::type;
	using QueueFlagBitsType          = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::QueueFlagBits, VkQueueFlagBits>::type;
	using QueueFlagsType             = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::QueueFlags, VkQueueFlags>::type;
	using QueueType                  = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Queue, VkQueue>::type;
	using ResultType                 = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Result, VkResult>::type;
	using SemaphoreType              = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::Semaphore, VkSemaphore>::type;
	using SurfaceType                = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::SurfaceKHR, VkSurfaceKHR>::type;

	using DebugUtilsType    = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPDebugUtils, vkb::DebugUtils>::type;
	using FencePoolType     = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::HPPFencePool, vkb::FencePool>::type;
	using CoreQueueType     = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPQueue, vkb::Queue>::type;
	using ResourceCacheType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::HPPResourceCache, vkb::ResourceCache>::type;

  public:
	/**
	 * @brief Device constructor
	 * @param gpu A valid Vulkan physical device and the requested gpu features
	 * @param surface The surface
	 * @param debug_utils The debug utils to be associated to this device
	 * @param requested_extensions (Optional) List of required device extensions and whether support is optional or not
	 * @param request_gpu_features (Optional) A function that will be called to request specific gpu features before device creation
	 */
	Device(PhysicalDevice<bindingType>                                  &gpu,
	       SurfaceType                                                   surface,
	       std::unique_ptr<DebugUtilsType>                             &&debug_utils,
	       std::unordered_map<const char *, bool> const                 &requested_extensions = {},
	       std::function<void(vkb::core::PhysicalDevice<bindingType> &)> request_gpu_features = {});

	/**
	 * @brief Device constructor
	 * @param gpu A valid Vulkan physical device and the requested gpu features
	 * @param vulkan_device A valid Vulkan device
	 * @param surface The surface
	 */
	Device(PhysicalDevice<bindingType> &gpu, DeviceType &vulkan_device, SurfaceType surface);

	Device(const Device &) = delete;
	Device(Device &&)      = delete;
	~Device();

	Device &operator=(const Device &) = delete;
	Device &operator=(Device &&)      = delete;

	void add_queue(size_t global_index, uint32_t family_index, QueueFamilyPropertiesType const &properties, Bool32Type can_present);
	void copy_buffer(
	    vkb::core::Buffer<bindingType> const &src, vkb::core::Buffer<bindingType> &dst, QueueType queue, BufferCopyType const *copy_region = nullptr);
	CommandBufferType                      create_command_buffer(CommandBufferLevelType level, bool begin = false) const;
	CommandPoolType                        create_command_pool(uint32_t queue_index, CommandPoolCreateFlagsType flags = 0);
	std::pair<ImageType, DeviceMemoryType> create_image(
	    FormatType format, Extent2DType const &extent, uint32_t mip_levels, ImageUsageFlagsType usage, MemoryPropertyFlagsType properties) const;
	void                                 create_internal_command_pool();
	void                                 create_internal_fence_pool();
	void                                 flush_command_buffer(CommandBufferType command_buffer, QueueType queue, bool free = true, SemaphoreType signal_semaphore = VK_NULL_HANDLE) const;
	vkb::core::CommandPool<bindingType> &get_command_pool() const;
	DebugUtilsType const                &get_debug_utils() const;
	FencePoolType                       &get_fence_pool() const;
	PhysicalDevice<bindingType> const   &get_gpu() const;
	CoreQueueType const                 &get_queue(uint32_t queue_family_index, uint32_t queue_index) const;
	CoreQueueType const                 &get_queue_by_flags(QueueFlagsType queue_flags, uint32_t queue_index) const;
	CoreQueueType const                 &get_queue_by_present(uint32_t queue_index) const;
	ResourceCacheType                   &get_resource_cache();
	bool                                 is_extension_enabled(const char *extension) const;
	bool                                 is_image_format_supported(FormatType format) const;
	void                                 wait_idle() const;

  private:
	void                                   copy_buffer_impl(vk::Device device, vkb::core::BufferCpp const &src, vkb::core::BufferCpp &dst, vk::Queue queue, vk::BufferCopy const *copy_region);
	vk::CommandBuffer                      create_command_buffer_impl(vk::Device device, vk::CommandBufferLevel level, bool begin) const;
	std::pair<vk::Image, vk::DeviceMemory> create_image_impl(
	    vk::Device device, vk::Format format, vk::Extent2D const &extent, uint32_t mip_levels, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
	    const;
	void flush_command_buffer_impl(
	    vk::Device device, vk::CommandBuffer command_buffer, vk::Queue queue, bool free = true, vk::Semaphore signal_semaphore = nullptr) const;
	vkb::core::HPPQueue const &get_queue_by_flags_impl(vk::QueueFlags queue_flags, uint32_t queue_index) const;
	void                       init(std::unordered_map<const char *, bool> const &requested_extensions, std::function<void(vkb::core::PhysicalDevice<bindingType> &)> request_gpu_features);

  private:
	std::unique_ptr<vkb::core::CommandPoolCpp>    command_pool;
	std::unique_ptr<vkb::core::HPPDebugUtils>     debug_utils;
	std::vector<const char *>                     enabled_extensions{};
	std::unique_ptr<vkb::HPPFencePool>            fence_pool;
	vkb::core::PhysicalDeviceCpp                 &gpu;
	std::vector<std::vector<vkb::core::HPPQueue>> queues;
	vkb::HPPResourceCache                         resource_cache;
	vk::SurfaceKHR                                surface = nullptr;
};

using DeviceC   = Device<vkb::BindingType::C>;
using DeviceCpp = Device<vkb::BindingType::Cpp>;

}        // namespace core
}        // namespace vkb

#include "core/command_pool.h"

namespace vkb
{
namespace core
{

template <>
inline Device<vkb::BindingType::Cpp>::Device(vkb::core::PhysicalDeviceCpp                       &gpu,
                                             vk::SurfaceKHR                                      surface,
                                             std::unique_ptr<vkb::core::HPPDebugUtils>         &&debug_utils,
                                             std::unordered_map<const char *, bool> const       &requested_extensions,
                                             std::function<void(vkb::core::PhysicalDeviceCpp &)> request_gpu_features) :
    vkb::core::VulkanResourceCpp<vk::Device>{nullptr, this}, debug_utils{std::move(debug_utils)}, gpu{gpu}, resource_cache{*this}, surface(surface)
{
	init(requested_extensions, request_gpu_features);
}

template <>
inline Device<vkb::BindingType::C>::Device(vkb::core::PhysicalDeviceC                       &gpu,
                                           VkSurfaceKHR                                      surface,
                                           std::unique_ptr<vkb::DebugUtils>                &&debug_utils,
                                           std::unordered_map<const char *, bool> const     &requested_extensions,
                                           std::function<void(vkb::core::PhysicalDeviceC &)> request_gpu_features) :
    vkb::core::VulkanResourceC<VkDevice>{VK_NULL_HANDLE, this}, debug_utils{reinterpret_cast<vkb::core::HPPDebugUtils *>(debug_utils.release())}, gpu{reinterpret_cast<vkb::core::PhysicalDeviceCpp &>(gpu)}, resource_cache{*reinterpret_cast<vkb::core::DeviceCpp *>(this)}, surface(static_cast<vk::SurfaceKHR>(surface))
{
	init(requested_extensions, request_gpu_features);
}

template <>
inline Device<vkb::BindingType::Cpp>::Device(vkb::core::PhysicalDeviceCpp &gpu, vk::Device &vulkan_device, vk::SurfaceKHR surface) :
    VulkanResource{vulkan_device}, gpu{gpu}, surface{surface}, resource_cache{*this}
{
	debug_utils = std::make_unique<HPPDummyDebugUtils>();
}

template <>
inline Device<vkb::BindingType::C>::Device(vkb::core::PhysicalDeviceC &gpu, VkDevice &vulkan_device, VkSurfaceKHR surface) :
    VulkanResource{vulkan_device}, gpu{reinterpret_cast<vkb::core::PhysicalDeviceCpp &>(gpu)}, resource_cache{*reinterpret_cast<vkb::core::DeviceCpp *>(this)}, surface{static_cast<vk::SurfaceKHR>(surface)}
{
	debug_utils = std::make_unique<HPPDummyDebugUtils>();
}

template <vkb::BindingType bindingType>
inline Device<bindingType>::~Device()
{
	resource_cache.clear();
	command_pool.reset();
	fence_pool.reset();
	vkb::allocated::shutdown();
	if (this->get_handle())
	{
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			this->get_handle().destroy();
		}
		else
		{
			static_cast<vk::Device>(this->get_handle()).destroy();
		}
	}
}

template <vkb::BindingType bindingType>
inline void Device<bindingType>::add_queue(size_t global_index, uint32_t family_index, QueueFamilyPropertiesType const &properties, Bool32Type can_present)
{
	if (queues.size() <= global_index)
	{
		queues.resize(global_index + 1);
	}
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		queues[global_index].emplace_back(*this, family_index, properties, can_present, 0);
	}
	else
	{
		queues[global_index].emplace_back(*reinterpret_cast<vkb::core::DeviceCpp *>(this),
		                                  family_index,
		                                  reinterpret_cast<vk::QueueFamilyProperties const &>(properties),
		                                  static_cast<vk::Bool32>(can_present),
		                                  0);
	}
}

template <vkb::BindingType bindingType>
inline void Device<bindingType>::copy_buffer(vkb::core::Buffer<bindingType> const &src,
                                             vkb::core::Buffer<bindingType>       &dst,
                                             QueueType                             queue,
                                             BufferCopyType const                 *copy_region)
{
	assert(dst.get_size() <= src.get_size());
	assert(src.get_handle());

	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		copy_buffer_impl(this->get_handle(), src, dst, queue, copy_region);
	}
	else
	{
		copy_buffer_impl(static_cast<vk::Device>(this->get_handle()), reinterpret_cast<vkb::core::BufferCpp const &>(src),
		                 reinterpret_cast<vkb::core::BufferCpp &>(dst),
		                 static_cast<vk::Queue>(queue),
		                 reinterpret_cast<vk::BufferCopy const *>(copy_region));
	}
}

template <vkb::BindingType bindingType>
inline typename Device<bindingType>::CommandBufferType Device<bindingType>::create_command_buffer(CommandBufferLevelType level, bool begin) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return create_command_buffer_impl(this->get_handle(), level, begin);
	}
	else
	{
		return static_cast<VkCommandBuffer>(
		    create_command_buffer_impl(static_cast<vk::Device>(this->get_handle()), static_cast<vk::CommandBufferLevel>(level), begin));
	}
}

template <vkb::BindingType bindingType>
inline typename Device<bindingType>::CommandPoolType Device<bindingType>::create_command_pool(uint32_t queue_index, CommandPoolCreateFlagsType flags)
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		vk::CommandPoolCreateInfo command_pool_info{.flags = flags, .queueFamilyIndex = queue_index};
		return this->get_handle().createCommandPool(command_pool_info);
	}
	else
	{
		vk::CommandPoolCreateInfo command_pool_info{.flags = static_cast<vk::CommandPoolCreateFlags>(flags), .queueFamilyIndex = queue_index};
		return static_cast<vk::Device>(this->get_handle()).createCommandPool(command_pool_info);
	}
}

template <vkb::BindingType bindingType>
inline std::pair<typename Device<bindingType>::ImageType, typename Device<bindingType>::DeviceMemoryType> Device<bindingType>::create_image(
    FormatType format, Extent2DType const &extent, uint32_t mip_levels, ImageUsageFlagsType usage, MemoryPropertyFlagsType properties) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return create_image_impl(this->get_handle(), format, extent, mip_levels, usage, properties);
	}
	else
	{
		return static_cast<std::pair<VkImage, VkDeviceMemory>>(create_image_impl(static_cast<vk::Device>(this->get_handle()),
		                                                                         static_cast<VkFormat>(format),
		                                                                         static_cast<VkExtent2D>(extent),
		                                                                         mip_levels,
		                                                                         static_cast<VkImageUsageFlags>(usage),
		                                                                         static_cast<VkMemoryPropertyFlags>(properties)));
	}
}

template <vkb::BindingType bindingType>
inline void Device<bindingType>::create_internal_command_pool()
{
	uint32_t familyIndex = get_queue_by_flags_impl(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).get_family_index();
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		command_pool = std::make_unique<vkb::core::CommandPoolCpp>(*this, familyIndex);
	}
	else
	{
		command_pool = std::make_unique<vkb::core::CommandPoolCpp>(*reinterpret_cast<vkb::core::DeviceCpp *>(this), familyIndex);
	}
}

template <vkb::BindingType bindingType>
inline void Device<bindingType>::create_internal_fence_pool()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		fence_pool = std::make_unique<vkb::HPPFencePool>(*this);
	}
	else
	{
		fence_pool = std::make_unique<vkb::HPPFencePool>(*reinterpret_cast<vkb::core::DeviceCpp *>(this));
	}
}

template <vkb::BindingType bindingType>
inline void Device<bindingType>::flush_command_buffer(CommandBufferType command_buffer, QueueType queue, bool free, SemaphoreType signal_semaphore) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		flush_command_buffer_impl(this->get_handle(), command_buffer, queue, free, signal_semaphore);
	}
	else
	{
		flush_command_buffer_impl(static_cast<vk::Device>(this->get_handle()),
		                          static_cast<vk::CommandBuffer>(command_buffer),
		                          static_cast<vk::Queue>(queue),
		                          free,
		                          static_cast<vk::Semaphore>(signal_semaphore));
	}
}

template <vkb::BindingType bindingType>
inline vkb::core::CommandPool<bindingType> &Device<bindingType>::get_command_pool() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *command_pool;
	}
	else
	{
		return reinterpret_cast<vkb::core::CommandPoolC &>(*command_pool);
	}
}

template <vkb::BindingType bindingType>
inline typename Device<bindingType>::DebugUtilsType const &Device<bindingType>::get_debug_utils() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *debug_utils;
	}
	else
	{
		return reinterpret_cast<vkb::DebugUtils const &>(*debug_utils);
	}
}

template <vkb::BindingType bindingType>
inline typename Device<bindingType>::FencePoolType &Device<bindingType>::get_fence_pool() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *fence_pool;
	}
	else
	{
		return reinterpret_cast<vkb::FencePool &>(*fence_pool);
	}
}

template <vkb::BindingType bindingType>
inline PhysicalDevice<bindingType> const &Device<bindingType>::get_gpu() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return gpu;
	}
	else
	{
		return reinterpret_cast<vkb::core::PhysicalDeviceC const &>(gpu);
	}
}

template <vkb::BindingType bindingType>
inline typename Device<bindingType>::CoreQueueType const &Device<bindingType>::get_queue(uint32_t queue_family_index, uint32_t queue_index) const
{
	assert(queue_family_index < queues.size() && "Queue family index out of bounds");
	assert(queue_index < queues[queue_family_index].size() && "Queue index out of bounds");

	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return queues[queue_family_index][queue_index];
	}
	else
	{
		return reinterpret_cast<vkb::Queue const &>(queues[queue_family_index][queue_index]);
	}
}

template <vkb::BindingType bindingType>
inline typename Device<bindingType>::CoreQueueType const &Device<bindingType>::get_queue_by_flags(QueueFlagsType required_queue_flags,
                                                                                                  uint32_t       queue_index) const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return get_queue_by_flags_impl(required_queue_flags, queue_index);
	}
	else
	{
		return reinterpret_cast<vkb::Queue const &>(get_queue_by_flags_impl(static_cast<vk::QueueFlags>(required_queue_flags), queue_index));
	}
}

template <vkb::BindingType bindingType>
inline typename Device<bindingType>::CoreQueueType const &Device<bindingType>::get_queue_by_present(uint32_t queue_index) const
{
	auto queueIt =
	    std::ranges::find_if(queues,
	                         [queue_index](const std::vector<vkb::core::HPPQueue> &queue_family) { return !queue_family.empty() && queue_index < queue_family[0].get_properties().queueCount && queue_family[0].support_present(); });
	if (queueIt != queues.end())
	{
		if constexpr (bindingType == vkb::BindingType::Cpp)
		{
			return (*queueIt)[queue_index];
		}
		else
		{
			return reinterpret_cast<vkb::Queue const &>((*queueIt)[queue_index]);
		}
	}

	throw std::runtime_error("Queue not found");
}

template <vkb::BindingType bindingType>
inline typename Device<bindingType>::ResourceCacheType &Device<bindingType>::get_resource_cache()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return resource_cache;
	}
	else
	{
		return reinterpret_cast<vkb::ResourceCache &>(resource_cache);
	}
}

template <vkb::BindingType bindingType>
inline bool Device<bindingType>::is_extension_enabled(const char *extension) const
{
	return std::ranges::find_if(enabled_extensions, [extension](const char *enabled_extension) { return strcmp(extension, enabled_extension) == 0; }) !=
	       enabled_extensions.end();
}

template <vkb::BindingType bindingType>
inline bool Device<bindingType>::is_image_format_supported(FormatType format) const
{
	// as we want to check for an error (vk::Result::eErrorFormatNotSupported) we use the non-throwing version of getImageFormatProperties here
	vk::ImageFormatProperties format_properties;
	return vk::Result::eErrorFormatNotSupported !=
	       gpu.get_handle().getImageFormatProperties(
	           static_cast<vk::Format>(format), vk::ImageType::e2D, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled, {}, &format_properties);
}

template <vkb::BindingType bindingType>
inline void Device<bindingType>::wait_idle() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		this->get_handle().waitIdle();
	}
	else
	{
		static_cast<vk::Device>(this->get_handle()).waitIdle();
	}
}

template <vkb::BindingType bindingType>
inline void Device<bindingType>::copy_buffer_impl(
    vk::Device device, vkb::core::BufferCpp const &src, vkb::core::BufferCpp &dst, vk::Queue queue, vk::BufferCopy const *copy_region)
{
	vk::CommandBuffer command_buffer = create_command_buffer_impl(device, vk::CommandBufferLevel::ePrimary, true);

	vk::BufferCopy buffer_copy;
	if (copy_region == nullptr)
	{
		buffer_copy.size = src.get_size();
	}
	else
	{
		buffer_copy = *copy_region;
	}

	command_buffer.copyBuffer(src.get_handle(), dst.get_handle(), buffer_copy);

	flush_command_buffer_impl(device, command_buffer, queue);
}

template <vkb::BindingType bindingType>
inline vk::CommandBuffer Device<bindingType>::create_command_buffer_impl(vk::Device device, vk::CommandBufferLevel level, bool begin) const
{
	assert(command_pool && "No command pool exists in the device");

	vk::CommandBufferAllocateInfo command_buffer_allocate_info{.commandPool = command_pool->get_handle(), .level = level, .commandBufferCount = 1};
	vk::CommandBuffer             command_buffer = device.allocateCommandBuffers(command_buffer_allocate_info).front();

	// If requested, also start recording for the new command buffer
	if (begin)
	{
		command_buffer.begin(vk::CommandBufferBeginInfo());
	}

	return command_buffer;
}

template <vkb::BindingType bindingType>
inline std::pair<vk::Image, vk::DeviceMemory> Device<bindingType>::create_image_impl(
    vk::Device device, vk::Format format, vk::Extent2D const &extent, uint32_t mip_levels, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) const
{
	vk::ImageCreateInfo image_create_info{.imageType   = vk::ImageType::e2D,
	                                      .format      = format,
	                                      .extent      = {.width = extent.width, .height = extent.height, .depth = 1},
	                                      .mipLevels   = mip_levels,
	                                      .arrayLayers = 1,
	                                      .samples     = vk::SampleCountFlagBits::e1,
	                                      .tiling      = vk::ImageTiling::eOptimal,
	                                      .usage       = usage};

	vk::Image image = device.createImage(image_create_info);

	vk::MemoryRequirements memory_requirements = device.getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo memory_allocation{.allocationSize  = memory_requirements.size,
	                                         .memoryTypeIndex = gpu.get_memory_type(memory_requirements.memoryTypeBits, properties)};
	vk::DeviceMemory       memory = device.allocateMemory(memory_allocation);
	device.bindImageMemory(image, memory, 0);

	return std::make_pair(image, memory);
}

template <vkb::BindingType bindingType>
inline void Device<bindingType>::flush_command_buffer_impl(
    vk::Device device, vk::CommandBuffer command_buffer, vk::Queue queue, bool free, vk::Semaphore signal_semaphore) const
{
	if (command_buffer)
	{
		command_buffer.end();

		vk::SubmitInfo submit_info{.commandBufferCount = 1, .pCommandBuffers = &command_buffer};
		if (signal_semaphore)
		{
			submit_info.setSignalSemaphores(signal_semaphore);
		}

		// Create fence to ensure that the command buffer has finished executing
		vk::Fence fence = device.createFence({});

		// Submit to the queue
		queue.submit(submit_info, fence);

		// Wait for the fence to signal that command buffer has finished executing
		vk::Result result = device.waitForFences(fence, true, DEFAULT_FENCE_TIMEOUT);
		if (result != vk::Result::eSuccess)
		{
			LOGE("Detected Vulkan error: {}", vkb::to_string(result));
			abort();
		}

		device.destroyFence(fence);

		if (command_pool && free)
		{
			device.freeCommandBuffers(command_pool->get_handle(), command_buffer);
		}
	}
}

template <vkb::BindingType bindingType>
vkb::core::HPPQueue const &Device<bindingType>::get_queue_by_flags_impl(vk::QueueFlags required_queue_flags, uint32_t queue_index) const
{
	auto queueIt =
	    std::ranges::find_if(queues,
	                         [required_queue_flags, queue_index](const std::vector<vkb::core::HPPQueue> &queue) {
		                         assert(!queue.empty());
		                         vk::QueueFamilyProperties const &properties = queue[0].get_properties();
		                         return ((properties.queueFlags & required_queue_flags) == required_queue_flags) && (queue_index < properties.queueCount);
	                         });

	if (queueIt == queues.end())
	{
		throw std::runtime_error("Queue not found");
	}

	return (*queueIt)[queue_index];
}

template <vkb::BindingType bindingType>
inline void Device<bindingType>::init(std::unordered_map<const char *, bool> const &requested_extensions, std::function<void(vkb::core::PhysicalDevice<bindingType> &)> request_gpu_features)
{
	LOGI("Selected GPU: {}", *gpu.get_properties().deviceName);

	// Prepare the device queues
	std::vector<vk::QueueFamilyProperties> queue_family_properties = gpu.get_queue_family_properties();
	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
	std::vector<std::vector<float>>        queue_priorities;

	queue_create_infos.reserve(queue_family_properties.size());
	queue_priorities.reserve(queue_family_properties.size());
	for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index)
	{
		auto const &queue_family_property = queue_family_properties[queue_family_index];

		queue_priorities.push_back(std::vector<float>(queue_family_property.queueCount, 0.5f));
		if (gpu.has_high_priority_graphics_queue() &&
		    (vkb::common::get_queue_family_index(queue_family_properties, vk::QueueFlagBits::eGraphics) == queue_family_index))
		{
			queue_priorities.back()[0] = 0.5f;
		}

		queue_create_infos.push_back({.queueFamilyIndex = queue_family_index,
		                              .queueCount       = queue_family_property.queueCount,
		                              .pQueuePriorities = queue_priorities[queue_family_index].data()});
	}

	// Check extensions to enable Vma Dedicated Allocation
	bool can_get_memory_requirements = gpu.is_extension_supported("VK_KHR_get_memory_requirements2");
	bool has_dedicated_allocation    = gpu.is_extension_supported("VK_KHR_dedicated_allocation");

	if (can_get_memory_requirements && has_dedicated_allocation)
	{
		enabled_extensions.push_back("VK_KHR_get_memory_requirements2");
		enabled_extensions.push_back("VK_KHR_dedicated_allocation");

		LOGI("Dedicated Allocation enabled");
	}

	// For performance queries, we also use host query reset since queryPool resets cannot
	// live in the same command buffer as beginQuery
	if (gpu.is_extension_supported("VK_KHR_performance_query") &&
	    gpu.is_extension_supported("VK_EXT_host_query_reset"))
	{
		auto perf_counter_features     = gpu.get_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>();
		auto host_query_reset_features = gpu.get_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>();

		if (perf_counter_features.performanceCounterQueryPools && host_query_reset_features.hostQueryReset)
		{
			gpu.add_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>().performanceCounterQueryPools = VK_TRUE;
			gpu.add_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>().hostQueryReset                    = VK_TRUE;
			enabled_extensions.push_back("VK_KHR_performance_query");
			enabled_extensions.push_back("VK_EXT_host_query_reset");
			LOGI("Performance query enabled");
		}
	}

	// Check that extensions are supported before trying to create the device
	std::vector<const char *> unsupported_extensions{};
	for (auto &extension : requested_extensions)
	{
		if (gpu.is_extension_supported(extension.first))
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
			auto extIt = requested_extensions.find(extension);
			assert(extIt != requested_extensions.end());
			if (extIt->second)
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

	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		request_gpu_features(gpu);
	}
	else
	{
		request_gpu_features(reinterpret_cast<vkb::core::PhysicalDeviceC &>(gpu));
	}

	// Latest requested feature will have the pNext's all set up for device creation.
	vk::DeviceCreateInfo create_info{.pNext                   = gpu.get_extension_feature_chain(),
	                                 .queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size()),
	                                 .pQueueCreateInfos       = queue_create_infos.data(),
	                                 .enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size()),
	                                 .ppEnabledExtensionNames = enabled_extensions.data(),
	                                 .pEnabledFeatures        = &gpu.get_requested_features()};

	this->set_handle(gpu.get_handle().createDevice(create_info));

	queues.resize(queue_family_properties.size());

	for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index)
	{
		const vk::QueueFamilyProperties &queue_family_property = gpu.get_queue_family_properties()[queue_family_index];

		vk::Bool32 present_supported = gpu.get_handle().getSurfaceSupportKHR(queue_family_index, surface);

		for (uint32_t queue_index = 0U; queue_index < queue_family_property.queueCount; ++queue_index)
		{
			if constexpr (bindingType == BindingType::Cpp)
			{
				queues[queue_family_index].emplace_back(*this, queue_family_index, queue_family_property, present_supported, queue_index);
			}
			else
			{
				queues[queue_family_index].emplace_back(
				    *reinterpret_cast<vkb::core::DeviceCpp *>(this), queue_family_index, queue_family_property, present_supported, queue_index);
			}
		}
	}

	vkb::allocated::init(*this);

	if constexpr (bindingType == BindingType::Cpp)
	{
		command_pool = std::make_unique<vkb::core::CommandPoolCpp>(
		    *this, get_queue_by_flags_impl(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).get_family_index());
		fence_pool = std::make_unique<vkb::HPPFencePool>(*this);
	}
	else
	{
		command_pool = std::make_unique<vkb::core::CommandPoolCpp>(
		    *reinterpret_cast<vkb::core::DeviceCpp *>(this),
		    get_queue_by_flags_impl(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, 0).get_family_index());
		fence_pool = std::make_unique<vkb::HPPFencePool>(*reinterpret_cast<vkb::core::DeviceCpp *>(this));
	}
}

}        // namespace core
}        // namespace vkb
