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

#pragma once

#include <utility>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
class HPPDevice;

namespace detail
{
void set_debug_name(const HPPDevice *device, vk::ObjectType object_type, uint64_t handle, const char *debug_name);
}

/// Inherit this for any Vulkan object with a handle of type `HPPHandle`.
///
/// This allows the derived class to store a Vulkan handle, and also a pointer to the parent vkb::core::Device.
/// It also allow for adding debug data to any Vulkan object.
template <typename HPPHandle, typename VKBDevice = vkb::core::HPPDevice>
class HPPVulkanResource
{
  public:
	HPPVulkanResource(HPPHandle handle = nullptr, VKBDevice *device = nullptr) :
	    handle{handle}, device{device}
	{
	}

	HPPVulkanResource(const HPPVulkanResource &) = delete;
	HPPVulkanResource &operator=(const HPPVulkanResource &) = delete;

	HPPVulkanResource(HPPVulkanResource &&other) :
	    handle(std::exchange(other.handle, {})), device(std::exchange(other.device, {}))
	{
		set_debug_name(std::exchange(other.debug_name, {}));
	}

	HPPVulkanResource &operator=(HPPVulkanResource &&other)
	{
		handle = std::exchange(other.handle, {});
		device = std::exchange(other.device, {});
		set_debug_name(std::exchange(other.debug_name, {}));

		return *this;
	}

	virtual ~HPPVulkanResource() = default;

	inline vk::ObjectType get_object_type() const
	{
		return HPPHandle::NativeType;
	}

	inline VKBDevice &get_device() const
	{
		assert(device && "VKBDevice handle not set");
		return *device;
	}

	inline const HPPHandle &get_handle() const
	{
		return handle;
	}

	inline const uint64_t get_handle_u64() const
	{
		// See https://github.com/KhronosGroup/Vulkan-Docs/issues/368 .
		// Dispatchable and non-dispatchable handle types are *not* necessarily binary-compatible!
		// Non-dispatchable handles _might_ be only 32-bit long. This is because, on 32-bit machines, they might be a typedef to a 32-bit pointer.
		using UintHandle = typename std::conditional<sizeof(HPPHandle) == sizeof(uint32_t), uint32_t, uint64_t>::type;

		return static_cast<uint64_t>(reinterpret_cast<UintHandle>(handle));
	}

	inline void set_handle(HPPHandle hdl)
	{
		assert(!handle && hdl);
		handle = hdl;
	}

	inline const std::string &get_debug_name() const
	{
		return debug_name;
	}

	inline void set_debug_name(const std::string &name)
	{
		debug_name = name;
		detail::set_debug_name(device, HPPHandle::NativeType, get_handle_u64(), debug_name.c_str());
	}

  private:
	HPPHandle   handle;
	VKBDevice * device;
	std::string debug_name;
};

}        // namespace core
}        // namespace vkb
