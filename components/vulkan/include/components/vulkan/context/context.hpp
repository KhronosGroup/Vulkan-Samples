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

#include <vector>
#include <memory>

#include <volk.h>

#include "queue_manager.hpp"

namespace components
{
namespace vulkan
{

inline VkResult init_meta_loader()
{
	return volkInitialize();
}

// stores logger handles
struct DebuggerInfo
{
	VkDebugUtilsMessengerEXT debug_utils_messenger{VK_NULL_HANDLE};
	VkDebugReportCallbackEXT debug_report_callback{VK_NULL_HANDLE};
};

// represents the capabilities of a queue family
struct QueueFamilyInfo
{
	uint32_t                index;
	VkQueueFamilyProperties properties;
};

// represents the capabilities of a gpu
struct PhysicalDeviceInfo
{
	VkPhysicalDeviceFeatures         features;
	VkPhysicalDeviceProperties       properties;
	VkPhysicalDeviceMemoryProperties memory_properties;
	std::vector<QueueFamilyInfo>     queue_families;
};

// represents the allocated queue at vkCreateDevice
struct QueueFamily
{
	uint32_t family_index;

	uint32_t graphics_queue_count{0};
	uint32_t compute_queue_count{0};
	uint32_t transfer_queue_count{0};

	uint32_t total_supported_queues_count{0};
	bool     supports_presentation{false};
};

/**
 * @brief A Vulkan Context containing the core Vulkan handles needed for a Sample
 *
 * 		  This can be passed to higher level components
 */
class Context
{
  public:
	Context(VkInstance                instance,
	        DebuggerInfo              debugger_info,
	        VkPhysicalDevice          gpu,
	        const PhysicalDeviceInfo &device_info,
	        VkDevice                  device,
	        const QueueManager       &queues) :
	    instance{instance},
	    debugger_info{debugger_info},
	    gpu{gpu},
	    device{device},
	    queues{queues},
	    device_info{device_info}
	{}

	~Context()
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(device, nullptr);
		}

		if (debugger_info.debug_utils_messenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(instance, debugger_info.debug_utils_messenger, nullptr);
		}
		if (debugger_info.debug_report_callback != VK_NULL_HANDLE)
		{
			vkDestroyDebugReportCallbackEXT(instance, debugger_info.debug_report_callback, nullptr);
		}

		if (instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(instance, nullptr);
		}
	}

	VkInstance instance{VK_NULL_HANDLE};

	VkPhysicalDevice   gpu{VK_NULL_HANDLE};
	PhysicalDeviceInfo device_info;

	VkDevice device{VK_NULL_HANDLE};

	QueueManager queues;

  private:
	DebuggerInfo debugger_info{};
};

using ContextPtr = std::shared_ptr<Context>;
}        // namespace vulkan
}        // namespace components