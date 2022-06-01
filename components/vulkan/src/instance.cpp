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

#include "components/vulkan/instance.hpp"

#include <vector>

namespace components
{
namespace vulkan
{
void default_instance_func(InstanceBuilder &builder)
{
	builder
	    .enable_validation_layers();
}

std::vector<std::vector<const char *>> InstanceBuilder::validation_layer_priority_list =
    {
        // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
        {"VK_LAYER_KHRONOS_validation"},

        // Otherwise we fallback to using the LunarG meta layer
        {"VK_LAYER_LUNARG_standard_validation"},

        // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
        {
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_GOOGLE_unique_objects",
        },

        // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
        {"VK_LAYER_LUNARG_core_validation"},
};

InstanceBuilder &InstanceBuilder::set_vulkan_api_version(uint32_t encoded_version)
{
	m_application_info.apiVersion = encoded_version;
	return *this;
}

InstanceBuilder &InstanceBuilder::enable_required_layer(const char *layer_name)
{
	m_required_layer_names.emplace(layer_name);
	return *this;
}

InstanceBuilder &InstanceBuilder::enable_optional_layer(const char *layer_name)
{
	m_optional_layer_names.emplace(layer_name);
	return *this;
}

InstanceBuilder &InstanceBuilder::enable_required_extension(const char *extension_name)
{
	m_required_extensions[std::string{extension_name}] = nullptr;
	return *this;
}

InstanceBuilder &InstanceBuilder::enable_optional_extension(const char *extension_name)
{
	m_optional_extensions[std::string{extension_name}] = nullptr;
	return *this;
}

InstanceBuilder &InstanceBuilder::enable_validation_layers()
{
	m_enable_validation = true;
	return *this;
}

#define APPEND_ERROR(msg, file, line)                  \
	{                                                  \
		if (err == nullptr)                            \
		{                                              \
			err = StackError::unique(msg, file, line); \
		}                                              \
		else                                           \
		{                                              \
			err->push(msg, file, line);                \
		}                                              \
	}

StackErrorPtr InstanceBuilder::build(Instance *o_instance) const
{
	StackErrorPtr err{nullptr};

	// populate application info
	VkApplicationInfo application_info = m_application_info;
	application_info.engineVersion     = VK_MAKE_API_VERSION(0, 1, 0, 0);
	application_info.pEngineName       = "vulkan_samples";
	application_info.pApplicationName  = "vulkan_application";

	// enumerate instance layers
	uint32_t available_layer_count;
	vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers;
	available_layers.resize(available_layer_count);
	vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data());

	std::set<std::string> enabled_layers;

	// enable required layers
	for (auto &layer_name : m_required_layer_names)
	{
		bool found = false;

		for (auto &layer_properties : available_layers)
		{
			if (layer_name == layer_properties.layerName)
			{
				// layer available
				enabled_layers.emplace(layer_name);
				found = true;
				break;
			}
		}

		if (found)
		{
			continue;
		}

		// layer not found
		APPEND_ERROR("required layer not supported", "vulkan/instance.cpp", __LINE__);
	}

	// enable optional layers
	for (auto &layer_name : m_optional_layer_names)
	{
		for (auto &layer_properties : available_layers)
		{
			if (layer_name == layer_properties.layerName)
			{
				// layer available
				enabled_layers.emplace(layer_name);
				break;
			}
		}
	}

	// populate instance create info
	std::vector<const char *> enable_layers_c_str;
	enable_layers_c_str.reserve(enabled_layers.size());

	for (auto &layer_name : enabled_layers)
	{
		enable_layers_c_str.push_back(layer_name.c_str());
	}

	VkInstanceCreateInfo instance_create_info = m_instance_create_info;
	instance_create_info.pApplicationInfo     = &application_info;
	instance_create_info.enabledLayerCount    = enable_layers_c_str.size();
	instance_create_info.ppEnabledLayerNames  = enable_layers_c_str.data();

	// return if a config error ocurred
	if (err)
	{
		return std::move(err);
	}

	// create a new instance
	Instance instance;
	auto     result = vkCreateInstance(&instance_create_info, nullptr, &instance.instance_handle);
	if (result != VK_SUCCESS)
	{
		return StackError::unique("failed to create instance", "vulkan/instance.cpp", __LINE__);
	}

	*o_instance = instance;

	return nullptr;
}

}        // namespace vulkan
}        // namespace components