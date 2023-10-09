/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "oit_linked_lists.h"

OITLinkedLists::OITLinkedLists()
{
}

OITLinkedLists::~OITLinkedLists()
{
	if (!device)
	{
		return;
	}

	vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, VK_NULL_HANDLE);
	object_desc.reset();
	scene_constants.reset();
	object.reset();
}

bool OITLinkedLists::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}
	
	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 0.0f, -4.0f});
	camera.set_rotation({0.0f, 180.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	load_assets();
	prepare_buffers();
	create_descriptor_pool();

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("oit_linked_lists/gather.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("oit_linked_lists/gather.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	update_scene_constants();
	fill_object_data();

	return true;
}

void OITLinkedLists::load_assets()
{
	object = load_model("scenes/geosphere.gltf");
}

void OITLinkedLists::prepare_buffers()
{
	scene_constants = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(SceneConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	object_desc = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ObjectDesc) * kObjectCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void OITLinkedLists::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
	};
	const uint32_t num_descriptor_sets = 1;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void OITLinkedLists::request_gpu_features(vkb::PhysicalDevice &gpu)
{
}

void OITLinkedLists::build_command_buffers()
{
}

void OITLinkedLists::update_scene_constants()
{
	SceneConstants constants;
	constants.projection       = camera.matrices.perspective;
	constants.view             = camera.matrices.view;
	scene_constants->convert_and_update(constants);
}

void OITLinkedLists::fill_object_data()
{
	ObjectDesc desc[kObjectCount] = {};

	auto get_random_float = []()
	{
		return static_cast<float>(rand()) / (RAND_MAX);
	};

	for (uint32_t l = 0; l < kObjectLayerCount; ++l)
	{
		for (uint32_t c = 0; c < kObjectColumnCount; ++c)
		{
			for (uint32_t r = 0; r < kObjectRowCount; ++r)
			{
				const uint32_t object_index =
					(l * kObjectColumnCount * kObjectRowCount) +
					(c * kObjectRowCount) + r;

				const float x = static_cast<float>(r) - ((kObjectRowCount - 1) * 0.5f);
				const float y = static_cast<float>(c) - ((kObjectColumnCount - 1) * 0.5f);
				const float z = static_cast<float>(l) - ((kObjectLayerCount - 1) * 0.5f);
				const float scale = 0.95f;
				desc[object_index].model =
					glm::scale(
						glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)),
						glm::vec3(scale));

				desc[object_index].color.r = get_random_float();
				desc[object_index].color.g = get_random_float();
				desc[object_index].color.b = get_random_float();
				desc[object_index].color.a = get_random_float() * 0.5f + 0.10f;
			}
		}
	}

	object_desc->convert_and_update(desc);
}

void OITLinkedLists::draw()
{
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void OITLinkedLists::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (camera.updated)
	{
		update_scene_constants();
	}
}

std::unique_ptr<vkb::VulkanSample> create_oit_linked_lists()
{
	return std::make_unique<OITLinkedLists>();
}
