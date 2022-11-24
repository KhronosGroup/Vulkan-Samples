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

#include "components/vulkan/context/context_builder.hpp"

namespace components
{
namespace vulkan
{
Context ContextBuilder::Build()
{
	VkInstance       instance = _instance_builder.Build();
	VkPhysicalDevice gpu      = _physical_device_selector.SelectPhysicalDevice(instance);
	VkDevice         device   = _device_builder.Build(gpu);
	QueueManager     queues   = _queue_builder.Build(device);

	return {
	    instance,
	    gpu,
	    device,
	    queues,
	};
}
}        // namespace vulkan
}        // namespace components
