/* Copyright (c) 2020, Arm Limited and Contributors
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

#include "16bit_storage_input_output.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include <random>

KHR16BitStorageInputOutputSample::KHR16BitStorageInputOutputSample()
{
	// For enabling 16-bit storage device extensions.
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true);

	// Will be used in vertex and fragment shaders to declare varying data as FP16 rather than FP32.
	// This significantly reduces bandwidth as varyings are stored in main memory on TBDR architectures.
	// On Vulkan 1.1, this extension is in core, but just enable the extension in case we are running on a Vulkan 1.0 implementation.
	add_device_extension(VK_KHR_16BIT_STORAGE_EXTENSION_NAME, true);
	// 16-bit storage depends on this extension as well.
	add_device_extension(VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME, true);

	auto &config = get_configuration();

	config.insert<vkb::BoolSetting>(0, khr_16bit_storage_input_output_enabled, false);
	config.insert<vkb::BoolSetting>(1, khr_16bit_storage_input_output_enabled, true);
}

void KHR16BitStorageInputOutputSample::setup_scene()
{
	load_scene("scenes/teapot.gltf");

	// Setup the scene so we have many teapots.
	vkb::sg::Mesh *teapot_mesh = nullptr;

	// Override the default material so it's not rendering all black.
	auto materials = scene->get_components<vkb::sg::PBRMaterial>();
	for (auto *material : materials)
	{
		material->base_color_factor = glm::vec4(0.8f, 0.6f, 0.5f, 1.0f);
		material->roughness_factor  = 1.0f;
		material->metallic_factor   = 0.0f;
	}

	std::default_random_engine            rng(42);        // Use a fixed seed, makes rendering deterministic from run to run.
	std::uniform_real_distribution<float> float_distribution{-1.0f, 1.0f};
	const auto                            get_random_axis = [&]() -> glm::vec3 {
        glm::vec3 axis;

        for (unsigned i = 0; i < 3; i++)
        {
            axis[i] = float_distribution(rng);
        }

        if (glm::all(glm::equal(axis, glm::vec3(0.0f))))
        {
            axis = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            axis = glm::normalize(axis);
        }

        return axis;
	};

	const auto get_random_angular_freq = [&]() -> float {
		return 1.0f + 0.2f * float_distribution(rng);
	};

	auto &root_node = scene->get_root_node();
	for (auto *child : root_node.get_children())
	{
		if (child->get_name() == "Teapot")
		{
			teapot_mesh = &child->get_component<vkb::sg::Mesh>();

			auto &transform = child->get_component<vkb::sg::Transform>();
			transform.set_scale(glm::vec3(1.0f));
			transform.set_translation(glm::vec3(-40.0f, -20.0f, 0.0f));
			transform.set_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

			Transform teapot;
			teapot.transform         = &transform;
			teapot.axis              = get_random_axis();
			teapot.angular_frequency = get_random_angular_freq();
			teapot_transforms.push_back(teapot);
		}
	}

	if (!teapot_mesh)
	{
		throw std::runtime_error("Teapot mesh does not exist in teapot.gltf?");
	}

	// Duplicate out a lot of unique nodes so that we can render the teapot many times.
	for (int y = -20; y <= 20; y += 5)
	{
		for (int x = -40; x <= 40; x += 5)
		{
			// We already have this teapot.
			if (x == -40 && y == -20)
			{
				continue;
			}

			auto node = std::make_unique<vkb::sg::Node>(-1, "Teapot");
			node->set_component(*teapot_mesh);
			teapot_mesh->add_node(*node);

			auto &transform = node->get_component<vkb::sg::Transform>();
			transform.set_scale(glm::vec3(1.0f));
			transform.set_translation(glm::vec3(x, y, 0.0f));
			transform.set_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

			Transform teapot;
			teapot.transform         = &transform;
			teapot.axis              = get_random_axis();
			teapot.angular_frequency = get_random_angular_freq();
			teapot_transforms.push_back(teapot);

			root_node.add_child(*node);
			scene->add_node(std::move(node));
		}
	}
}

void KHR16BitStorageInputOutputSample::update_pipeline()
{
	const char *vertex_path;
	const char *fragment_path;

	const std::string base_path = "16bit_storage_input_output/";
	if (khr_16bit_storage_input_output_enabled && supports_16bit_storage)
	{
		vertex_path   = "16bit_storage_input_output_enabled.vert";
		fragment_path = "16bit_storage_input_output_enabled.frag";
	}
	else
	{
		vertex_path   = "16bit_storage_input_output_disabled.vert";
		fragment_path = "16bit_storage_input_output_disabled.frag";
	}

	vkb::ShaderSource vert_shader(base_path + vertex_path);
	vkb::ShaderSource frag_shader(base_path + fragment_path);
	auto              scene_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), *scene, *camera);

	auto render_pipeline = vkb::RenderPipeline();
	render_pipeline.add_subpass(std::move(scene_subpass));

	set_render_pipeline(std::move(render_pipeline));
}

bool KHR16BitStorageInputOutputSample::prepare(vkb::Platform &platform)
{
	if (!VulkanSample::prepare(platform))
	{
		return false;
	}

	setup_scene();

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	auto &camera_transform = camera->get_node()->get_component<vkb::sg::Transform>();
	camera_transform.set_translation(glm::vec3(0.0f, 0.0f, 60.0f));
	camera_transform.set_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

	update_pipeline();

	stats->request_stats({vkb::StatIndex::gpu_ext_read_bytes, vkb::StatIndex::gpu_ext_write_bytes});

	gui = std::make_unique<vkb::Gui>(*this, platform.get_window(), stats.get());

	return true;
}

void KHR16BitStorageInputOutputSample::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	auto &features_16bit_storage = gpu.request_extension_features<VkPhysicalDevice16BitStorageFeatures>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES);
	supports_16bit_storage       = features_16bit_storage.storageInputOutput16 == VK_TRUE;
}

void KHR16BitStorageInputOutputSample::update(float delta_time)
{
	if (khr_16bit_storage_input_output_enabled != khr_16bit_storage_input_output_last_enabled)
	{
		update_pipeline();
		khr_16bit_storage_input_output_last_enabled = khr_16bit_storage_input_output_enabled;
	}

	float angular_freq = 1.0f;
	for (auto &teapot : teapot_transforms)
	{
		glm::quat rotation = teapot.transform->get_rotation();
		rotation           = glm::normalize(glm::angleAxis(teapot.angular_frequency * delta_time, teapot.axis) * rotation);
		teapot.transform->set_rotation(rotation);
		angular_freq += 0.05f;
	}

	VulkanSample::update(delta_time);
}

void KHR16BitStorageInputOutputSample::draw_gui()
{
	const char *label;
	if (supports_16bit_storage)
	{
		label = "Enable 16-bit InputOutput";
	}
	else
	{
		label = "Enable 16-bit InputOutput (noop - unsupported by device)";
	}

	gui->show_options_window(
	    /* body = */ [this, label]() {
		    ImGui::Checkbox(label, &khr_16bit_storage_input_output_enabled);
	    },
	    /* lines = */ 1);
}

void KHR16BitStorageInputOutputSample::recreate_swapchain()
{
	std::set<VkImageUsageFlagBits> image_usage_flags = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

	get_device().wait_idle();

	get_render_context().update_swapchain(image_usage_flags);
}

std::unique_ptr<vkb::VulkanSample> create_16bit_storage_input_output()
{
	return std::make_unique<KHR16BitStorageInputOutputSample>();
}
