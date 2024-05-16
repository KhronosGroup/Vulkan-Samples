/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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
namespace core
{
class HPPCommandBuffer;

/**
 * @brief An interface over platform-specific debug extensions.
 */
class HPPDebugUtils
{
  public:
	virtual ~HPPDebugUtils() = default;

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
};

/**
 * @brief HPPDebugUtils implemented on top of VK_EXT_debug_utils.
 */
class HPPDebugUtilsExtDebugUtils final : public vkb::core::HPPDebugUtils
{
  public:
	~HPPDebugUtilsExtDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override;

	void set_debug_tag(
	    vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void *tag_data, size_t tag_data_size) const override;

	void cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const override;

	void cmd_end_label(vk::CommandBuffer command_buffer) const override;

	void cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const override;
};

/**
 * @brief HPPDebugUtils implemented on top of VK_EXT_debug_marker.
 */
class HPPDebugMarkerExtDebugUtils final : public vkb::core::HPPDebugUtils
{
  public:
	~HPPDebugMarkerExtDebugUtils() override = default;

	void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const override;

	void set_debug_tag(
	    vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void *tag_data, size_t tag_data_size) const override;

	void cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const override;

	void cmd_end_label(vk::CommandBuffer command_buffer) const override;

	void cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const override;
};

/**
 * @brief No-op HPPDebugUtils.
 */
class HPPDummyDebugUtils final : public vkb::core::HPPDebugUtils
{
  public:
	~HPPDummyDebugUtils() override = default;

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
class HPPScopedDebugLabel final
{
  public:
	HPPScopedDebugLabel(const vkb::core::HPPDebugUtils &debug_utils, vk::CommandBuffer command_buffer, std::string const &name, glm::vec4 const color = {});

	HPPScopedDebugLabel(const vkb::core::HPPCommandBuffer &command_buffer, std::string const &name, glm::vec4 const color = {});

	~HPPScopedDebugLabel();

  private:
	const vkb::core::HPPDebugUtils *debug_utils;
	vk::CommandBuffer               command_buffer;
};

}        // namespace core
}        // namespace vkb