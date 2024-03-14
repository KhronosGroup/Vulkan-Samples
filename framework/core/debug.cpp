/* Copyright (c) 2021, Arm Limited and Contributors
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

#include "core/command_buffer.h"
#include "core/device.h"

#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>

namespace vkb
{
void DebugUtilsExtDebugUtils::set_debug_name(VkDevice device, VkObjectType object_type, uint64_t object_handle,
                                             const char *name) const
{
	VkDebugUtilsObjectNameInfoEXT name_info{};
	name_info.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	name_info.objectType   = object_type;
	name_info.objectHandle = object_handle;
	name_info.pObjectName  = name;

	assert(vkSetDebugUtilsObjectNameEXT);
	vkSetDebugUtilsObjectNameEXT(device, &name_info);
}

void DebugUtilsExtDebugUtils::set_debug_tag(VkDevice device, VkObjectType object_type, uint64_t object_handle,
                                            uint64_t tag_name, const void *tag_data, size_t tag_data_size) const
{
	VkDebugUtilsObjectTagInfoEXT tag_info{};
	tag_info.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
	tag_info.objectType   = object_type;
	tag_info.objectHandle = object_handle;
	tag_info.tagName      = tag_name;
	tag_info.tagSize      = tag_data_size;
	tag_info.pTag         = tag_data;

	assert(vkSetDebugUtilsObjectTagEXT);
	vkSetDebugUtilsObjectTagEXT(device, &tag_info);
}

void DebugUtilsExtDebugUtils::cmd_begin_label(VkCommandBuffer command_buffer,
                                              const char *name, glm::vec4 color) const
{
	VkDebugUtilsLabelEXT label_info{};
	label_info.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label_info.pLabelName = name;
	memcpy(label_info.color, glm::value_ptr(color), sizeof(glm::vec4));

	assert(vkCmdBeginDebugUtilsLabelEXT);
	vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label_info);
}

void DebugUtilsExtDebugUtils::cmd_end_label(VkCommandBuffer command_buffer) const
{
	assert(vkCmdEndDebugUtilsLabelEXT);
	vkCmdEndDebugUtilsLabelEXT(command_buffer);
}

void DebugUtilsExtDebugUtils::cmd_insert_label(VkCommandBuffer command_buffer,
                                               const char *name, glm::vec4 color) const
{
	VkDebugUtilsLabelEXT label_info{};
	label_info.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label_info.pLabelName = name;
	memcpy(label_info.color, glm::value_ptr(color), sizeof(glm::vec4));

	assert(vkCmdInsertDebugUtilsLabelEXT);
	vkCmdInsertDebugUtilsLabelEXT(command_buffer, &label_info);
}

// See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkDebugReportObjectTypeEXT.html
static const std::unordered_map<VkObjectType, VkDebugReportObjectTypeEXT> VK_OBJECT_TYPE_TO_DEBUG_REPORT_TYPE{
    {VK_OBJECT_TYPE_UNKNOWN, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT},
    {VK_OBJECT_TYPE_INSTANCE, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT},
    {VK_OBJECT_TYPE_PHYSICAL_DEVICE, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT},
    {VK_OBJECT_TYPE_DEVICE, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT},
    {VK_OBJECT_TYPE_QUEUE, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT},
    {VK_OBJECT_TYPE_SEMAPHORE, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT},
    {VK_OBJECT_TYPE_COMMAND_BUFFER, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT},
    {VK_OBJECT_TYPE_FENCE, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT},
    {VK_OBJECT_TYPE_DEVICE_MEMORY, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT},
    {VK_OBJECT_TYPE_BUFFER, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT},
    {VK_OBJECT_TYPE_IMAGE, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT},
    {VK_OBJECT_TYPE_EVENT, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT},
    {VK_OBJECT_TYPE_QUERY_POOL, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT},
    {VK_OBJECT_TYPE_BUFFER_VIEW, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT},
    {VK_OBJECT_TYPE_IMAGE_VIEW, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT},
    {VK_OBJECT_TYPE_SHADER_MODULE, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT},
    {VK_OBJECT_TYPE_PIPELINE_CACHE, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT},
    {VK_OBJECT_TYPE_PIPELINE_LAYOUT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT},
    {VK_OBJECT_TYPE_RENDER_PASS, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT},
    {VK_OBJECT_TYPE_PIPELINE, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT},
    {VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT},
    {VK_OBJECT_TYPE_SAMPLER, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT},
    {VK_OBJECT_TYPE_DESCRIPTOR_POOL, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT},
    {VK_OBJECT_TYPE_DESCRIPTOR_SET, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT},
    {VK_OBJECT_TYPE_FRAMEBUFFER, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT},
    {VK_OBJECT_TYPE_COMMAND_POOL, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT},
    {VK_OBJECT_TYPE_SURFACE_KHR, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT},
    {VK_OBJECT_TYPE_SWAPCHAIN_KHR, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT},
    {VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT},
    {VK_OBJECT_TYPE_DISPLAY_KHR, VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT},
    {VK_OBJECT_TYPE_DISPLAY_MODE_KHR, VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT},
    {VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT},
};

void DebugMarkerExtDebugUtils::set_debug_name(VkDevice device, VkObjectType object_type, uint64_t object_handle,
                                              const char *name) const
{
	VkDebugMarkerObjectNameInfoEXT name_info{};
	name_info.sType       = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
	name_info.objectType  = VK_OBJECT_TYPE_TO_DEBUG_REPORT_TYPE.at(object_type);
	name_info.object      = object_handle;
	name_info.pObjectName = name;

	assert(vkDebugMarkerSetObjectNameEXT);
	vkDebugMarkerSetObjectNameEXT(device, &name_info);
}

void DebugMarkerExtDebugUtils::set_debug_tag(VkDevice device, VkObjectType object_type, uint64_t object_handle,
                                             uint64_t tag_name, const void *tag_data, size_t tag_data_size) const
{
	VkDebugMarkerObjectTagInfoEXT tag_info{};
	tag_info.sType      = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
	tag_info.objectType = VK_OBJECT_TYPE_TO_DEBUG_REPORT_TYPE.at(object_type);
	tag_info.object     = object_handle;
	tag_info.tagName    = tag_name;
	tag_info.tagSize    = tag_data_size;
	tag_info.pTag       = tag_data;

	assert(vkDebugMarkerSetObjectTagEXT);
	vkDebugMarkerSetObjectTagEXT(device, &tag_info);
}

void DebugMarkerExtDebugUtils::cmd_begin_label(VkCommandBuffer command_buffer,
                                               const char *name, glm::vec4 color) const
{
	VkDebugMarkerMarkerInfoEXT marker_info{};
	marker_info.sType       = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
	marker_info.pMarkerName = name;
	memcpy(marker_info.color, glm::value_ptr(color), sizeof(glm::vec4));

	assert(vkCmdDebugMarkerBeginEXT);
	vkCmdDebugMarkerBeginEXT(command_buffer, &marker_info);
}

void DebugMarkerExtDebugUtils::cmd_end_label(VkCommandBuffer command_buffer) const
{
	assert(vkCmdDebugMarkerEndEXT);
	vkCmdDebugMarkerEndEXT(command_buffer);
}

void DebugMarkerExtDebugUtils::cmd_insert_label(VkCommandBuffer command_buffer,
                                                const char *name, glm::vec4 color) const
{
	VkDebugMarkerMarkerInfoEXT marker_info{};
	marker_info.sType       = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
	marker_info.pMarkerName = name;
	memcpy(marker_info.color, glm::value_ptr(color), sizeof(glm::vec4));

	assert(vkCmdDebugMarkerInsertEXT);
	vkCmdDebugMarkerInsertEXT(command_buffer, &marker_info);
}

ScopedDebugLabel::ScopedDebugLabel(const DebugUtils &debug_utils, VkCommandBuffer command_buffer,
                                   const char *name, glm::vec4 color) :
    debug_utils{&debug_utils},
    command_buffer{VK_NULL_HANDLE}
{
	if (name && *name != '\0')
	{
		assert(command_buffer != VK_NULL_HANDLE);
		this->command_buffer = command_buffer;

		debug_utils.cmd_begin_label(command_buffer, name, color);
	}
}

ScopedDebugLabel::ScopedDebugLabel(const vkb::core::CommandBuffer<vkb::BindingType::C> &command_buffer, const char *name, glm::vec4 color) :
    ScopedDebugLabel{command_buffer.get_device().get_debug_utils(), command_buffer.get_handle(), name, color}
{
}

ScopedDebugLabel::~ScopedDebugLabel()
{
	if (command_buffer != VK_NULL_HANDLE)
	{
		debug_utils->cmd_end_label(command_buffer);
	}
}

}        // namespace vkb