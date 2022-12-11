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

#include <volk.h>

#include "components/vulkan/context/queue_manager.hpp"

/**
 * @brief A Vulkan Context containing the core Vulkan handles needed for a Sample
 *
 * 		  This can be passed to higher level components
 */
class Context
{
  public:
	Context(VkInstance          instance,
	        VkPhysicalDevice    gpu,
	        VkDevice            device,
	        const QueueManager &queues) :
	    instance{instance},
	    gpu{gpu},
	    device{device},
	    queues{queues}
	{}
	~Context() = default;

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

	VkInstance       instance{VK_NULL_HANDLE};
	VkPhysicalDevice gpu{VK_NULL_HANDLE};
	VkDevice         device{VK_NULL_HANDLE};
	QueueManager     queues{};

  private:
	DebuggerInfo debugger_info{};
};
}        // namespace vulkan
}        // namespace components
