/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "debug.h"
#include "core/hpp_command_buffer.h"
#include "core/hpp_device.h"

namespace vkb
{
namespace core
{
void DebugUtilsExtDebugUtils::set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const
{
	vk::DebugUtilsObjectNameInfoEXT name_info(object_type, object_handle, name);
	device.setDebugUtilsObjectNameEXT(name_info);
}

void DebugUtilsExtDebugUtils::set_debug_tag(
    vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void *tag_data, size_t tag_data_size) const
{
	vk::DebugUtilsObjectTagInfoEXT tag_info(object_type, object_handle, tag_name, tag_data_size, tag_data);
	device.setDebugUtilsObjectTagEXT(tag_info);
}

void DebugUtilsExtDebugUtils::cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const
{
	vk::DebugUtilsLabelEXT label_info(name, reinterpret_cast<std::array<float, 4> const &>(*&color[0]));
	command_buffer.beginDebugUtilsLabelEXT(label_info);
}

void DebugUtilsExtDebugUtils::cmd_end_label(vk::CommandBuffer command_buffer) const
{
	command_buffer.endDebugUtilsLabelEXT();
}

void DebugUtilsExtDebugUtils::cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const
{
	vk::DebugUtilsLabelEXT label_info(name, reinterpret_cast<std::array<float, 4> const &>(*&color[0]));
	command_buffer.insertDebugUtilsLabelEXT(label_info);
}

void DebugMarkerExtDebugUtils::set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char *name) const
{
	vk::DebugMarkerObjectNameInfoEXT name_info(vk::debugReportObjectType(object_type), object_handle, name);
	device.debugMarkerSetObjectNameEXT(name_info);
}

void DebugMarkerExtDebugUtils::set_debug_tag(
    vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void *tag_data, size_t tag_data_size) const
{
	vk::DebugMarkerObjectTagInfoEXT tag_info(vk::debugReportObjectType(object_type), object_handle, tag_name, tag_data_size, tag_data);
	device.debugMarkerSetObjectTagEXT(tag_info);
}

void DebugMarkerExtDebugUtils::cmd_begin_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const
{
	vk::DebugMarkerMarkerInfoEXT marker_info(name, reinterpret_cast<std::array<float, 4> const &>(*&color[0]));
	command_buffer.debugMarkerBeginEXT(marker_info);
}

void DebugMarkerExtDebugUtils::cmd_end_label(vk::CommandBuffer command_buffer) const
{
	command_buffer.debugMarkerEndEXT();
}

void DebugMarkerExtDebugUtils::cmd_insert_label(vk::CommandBuffer command_buffer, const char *name, glm::vec4 const color) const
{
	vk::DebugMarkerMarkerInfoEXT marker_info(name, reinterpret_cast<std::array<float, 4> const &>(*&color[0]));
	command_buffer.debugMarkerInsertEXT(marker_info);
}

ScopedDebugLabel::ScopedDebugLabel(const vkb::core::HPPCommandBuffer &command_buffer_, std::string const &name, glm::vec4 const color)
{
	if (!name.empty())
	{
		assert(command_buffer);
		command_buffer = &command_buffer_;

		command_buffer->get_device().get_debug_utils().cmd_begin_label(command_buffer->get_handle(), name.c_str(), color);
	}
}

ScopedDebugLabel::~ScopedDebugLabel()
{
	if (command_buffer)
	{
		command_buffer->get_device().get_debug_utils().cmd_end_label(command_buffer->get_handle());
	}
}

}        // namespace core
}        // namespace vkb
