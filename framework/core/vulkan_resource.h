/* Copyright (c) 2021-2024, Arm Limited and Contributors
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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
#include "vulkan_type_mapping.h"

#include <utility>
#include <vulkan/vulkan.hpp>

namespace vkb
{
class Device;

namespace core
{
class HPPDevice;

/// Inherit this for any Vulkan object with a handle of type `HPPHandle`.
///
/// This allows the derived class to store a Vulkan handle, and also a pointer to the parent vkb::core::Device.
/// It also allows to set a debug name for any Vulkan object.
template <vkb::BindingType bindingType, typename Handle>
class VulkanResource
{
  public:
	using DeviceType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vkb::core::HPPDevice, vkb::Device>::type;
	using ObjectType = typename std::conditional<bindingType == vkb::BindingType::Cpp, vk::ObjectType, VkObjectType>::type;

	VulkanResource(Handle handle = nullptr, DeviceType *device_ = nullptr);

	VulkanResource(const VulkanResource &)            = delete;
	VulkanResource &operator=(const VulkanResource &) = delete;

	VulkanResource(VulkanResource &&other);
	VulkanResource &operator=(VulkanResource &&other);

	virtual ~VulkanResource() = default;

	const std::string &get_debug_name() const;
	DeviceType        &get_device();
	DeviceType const  &get_device() const;
	Handle            &get_handle();
	const Handle      &get_handle() const;
	uint64_t           get_handle_u64() const;
	ObjectType         get_object_type() const;
	bool               has_device() const;
	bool               has_handle() const;
	void               set_debug_name(const std::string &name);
	void               set_handle(Handle hdl);

  private:
	// we always want to store a vk::Handle as a resource, so we have to figure out that type, depending on the BindingType!
	using ResourceType = typename vkb::VulkanTypeMapping<bindingType, Handle>::Type;

	std::string  debug_name;
	HPPDevice   *device;
	ResourceType handle;
};

template <vkb::BindingType bindingType, typename Handle>
inline VulkanResource<bindingType, Handle>::VulkanResource(Handle handle_, DeviceType *device_) :
    handle{handle_}
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		device = device_;
	}
	else
	{
		device = reinterpret_cast<vkb::core::HPPDevice *>(device_);
	}
}

template <vkb::BindingType bindingType, typename Handle>
inline VulkanResource<bindingType, Handle>::VulkanResource(VulkanResource &&other) :
    handle(std::exchange(other.handle, {})),
    device(std::exchange(other.device, {})),
    debug_name(std::exchange(other.debug_name, {}))
{}

template <vkb::BindingType bindingType, typename Handle>
inline VulkanResource<bindingType, Handle> &VulkanResource<bindingType, Handle>::operator=(VulkanResource &&other)
{
	handle     = std::exchange(other.handle, {});
	device     = std::exchange(other.device, {});
	debug_name = std::exchange(other.debug_name, {});
	return *this;
}

template <vkb::BindingType bindingType, typename Handle>
inline const std::string &VulkanResource<bindingType, Handle>::get_debug_name() const
{
	return debug_name;
}

template <vkb::BindingType bindingType, typename Handle>
inline typename VulkanResource<bindingType, Handle>::DeviceType &VulkanResource<bindingType, Handle>::get_device()
{
	assert(device && "VKBDevice handle not set");
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *device;
	}
	else
	{
		return *reinterpret_cast<vkb::Device *>(device);
	}
}

template <vkb::BindingType bindingType, typename Handle>
inline typename VulkanResource<bindingType, Handle>::DeviceType const &VulkanResource<bindingType, Handle>::get_device() const
{
	assert(device && "VKBDevice handle not set");
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return *device;
	}
	else
	{
		return *reinterpret_cast<vkb::Device const *>(device);
	}
}

template <vkb::BindingType bindingType, typename Handle>
inline Handle &VulkanResource<bindingType, Handle>::get_handle()
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return handle;
	}
	else
	{
		return *reinterpret_cast<typename ResourceType::NativeType *>(&handle);
	}
}

template <vkb::BindingType bindingType, typename Handle>
inline const Handle &VulkanResource<bindingType, Handle>::get_handle() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return handle;
	}
	else
	{
		return *reinterpret_cast<typename ResourceType::NativeType const *>(&handle);
	}
}

template <vkb::BindingType bindingType, typename Handle>
inline uint64_t VulkanResource<bindingType, Handle>::get_handle_u64() const
{
	// See https://github.com/KhronosGroup/Vulkan-Docs/issues/368 .
	// Dispatchable and non-dispatchable handle types are *not* necessarily binary-compatible!
	// Non-dispatchable handles _might_ be only 32-bit long. This is because, on 32-bit machines, they might be a typedef to a 32-bit pointer.
	using UintHandle = typename std::conditional<sizeof(ResourceType) == sizeof(uint32_t), uint32_t, uint64_t>::type;

	return static_cast<uint64_t>(*reinterpret_cast<UintHandle const *>(&handle));
}

template <vkb::BindingType bindingType, typename Handle>
inline typename VulkanResource<bindingType, Handle>::ObjectType VulkanResource<bindingType, Handle>::get_object_type() const
{
	if constexpr (bindingType == vkb::BindingType::Cpp)
	{
		return ResourceType::objectType;
	}
	else
	{
		return static_cast<VkObjectType>(ResourceType::objectType);
	}
}

template <vkb::BindingType bindingType, typename Handle>
inline bool VulkanResource<bindingType, Handle>::has_device() const
{
	return device != nullptr;
}

template <vkb::BindingType bindingType, typename Handle>
inline bool VulkanResource<bindingType, Handle>::has_handle() const
{
	return handle != nullptr;
}

template <vkb::BindingType bindingType, typename Handle>
inline void VulkanResource<bindingType, Handle>::set_debug_name(const std::string &name)
{
	debug_name = name;

	if (device && !debug_name.empty())
	{
		get_device().get_debug_utils().set_debug_name(get_device().get_handle(), get_object_type(), get_handle_u64(), debug_name.c_str());
	}
}

template <vkb::BindingType bindingType, typename Handle>
inline void VulkanResource<bindingType, Handle>::set_handle(Handle hdl)
{
	handle = hdl;
}

template <typename Handle>
using VulkanResourceC = VulkanResource<vkb::BindingType::C, Handle>;
template <typename Handle>
using VulkanResourceCpp = VulkanResource<vkb::BindingType::Cpp, Handle>;
}        // namespace core
}        // namespace vkb
