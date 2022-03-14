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

#include <core/debug.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
/**
 * @brief facade class around vkb::DebugUtils, providing a vulkan.hpp-based interface
 *
 * See vkb::DebugUtils for documentation
 */
class HPPDebugUtils : private vkb::DebugUtils
{
  public:
	void set_debug_name(VkDevice     device,
	                    VkObjectType object_type,
	                    uint64_t     object_handle,
	                    const char * name) const override
	{
		set_debug_name(
		    static_cast<vk::Device>(device), static_cast<vk::ObjectType>(object_type), object_handle, name);
	}

	void set_debug_tag(VkDevice     device,
	                   VkObjectType object_type,
	                   uint64_t     object_handle,
	                   uint64_t     tag_name,
	                   const void * tag_data,
	                   size_t       tag_data_size) const override
	{
		set_debug_tag(static_cast<vk::Device>(device),
		              static_cast<vk::ObjectType>(object_type),
		              object_handle,
		              tag_name,
		              tag_data,
		              tag_data_size);
	}

	void cmd_begin_label(VkCommandBuffer command_buffer, const char *name, glm::vec4 color) const override
	{
		cmd_begin_label(static_cast<vk::CommandBuffer>(command_buffer), name, color);
	}

	void cmd_end_label(VkCommandBuffer command_buffer) const override
	{
		cmd_end_label(static_cast<vk::CommandBuffer>(command_buffer));
	}

	void cmd_insert_label(VkCommandBuffer command_buffer, const char *name, glm::vec4 color) const override
	{
		cmd_insert_label(static_cast<vk::CommandBuffer>(command_buffer), name, color);
	}

	virtual void set_debug_name(vk::Device     device,
	                            vk::ObjectType object_type,
	                            uint64_t       object_handle,
	                            const char *   name) const                                                           = 0;
	virtual void set_debug_tag(vk::Device     device,
	                           vk::ObjectType object_type,
	                           uint64_t       object_handle,
	                           uint64_t       tag_name,
	                           const void *   tag_data,
	                           size_t         tag_data_size) const                                                        = 0;
	virtual void cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 color = {}) const  = 0;
	virtual void cmd_end_label(vk::CommandBuffer command_buffer) const                                            = 0;
	virtual void cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 color = {}) const = 0;
};

/**
 * @brief facade class around vkb::DebugUtilsExtDebugUtils, providing a vulkan.hpp-based interface
 *
 * See vkb::DebugUtilsExtDebugUtils for documentation
 */
class HPPDebugUtilsExtDebugUtils final : public vkb::core::HPPDebugUtils
{
  public:
	virtual void set_debug_name(vk::Device     device,
	                            vk::ObjectType object_type,
	                            uint64_t       object_handle,
	                            const char *   name) const override
	{
		reinterpret_cast<vkb::DebugUtilsExtDebugUtils const *>(this)->set_debug_name(
		    static_cast<VkDevice>(device), static_cast<VkObjectType>(object_type), object_handle, name);
	}

	virtual void set_debug_tag(vk::Device     device,
	                           vk::ObjectType object_type,
	                           uint64_t       object_handle,
	                           uint64_t       tag_name,
	                           const void *   tag_data,
	                           size_t         tag_data_size) const override
	{
		reinterpret_cast<vkb::DebugUtilsExtDebugUtils const *>(this)->set_debug_tag(
		    static_cast<VkDevice>(device),
		    static_cast<VkObjectType>(object_type),
		    object_handle,
		    tag_name,
		    tag_data,
		    tag_data_size);
	}

	virtual void
	    cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 color) const override
	{
		reinterpret_cast<vkb::DebugUtilsExtDebugUtils const *>(this)->cmd_begin_label(
		    static_cast<VkCommandBuffer>(command_buffer), name, color);
	}

	virtual void cmd_end_label(vk::CommandBuffer command_buffer) const override
	{
		reinterpret_cast<vkb::DebugUtilsExtDebugUtils const *>(this)->cmd_end_label(
		    static_cast<VkCommandBuffer>(command_buffer));
	}

	virtual void
	    cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 color) const override
	{
		reinterpret_cast<vkb::DebugUtilsExtDebugUtils const *>(this)->cmd_insert_label(
		    static_cast<VkCommandBuffer>(command_buffer), name, color);
	}
};

