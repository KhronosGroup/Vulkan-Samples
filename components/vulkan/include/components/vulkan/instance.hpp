/* Copyright (c) 2022, Arm Limited and Contributors
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

#include <cassert>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include <volk.h>

#include <components/common/stack_error.hpp>

namespace components
{
namespace vulkan
{
// Unified message severity for DebugUtils and DebugReporter
enum class DebugMessageSeverity
{
	VERBOSE,
	INFO,
	WARNING,
	ERROR
};

// Unified message types for DebugUtils and DebugReporter
enum class DebugMessageType
{
	GENERAL,
	VALIDATION,
	PERFORMANCE,
};

namespace
{
inline DebugMessageSeverity message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		return DebugMessageSeverity::ERROR;
	}
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		return DebugMessageSeverity::WARNING;
	}
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		return DebugMessageSeverity::INFO;
	}

	return DebugMessageSeverity::VERBOSE;
}

inline DebugMessageType message_type(VkDebugUtilsMessageTypeFlagsEXT type)
{
	if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
	{
		return DebugMessageType::PERFORMANCE;
	}
	else if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
	{
		return DebugMessageType::VALIDATION;
	}

	return DebugMessageType::GENERAL;
}

template <typename Func>
PFN_vkDebugUtilsMessengerCallbackEXT wrap_debug_utils(Func &&func)
{
	return [func](VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	              VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
	              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	              void *                                      pUserData) -> VkBool32 {
		if (pCallbackData)
		{
			func(message_severity(messageSeverity), message_type(messageTypes), pCallbackData->pMessage, pUserData);
		}

		return VK_FALSE;
	};
}

template <typename Func>
PFN_vkDebugReportCallbackEXT wrap_debug_report(Func &&func)
{
	return [func](
	           VkDebugReportFlagsEXT      flags,
	           VkDebugReportObjectTypeEXT objectType,
	           uint64_t                   object,
	           size_t                     location,
	           int32_t                    messageCode,
	           const char *               pLayerPrefix,
	           const char *               pMessage,
	           void *                     pUserData) -> VkBool32 {
		DebugMessageSeverity severity{DebugMessageSeverity::VERBOSE};
		DebugMessageType     type{DebugMessageType::GENERAL};

		if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		{
			severity = DebugMessageSeverity::INFO;
		}
		if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		{
			severity = DebugMessageSeverity::WARNING;
		}
		if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		{
			severity = DebugMessageSeverity::WARNING;
			type     = DebugMessageType::PERFORMANCE;
		}
		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		{
			severity = DebugMessageSeverity::ERROR;
		}
		if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		{
			severity = DebugMessageSeverity::INFO;
		}

		func(severity, type, pMessage, pUserData);

		return VK_FALSE;
	};
}
}        // namespace

struct Instance
{
	VkInstance instance_handle{VK_NULL_HANDLE};

	// Debug Callbacks
	VkDebugReportCallbackEXT debug_report_callback{VK_NULL_HANDLE};
	VkDebugUtilsMessengerEXT debug_messenger_handle{VK_NULL_HANDLE};

	~Instance()
	{
		if (debug_report_callback != VK_NULL_HANDLE)
		{
			assert(instance_handle != VK_NULL_HANDLE);
			vkDestroyDebugReportCallbackEXT(instance_handle, debug_report_callback, nullptr);
		}

		if (debug_messenger_handle != VK_NULL_HANDLE)
		{
			assert(instance_handle != VK_NULL_HANDLE);
			vkDestroyDebugUtilsMessengerEXT(instance_handle, debug_messenger_handle, nullptr);
		}

		if (instance_handle != VK_NULL_HANDLE)
		{
			vkDestroyInstance(instance_handle, nullptr);
		}
	}
};

class InstanceBuilder;

void default_instance_func(InstanceBuilder &builder);

class InstanceBuilder
{
  public:
	InstanceBuilder()  = default;
	~InstanceBuilder() = default;

	inline InstanceBuilder &set_vulkan_api_version(uint32_t major, uint32_t minor, uint32_t patch = 0, uint32_t variant = 0)
	{
		return set_vulkan_api_version(VK_MAKE_API_VERSION(variant, major, minor, patch));
	}

	InstanceBuilder &set_vulkan_api_version(uint32_t encoded_version);

	InstanceBuilder &enable_required_layer(const char *layer_name);
	InstanceBuilder &enable_optional_layer(const char *layer_name);

	InstanceBuilder &enable_required_extension(const char *extension_name);
	InstanceBuilder &enable_optional_extension(const char *extension_name);

	InstanceBuilder &enable_validation_layers();

	template <typename Func>
	InstanceBuilder &enable_debugger(Func &&func, void *user_data = nullptr)
	{
		m_debug_utils = {
		    wrap_debug_func(func),
		    user_data,
		};

		return *this;
	}

	template <typename Func>
	InstanceBuilder &apply(Func &&func)
	{
		func(*this);
		return *this;
	}

	StackErrorPtr build(Instance *o_instance) const;

  private:
	VkApplicationInfo    m_application_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	VkInstanceCreateInfo m_instance_create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

	std::set<std::string> m_required_layer_names;
	std::set<std::string> m_optional_layer_names;

	// header for all extension structs
	struct ChainedExtension
	{
		VkStructureType sType;
		void *          pNext;
	};

	std::unordered_map<std::string, std::unique_ptr<ChainedExtension>> m_required_extensions;
	std::unordered_map<std::string, std::unique_ptr<ChainedExtension>> m_optional_extensions;

	struct DebugReport
	{
		PFN_vkDebugReportCallbackEXT callback;
	};

	struct DebugUtils
	{
		PFN_vkDebugUtilsMessengerCallbackEXT callback;
		void *                               user_data;

		inline VkDebugUtilsMessengerCreateInfoEXT create_info() const
		{
			VkDebugUtilsMessengerCreateInfoEXT create_info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
			create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			create_info.pfnUserCallback = callback;
			return create_info;
		}
	};
	std::optional<DebugUtils> m_debug_utils;

	bool m_enable_validation{false};

	static std::vector<std::vector<const char *>> validation_layer_priority_list;
};
}        // namespace vulkan
}        // namespace components