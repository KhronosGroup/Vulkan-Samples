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

#pragma once

#include <common/glm_common.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
class CommandBuffer;

namespace core
{
class HPPCommandBuffer;

/**
 * @brief An interface over platform-specific debug extensions.
 */
class DebugUtils
{
  public:
	virtual ~DebugUtils() = default;

	//===========================
	//=== Vulkan C++-bindings ===
	//===========================

	/**
	 * @brief Sets the debug name for a Vulkan object.
	 */
	virtual void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const = 0;

	/**
	 * @brief Tags the given Vulkan object with some data.
	 */
	virtual void set_debug_tag(
	    vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void *tag_data, size_t tag_data_size) const = 0;

	/**
	 * @brief Inserts a command to begin a new debug label/marker scope.
	 */
	virtual void cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color = {}) const = 0;

	/**
	 * @brief Inserts a command to end the current debug label/marker scope.
	 */
	virtual void cmd_end_label(vk::CommandBuffer command_buffer) const = 0;

	/**
	 * @brief Inserts a (non-scoped) debug label/marker in the command buffer.
	 */
	virtual void cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color = {}) const = 0;

	//=========================
	//=== Vulkan C-bindings ===
	//=========================

	/**
	 * @brief Sets the debug name for a Vulkan object
	 */
	void set_debug_name(VkDevice device, VkObjectType object_type, uint64_t object_handle, const char *name) const
	{
		set_debug_name(static_cast<vk::Device>(device), static_cast<vk::ObjectType>(object_type), object_handle, name);
	}

	/**
	 * @brief Tags the given Vulkan object with some data
	 */
	void set_debug_tag(VkDevice device, VkObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void *tag_data, size_t tag_data_size) const
	{
		set_debug_tag(static_cast<vk::Device>(device), static_cast<vk::ObjectType>(object_type), object_handle, tag_name, tag_data, tag_data_size);
	}

	/**
	 * @brief Inserts a command to begin a new debug label/marker scope.
	 */
	void cmd_begin_label(VkCommandBuffer command_buffer, const char *name, glm::vec4 const color = {}) const
	{
		cmd_begin_label(static_cast<vk::CommandBuffer>(command_buffer), name, color);
	}

	/**
	 * @brief Inserts a command to end the current debug label/marker scope.
	 */
	void cmd_end_label(VkCommandBuffer command_buffer) const
	{
		cmd_end_label(static_cast<vk::CommandBuffer>(command_buffer));
	}

	/**
	 * @brief Inserts a (non-scoped) debug label/marker in the command buffer.
	 */
	void cmd_insert_label(VkCommandBuffer command_buffer, const char *name, glm::vec4 const color = {}) const
	{
		cmd_insert_label(static_cast<vk::CommandBuffer>(command_buffer), name, color);
	}
};

/**
 * @brief DebugUtils implemented on top of VK_EXT_debug_utils.
 */
class DebugUtilsExtDebugUtils final : public vkb::core::DebugUtils
{
  public:
	~DebugUtilsExtDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override;

	void set_debug_tag(
	    vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void *tag_data, size_t tag_data_size) const override;

	void cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const override;

	void cmd_end_label(vk::CommandBuffer command_buffer) const override;

	void cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const override;
};

/**
 * @brief DebugUtils implemented on top of VK_EXT_debug_marker.
 */
class DebugMarkerExtDebugUtils final : public vkb::core::DebugUtils
{
  public:
	~DebugMarkerExtDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override;

	void set_debug_tag(
	    vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void *tag_data, size_t tag_data_size) const override;

	void cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const override;

	void cmd_end_label(vk::CommandBuffer command_buffer) const override;

	void cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const override;
};

/**
 * @brief No-op DebugUtils.
 */
class DummyDebugUtils final : public vkb::core::DebugUtils
{
  public:
	~DummyDebugUtils() override = default;

	inline void set_debug_name(vk::Device, vk::ObjectType, uint64_t, const char *) const override
	{}

	inline void set_debug_tag(vk::Device, vk::ObjectType, uint64_t, uint64_t, const void *, size_t) const override
	{}

	inline void cmd_begin_label(vk::CommandBuffer, const char *, glm::vec4 const) const override
	{}

	inline void cmd_end_label(vk::CommandBuffer) const override
	{}

	inline void cmd_insert_label(vk::CommandBuffer, const char *, glm::vec4 const) const override
	{}
};

/**
 * @brief A RAII debug label.
 *        If any of EXT_debug_utils or EXT_debug_marker is available, this:
 *        - Begins a debug label / marker on construction
 *        - Ends it on destruction
 */
class ScopedDebugLabel final
{
  public:
	ScopedDebugLabel(const vkb::core::HPPCommandBuffer &command_buffer, std::string const &name, glm::vec4 const color = {});

	ScopedDebugLabel(const vkb::CommandBuffer &command_buffer, const char *name, glm::vec4 color = {}) :
	    ScopedDebugLabel{reinterpret_cast<vkb::core::HPPCommandBuffer const &>(command_buffer), name, color}
	{
	}

	~ScopedDebugLabel();

  private:
	vkb::core::HPPCommandBuffer const *command_buffer;
};

}        // namespace core
}        // namespace vkb