/**
 * @brief facade class around vkb::DebugMarkerExtDebugUtils, providing a vulkan.hpp-based interface
 *
 * See vkb::DebugMarkerExtDebugUtils for documentation
 */
class HPPDebugMarkerExtDebugUtils final : public vkb::core::HPPDebugUtils
{
  public:
	virtual void set_debug_name(vk::Device     device,
	                            vk::ObjectType object_type,
	                            uint64_t       object_handle,
	                            const char *   name) const override
	{
		reinterpret_cast<vkb::DebugMarkerExtDebugUtils const *>(this)->set_debug_name(
		    static_cast<VkDevice>(device), static_cast<VkObjectType>(object_type), object_handle, name);
	}

	virtual void set_debug_tag(vk::Device     device,
	                           vk::ObjectType object_type,
	                           uint64_t       object_handle,
	                           uint64_t       tag_name,
	                           const void *   tag_data,
	                           size_t         tag_data_size) const override
	{
		reinterpret_cast<vkb::DebugMarkerExtDebugUtils const *>(this)->set_debug_tag(
		    static_cast<VkDevice>(device),
		    static_cast<VkObjectType>(object_type),
		    object_handle,
		    tag_name,
		    tag_data,
		    tag_data_size);
	}

	virtual void
	    cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 color) const override
	{
		reinterpret_cast<vkb::DebugMarkerExtDebugUtils const *>(this)->cmd_begin_label(
		    static_cast<VkCommandBuffer>(command_buffer), name, color);
	}

	virtual void cmd_end_label(vk::CommandBuffer command_buffer) const override
	{
		reinterpret_cast<vkb::DebugMarkerExtDebugUtils const *>(this)->cmd_end_label(
		    static_cast<VkCommandBuffer>(command_buffer));
	}

	virtual void
	    cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 color) const override
	{
		reinterpret_cast<vkb::DebugMarkerExtDebugUtils const *>(this)->cmd_insert_label(
		    static_cast<VkCommandBuffer>(command_buffer), name, color);
	}
};

/**
 * @brief facade class around vkb::DummyDebugUtils, providing a vulkan.hpp-based interface
 *
 * See vkb::DummyDebugUtils for documentation
 */
class HPPDummyDebugUtils final : public vkb::core::HPPDebugUtils
{
  public:
	virtual void set_debug_name(vk::Device     device,
	                            vk::ObjectType object_type,
	                            uint64_t       object_handle,
	                            const char *   name) const override
	{
		reinterpret_cast<vkb::DummyDebugUtils const *>(this)->set_debug_name(
		    static_cast<VkDevice>(device), static_cast<VkObjectType>(object_type), object_handle, name);
	}

	virtual void set_debug_tag(vk::Device     device,
	                           vk::ObjectType object_type,
	                           uint64_t       object_handle,
	                           uint64_t       tag_name,
	                           const void *   tag_data,
	                           size_t         tag_data_size) const override
	{
		reinterpret_cast<vkb::DummyDebugUtils const *>(this)->set_debug_tag(static_cast<VkDevice>(device),
		                                                                    static_cast<VkObjectType>(object_type),
		                                                                    object_handle,
		                                                                    tag_name,
		                                                                    tag_data,
		                                                                    tag_data_size);
	}

	virtual void
	    cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 color) const override
	{
		reinterpret_cast<vkb::DummyDebugUtils const *>(this)->cmd_begin_label(
		    static_cast<VkCommandBuffer>(command_buffer), name, color);
	}

	virtual void cmd_end_label(vk::CommandBuffer command_buffer) const override
	{
		reinterpret_cast<vkb::DummyDebugUtils const *>(this)->cmd_end_label(
		    static_cast<VkCommandBuffer>(command_buffer));
	}

	virtual void
	    cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 color) const override
	{
		reinterpret_cast<vkb::DummyDebugUtils const *>(this)->cmd_insert_label(
		    static_cast<VkCommandBuffer>(command_buffer), name, color);
	}
};

}        // namespace core
}        // namespace vkb
