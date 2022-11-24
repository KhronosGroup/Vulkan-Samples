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

#include "context.hpp"
#include "device_builder.hpp"
#include "instance_builder.hpp"
#include "physical_device_builder.hpp"
#include "queue_builder.hpp"

class ContextBuilder final
{
	using Self = ContextBuilder;

  public:
	ContextBuilder()  = default;
	~ContextBuilder() = default;

	InstanceBuilder       &ConfigureInstance();
	PhysicalDeviceBuilder &SelectGPU();
	DeviceBuilder         &ConfigureDevice();
	QueueBuilder          &ConfigureQueues();

	Context Build()
	{
		_instance_builder.Build();
	}

  private:
	InstanceBuilder _instance_builder;
    PhysicalDeviceBuilder _phys
};