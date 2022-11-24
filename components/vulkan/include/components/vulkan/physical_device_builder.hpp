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

#include <volk/volk.h>

#include <functional>
#include <vector>

class ContextBuilder;

class PhysicalDeviceBuilder final
{
	using Self = PhysicalDeviceBuilder;

  public:
	PhysicalDeviceBuilder(ContextBuilder &parent) :
	    _parent{parent}
	{}

	~PhysicalDeviceBuilder() = default;

	Self &AcceptType(VkPhysicalDeviceType type);

	using FeatureFilter = std::function<void(const VkPhysicalDeviceFeatures &)>;
	Self &ApplyFeatureFilter(FeatureFilter &&filter);

	using Feature2Filter = std::function<void(const VkPhysicalDeviceFeatures2 &)>;
	Self &ApplyFeature2Filter(FeatureFilter &&filter);

	using LimitFilter = std::function<void(const VkPhysicalDeviceLimits &)>;
	Self &ApplyLimitFilter(LimitFilter &&filter);

	using CustomFilter = std::function<void(VkPhysicalDevice)>;
	Self &ApplyCustomFilter(CustomFilter &&filter);

	ContextBuilder &Done();

  private:
	VkPhysicalDevice SelectPhysicalDevice(VkInstance instance);

	ContextBuilder &_parent;

	std::vector<VkPhysicalDeviceType> _acceptable_device_types;
	std::vector<FeatureFilter>        _feature_filters;
	std::vector<Feature2Filter>       _feature2_filters;
	std::vector<LimitFilter>          _limit_filters;
	std::vector<CustomFilter>         _custom_filters;
};