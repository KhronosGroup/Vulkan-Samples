/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
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

#include "common/optional.h"
#include <common/hpp_error.h>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace vkb
{
namespace core
{
class HPPPhysicalDevice;
/**
 * @brief Returns a list of Khronos/LunarG supported validation layers
 *        Attempting to enable them in order of preference, starting with later Vulkan SDK versions
 * @param supported_instance_layers A list of validation layers to check against
 */
std::vector<const char *> get_optimal_validation_layers(const std::vector<vk::LayerProperties> &supported_instance_layers);

/**
 * @brief A wrapper class for vk::Instance
 *
 * This class is responsible for initializing the dispatcher, enumerating over all available extensions and validation layers
 * enabling them if they exist, setting up debug messaging and querying all the physical devices existing on the machine.
 */
class HPPInstance
{
  public:
	/**
	 * @brief Can be set from the GPU selection plugin to explicitly select a GPU instead
	 */
	static Optional<uint32_t> selected_gpu_index;

	/**
	 * @brief Initializes the connection to Vulkan
	 * @param application_name The name of the application
	 * @param required_extensions The extensions requested to be enabled
	 * @param required_validation_layers The validation layers to be enabled
	 * @param required_layer_settings The layer settings to be enabled
	 * @param headless Whether the application is requesting a headless setup or not
	 * @param api_version The Vulkan API version that the instance will be using
	 * @throws runtime_error if the required extensions and validation layers are not found
	 */
	HPPInstance(const std::string                            &application_name,
	            const std::unordered_map<const char *, bool> &required_extensions        = {},
	            const std::vector<const char *>              &required_validation_layers = {},
#if defined(VK_EXT_layer_settings)
				const std::vector<VkLayerSettingEXT>		  &required_layer_settings	 = {},
#endif
	            bool                                          headless                   = false,
	            uint32_t                                      api_version                = VK_API_VERSION_1_0);

	/**
	 * @brief Queries the GPUs of a vk::Instance that is already created
	 * @param instance A valid vk::Instance
	 */
	HPPInstance(vk::Instance instance);

	HPPInstance(const HPPInstance &) = delete;

	HPPInstance(HPPInstance &&) = delete;

	~HPPInstance();

	HPPInstance &operator=(const HPPInstance &) = delete;

	HPPInstance &operator=(HPPInstance &&) = delete;

	const std::vector<const char *> &get_extensions();

	/**
	 * @brief Tries to find the first available discrete GPU
	 * @returns A valid physical device
	 */
	HPPPhysicalDevice &get_first_gpu();

	vk::Instance get_handle() const;

	/**
	 * @brief Tries to find the first available discrete GPU that can render to the given surface
	 * @param surface to test against
	 * @returns A valid physical device
	 */
	HPPPhysicalDevice &get_suitable_gpu(vk::SurfaceKHR);

	/**
	 * @brief Checks if the given extension is enabled in the vk::Instance
	 * @param extension An extension to check
	 */
	bool is_enabled(const char *extension) const;

  private:
	/**
	 * @brief Queries the instance for the physical devices on the machine
	 */
	void query_gpus();

  private:
	/**
	 * @brief The Vulkan instance
	 */
	vk::Instance handle;

	/**
	 * @brief The enabled extensions
	 */
	std::vector<const char *> enabled_extensions;

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
	/**
	 * @brief Debug utils messenger callback for VK_EXT_Debug_Utils
	 */
	vk::DebugUtilsMessengerEXT debug_utils_messenger;

	/**
	 * @brief The debug report callback
	 */
	vk::DebugReportCallbackEXT debug_report_callback;
#endif

	/**
	 * @brief The physical devices found on the machine
	 */
	std::vector<std::unique_ptr<HPPPhysicalDevice>> gpus;
};
}        // namespace core
}        // namespace vkb
