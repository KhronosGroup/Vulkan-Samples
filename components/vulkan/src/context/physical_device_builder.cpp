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

#include "components/vulkan/context/physical_device_builder.hpp"
#include "macros.hpp"

#ifdef min
#	undef min
#endif

namespace components
{
namespace vulkan
{
PhysicalDeviceInfo PhysicalDeviceBuilder::get_device_info(VkPhysicalDevice gpu) const
{
	PhysicalDeviceInfo info;
	vkGetPhysicalDeviceFeatures(gpu, &info.features);
	vkGetPhysicalDeviceProperties(gpu, &info.properties);
	vkGetPhysicalDeviceMemoryProperties(gpu, &info.memory_properties);

	uint32_t queue_family_properties_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_properties_count, nullptr);
	std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_properties_count);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_properties_count, queue_family_properties.data());

	for (uint32_t i = 0; i < queue_family_properties_count; i++)
	{
		info.queue_families.emplace_back(QueueFamilyInfo{i, queue_family_properties[i]});
	}

	return info;
}

PhysicalDeviceBuilder::Output PhysicalDeviceBuilder::select_physical_device(VkInstance instance)
{
	uint32_t physical_device_count{0U};
	vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
	std::vector<VkPhysicalDevice> physical_devices{physical_device_count};
	vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());

	Output                highest_scoring_gpu{VK_NULL_HANDLE};
	int                   highest_score_index = -1;
	uint32_t              highest_score       = 0;
	std::vector<uint32_t> scores{physical_device_count};

	for (VkPhysicalDevice gpu : physical_devices)
	{
		PhysicalDeviceInfo gpu_info = get_device_info(gpu);

		uint32_t score{0};

		for (auto &func : _scoring_funcs)
		{
			if (func)
			{
				score += func(gpu, gpu_info);
			}
		}

		if (score > highest_score)
		{
			highest_score            = score;
			highest_score_index      = static_cast<uint32_t>(scores.size());
			highest_scoring_gpu.gpu  = gpu;
			highest_scoring_gpu.info = gpu_info;
		}

		scores.push_back(score);
	}

	return highest_scoring_gpu;
}

namespace scores
{
PhysicalDeviceBuilder::ScoringFunc combined_scoring(std::vector<PhysicalDeviceBuilder::ScoringFunc> &&funcs)
{
	return [scoring_funcs = std::move(funcs)](VkPhysicalDevice gpu, const PhysicalDeviceInfo &device_info) -> uint32_t {
		uint32_t combined_score{0};
		for (auto &func : scoring_funcs)
		{
			if (func)
			{
				combined_score += func(gpu, device_info);
			}
		}
		return combined_score;
	};
}

PhysicalDeviceBuilder::ScoringFunc require_device_type(VkPhysicalDeviceType type)
{
	return [=](VkPhysicalDevice gpu, const PhysicalDeviceInfo &device_info) -> uint32_t {
		if (device_info.properties.deviceType == type)
		{
			return PhysicalDeviceBuilder::RequiredScore;
		}
		return 0;
	};
}

PhysicalDeviceBuilder::ScoringFunc device_preference(const std::vector<VkPhysicalDeviceType> &preference_order)
{
	return [=](VkPhysicalDevice gpu, const PhysicalDeviceInfo &device_info) -> uint32_t {
		for (size_t i = 0; i < preference_order.size(); i++)
		{
			if (device_info.properties.deviceType == preference_order[i])
			{
				return PhysicalDeviceBuilder::PreferredScore * static_cast<uint32_t>(preference_order.size() - i);
			}
		}
		return 0;
	};
}

PhysicalDeviceBuilder::ScoringFunc supports_queue(const QueuePtr &queue)
{
	return [=](VkPhysicalDevice gpu, const PhysicalDeviceInfo &device_info) -> uint32_t {
		uint32_t score{0};

		for (auto &family : device_info.queue_families)
		{
			// supports queue types
			bool supports_queue_types{true};
			if (family.properties.queueFlags & queue->requested_queue_types())
			{
				score += PhysicalDeviceBuilder::GeneralScore;
			}
			else
			{
				supports_queue_types = false;
			}

			// supports surface
			bool supports_all_surfaces{true};
			for (auto &surface : queue->requested_surfaces())
			{
				VkBool32 present_supported{false};
				VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family.index, surface, &present_supported), "failed to query for presentation support");

				if (present_supported)
				{
					score += PhysicalDeviceBuilder::GeneralScore;
				}
				else
				{
					supports_all_surfaces = false;
				}
			}

			// prefer devices which support presentation to all required surfaces
			if (supports_all_surfaces)
			{
				score += PhysicalDeviceBuilder::RequiredScore;
			}
		}

		return score;
	};
}
}        // namespace scores
}        // namespace vulkan
}        // namespace components