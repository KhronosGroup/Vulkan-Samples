/* Copyright (c) 2021-2024, Arm Limited and Contributors
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

#include "common/vk_common.h"
#include <cassert>
#include <cstdint>
#include <type_traits>

namespace vkb
{
class Device;

namespace core
{
namespace detail
{
void set_debug_name(const Device *device, VkObjectType object_type, uint64_t handle, const char *debug_name);
}

template <typename HandleType>
constexpr VkObjectType get_object_type(const HandleType &handle)
{
	throw std::runtime_error("Unknown handle type");
	return static_cast<VkObjectType>(-1);
}

template <>
constexpr VkObjectType get_object_type(const VkImage &handle)
{
	return VK_OBJECT_TYPE_IMAGE;
}

template <>
constexpr VkObjectType get_object_type(const VkImageView &handle)
{
	return VK_OBJECT_TYPE_IMAGE_VIEW;
}

template <>
constexpr VkObjectType get_object_type(const VkRenderPass &handle)
{
	return VK_OBJECT_TYPE_RENDER_PASS;
}

template <>
constexpr VkObjectType get_object_type(const VkSampler &handle)
{
	return VK_OBJECT_TYPE_SAMPLER;
}

template <>
constexpr VkObjectType get_object_type(const VkBuffer &handle)
{
	return VK_OBJECT_TYPE_BUFFER;
}

template <>
constexpr VkObjectType get_object_type(const VkDevice &handle)
{
	return VK_OBJECT_TYPE_DEVICE;
}

template <>
constexpr VkObjectType get_object_type(const VkCommandBuffer &handle)
{
	return VK_OBJECT_TYPE_COMMAND_BUFFER;
}

/// Inherit this for any Vulkan object with a handle of type `THandle`.
///
/// This allows the derived class to store a Vulkan handle, and also a pointer to the parent Device.
/// It also allow for adding debug data to any Vulkan object.
template <typename THandle, typename Device = vkb::Device>
class VulkanResource
{
  public:
	VulkanResource(THandle handle = VK_NULL_HANDLE, Device *device = nullptr) :
	    handle{handle}, device{device}
	{
	}

	VulkanResource(const VulkanResource &)            = delete;
	VulkanResource &operator=(const VulkanResource &) = delete;

	VulkanResource(VulkanResource &&other) :
	    handle{other.handle}, device{other.device}
	{
		set_debug_name(other.debug_name);

		other.handle = VK_NULL_HANDLE;
	}

	VulkanResource &operator=(VulkanResource &&other)
	{
		handle = other.handle;
		device = other.device;
		set_debug_name(other.debug_name);

		other.handle = VK_NULL_HANDLE;

		return *this;
	}

	virtual ~VulkanResource() = default;

	constexpr VkObjectType get_object_type() const
	{
		return vkb::core::get_object_type(handle);
	}

	inline Device &get_device() const
	{
		assert(device && "Device handle not set");
		return *device;
	}

	inline const THandle &get_handle() const
	{
		return handle;
	}

	inline const uint64_t get_handle_u64() const
	{
		// See https://github.com/KhronosGroup/Vulkan-Docs/issues/368 .
		// Dispatchable and non-dispatchable handle types are *not* necessarily binary-compatible!
		// Non-dispatchable handles _might_ be only 32-bit long. This is because, on 32-bit machines, they might be a typedef to a 32-bit pointer.
		using UintHandle = typename std::conditional<sizeof(THandle) == sizeof(uint32_t), uint32_t, uint64_t>::type;

		return static_cast<uint64_t>(reinterpret_cast<UintHandle>(handle));
	}

	inline const std::string &get_debug_name() const
	{
		return debug_name;
	}

	inline void set_debug_name(const std::string &name)
	{
		debug_name = name;
		detail::set_debug_name(device, get_object_type(), get_handle_u64(), debug_name.c_str());
	}

  protected:
	THandle     handle;
	Device     *device;
	std::string debug_name;
};

}        // namespace core
}        // namespace vkb
