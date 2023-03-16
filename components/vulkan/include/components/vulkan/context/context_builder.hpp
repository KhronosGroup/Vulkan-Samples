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

#include "device_builder.hpp"
#include "queue.hpp"

namespace components
{
namespace vulkan
{
InstanceBuilder::ApplicationInfoFunc default_application_info(uint32_t api_version = VK_API_VERSION_1_3);

/**
 * @brief A builder to construct a Context Object
 *
 * 		  Allows a user to configure each part of the context using nested builders
 * 		  A nested builder can be completed by calling `ContextBuilder& Done()`
 *		  Calling `Context Build()` orchestrates the build process of the context using the configured builders
 */
class ContextBuilder final
{
	using Self        = ContextBuilder;
	using BuilderFunc = std::function<void(Self &)>;

  public:
	ContextBuilder()  = default;
	~ContextBuilder() = default;

	CVC_APPLY_BUILDER_FUNC()

	inline InstanceBuilder &configure_instance()
	{
		return _instance_builder;
	}

	ContextBuilder &request_queue(VkQueueFlags queue_types, const std::vector<VkSurfaceKHR> &presentable_surfaces, QueuePtr *queue);

	inline PhysicalDeviceBuilder &select_gpu()
	{
		return _physical_device_selector;
	}

	inline DeviceBuilder &configure_device()
	{
		return _device_builder;
	}

	/**
	 * @brief Orchestrates the building of a context by calling the nested build functions for each component
	 *
	 * @return Context The built context
	 */
	ContextPtr build();

  private:
	InstanceBuilder       _instance_builder{*this};
	PhysicalDeviceBuilder _physical_device_selector{*this};
	DeviceBuilder         _device_builder{*this};

	std::vector<QueuePtr> _requested_queues;
};
}        // namespace vulkan
}        // namespace components