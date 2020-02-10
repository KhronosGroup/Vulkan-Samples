/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "core/physical_device.h"

namespace vkb
{
/**
 * @brief Returns a list of Khronos/LunarG supported validation layers
 *        Attempting to enable them in order of preference, starting with later Vulkan SDK versions
 * @param supported_instance_layers A list of validation layers to check against
 */
std::vector<const char *> get_optimal_validation_layers(const std::vector<VkLayerProperties> &supported_instance_layers);

/**
 * @brief A wrapper class for VkInstance
 *
 * This class is responsible for initializing volk, enumerating over all available extensions and validation layers
 * enabling them if they exist, setting up debug messaging and querying all the physical devices existing on the machine.
 */
class Instance
{
  public:
	/**
	 * @brief Initializes the connection to Vulkan
	 * @param application_name The name of the application
	 * @param required_extensions The extensions requested to be enabled
	 * @param required_validation_layers The validation layers to be enabled
	 * @param headless Whether the application is requesting a headless setup or not
	 * @throws runtime_error if the required extensions and validation layers are not found
	 */
	Instance(const std::string &                           application_name,
	         const std::unordered_map<const char *, bool> &required_extensions        = {},
	         const std::vector<const char *> &             required_validation_layers = {},
	         bool                                          headless                   = false);

	/**
	 * @brief Queries the GPUs of a VkInstance that is already created
	 * @param instance A valid VkInstance
	 */
	Instance(VkInstance instance);

	Instance(const Instance &) = delete;

	Instance(Instance &&) = delete;

	~Instance();

	Instance &operator=(const Instance &) = delete;

	Instance &operator=(Instance &&) = delete;

	/**
	 * @brief Queries the instance for the physical devices on the machine
	 */
	void query_gpus();

	/**
	 * @brief Tries to find the first available discrete GPU
	 * @returns A valid physical device
	 */
	PhysicalDevice &get_suitable_gpu();

	/**
	 * @brief Checks if the given extension is enabled in the VkInstance
	 * @param extension An extension to check
	 */
	bool is_enabled(const char *extension) const;

	VkInstance get_handle();

	const std::vector<const char *> &get_extensions();

  private:
	/**
	 * @brief The Vulkan instance
	 */
	VkInstance handle{VK_NULL_HANDLE};

	/**
	 * @brief The enabled extensions
	 */
	std::vector<const char *> enabled_extensions;

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	/**
	 * @brief The debug report callback
	 */
	VkDebugReportCallbackEXT debug_report_callback{VK_NULL_HANDLE};
#endif

	/**
	 * @brief The physical devices found on the machine
	 */
	std::vector<std::unique_ptr<PhysicalDevice>> gpus;
};        // namespace Instance
}        // namespace vkb
