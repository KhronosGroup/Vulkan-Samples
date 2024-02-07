/* Copyright (c) 2019-2023, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "constant_data.h"

#include "common/vk_common.h"
#include "gltf_loader.h"
#include "gui.h"
#include "filesystem/legacy.h"

#include "rendering/pipeline_state.h"
#include "rendering/render_context.h"
#include "rendering/render_pipeline.h"
#include "rendering/subpasses/forward_subpass.h"
#include "rendering/subpasses/geometry_subpass.h"
#include "rendering/subpasses/lighting_subpass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/components/transform.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"
#include "stats/stats.h"

namespace
{
/**
 * @brief Helper function to fill the contents of the MVPUniform struct with the transform of the node and the camera view-projection matrix.
 */
inline MVPUniform fill_mvp(vkb::sg::Node &node, vkb::sg::Camera &camera)
{
	MVPUniform mvp;

	auto &transform = node.get_transform();

	mvp.model = transform.get_world_matrix();

	mvp.camera_view_proj = vkb::vulkan_style_projection(camera.get_projection()) * camera.get_view();

	mvp.scale = glm::mat4(1.0f);

	mvp.padding = glm::mat4(1.0f);

	return mvp;
}
}        // namespace

ConstantData::ConstantData()
{
	auto &config = get_configuration();

	// Set all types as a configuration, the sample should no-op on the types that aren't supported (i.e. update after binds)
	for (size_t i = 0; i < methods.size(); ++i)
	{
		config.insert<vkb::IntSetting>(vkb::to_u32(i), gui_method_value, vkb::to_u32(i));
	}

	// Request sample-specific extensions as optional
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true);
	add_device_extension(VK_KHR_MAINTENANCE3_EXTENSION_NAME, true);
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, true);
}

bool ConstantData::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	// If descriptor indexing and its dependencies were enabled, then we can mark the update after bind method as supported
	if (instance->is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) &&
	    device->is_enabled(VK_KHR_MAINTENANCE3_EXTENSION_NAME) &&
	    device->is_enabled(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME))
	{
		methods[Method::UpdateAfterBindDescriptorSets].supported = true;
	}
	else
	{
		LOGW("Update-after-bind descriptor sets are not supported by your device, this sample option will be disabled.");
	}

	// Load a scene from the assets folder
	load_scene("scenes/bonza/Bonza4X.gltf");

	// Attach a move script to the camera component in the scene
	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

	// Create the render pipelines
	push_constant_render_pipeline  = create_render_pipeline<PushConstantSubpass>("constant_data/push_constant.vert", "constant_data/push_constant.frag");
	descriptor_set_render_pipeline = create_render_pipeline<DescriptorSetSubpass>("constant_data/ubo.vert", "constant_data/ubo.frag");
	buffer_array_render_pipeline   = create_render_pipeline<BufferArraySubpass>("constant_data/buffer_array.vert", "constant_data/buffer_array.frag");

	// Add a GUI with the stats you want to monitor
	stats->request_stats(std::set<vkb::StatIndex>{vkb::StatIndex::frame_times, vkb::StatIndex::gpu_load_store_cycles});
	gui = std::make_unique<vkb::Gui>(*this, *window, stats.get());

	return true;
}

void ConstantData::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().vertexPipelineStoresAndAtomics)
	{
		gpu.get_mutable_requested_features().vertexPipelineStoresAndAtomics = VK_TRUE;
	}

	gpu.request_extension_features<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT);
}

void ConstantData::draw_renderpass(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
	auto &extent = render_target.get_extent();

	VkViewport viewport{};
	viewport.width    = static_cast<float>(extent.width);
	viewport.height   = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	command_buffer.set_viewport(0, {viewport});

	VkRect2D scissor{};
	scissor.extent = extent;
	command_buffer.set_scissor(0, {scissor});

	// Get the selected method from the GUI - ensuring that it is also supported
	auto selected_method = get_active_method();

	// Only draw if using a defined method
	if (selected_method != Undefined)
	{
		// If the GUI dropdown value is changed by the user, then handle updating the subpasses and sample state
		if (gui_method_value != last_gui_method_value)
		{
			// Clear the descriptor sets for all render frames so that they recreate properly
			get_device().wait_idle();

			for (auto &render_frame : get_render_context().get_render_frames())
			{
				render_frame->clear_descriptors();
			}

			// If we are using a descriptor set method, we need to pass the method to the descriptor set pipeline
			if (selected_method != Method::PushConstants && selected_method != Method::BufferArray)
			{
				auto &subpasses = descriptor_set_render_pipeline->get_subpasses();

				for (auto &subpass : subpasses)
				{
					if (auto ubo_subpass = dynamic_cast<DescriptorSetSubpass *>(subpass.get()))
					{
						// We store the method so the subpass can apply the right resource tags
						ubo_subpass->method = selected_method;
					}
				}

				// Prepare all the subpasses again
				descriptor_set_render_pipeline->prepare();
			}

			// Set the command buffer to enable updating update-after-bind bindings if we are using update-after-binds
			command_buffer.set_update_after_bind(selected_method == Method::UpdateAfterBindDescriptorSets);

			last_gui_method_value = gui_method_value;
		}

		// Choose the correct dedicated pipeline to draw to the render target
		if (selected_method == Method::PushConstants)
		{
			push_constant_render_pipeline->draw(command_buffer, render_target);
		}
		else if (selected_method == Method::BufferArray)
		{
			buffer_array_render_pipeline->draw(command_buffer, render_target);
		}
		else
		{
			// The descriptor set pipeline has the active method stored for later
			descriptor_set_render_pipeline->draw(command_buffer, render_target);
		}

		if (gui)
		{
			gui->draw(command_buffer);
		}

		// Update the remaining bindings on all the descriptor sets
		if (selected_method == Method::UpdateAfterBindDescriptorSets)
		{
			get_render_context().get_active_frame().update_descriptor_sets();
		}

		command_buffer.end_render_pass();
	}
}

inline ConstantData::Method ConstantData::get_active_method()
{
	auto selected_method_it = methods.find(static_cast<Method>(gui_method_value));

	// If method couldn't be found, or it isn't supported we set iterator to the start
	if (selected_method_it == methods.end() || !selected_method_it->second.supported)
	{
		return Method::Undefined;
	}

	return selected_method_it->first;
}

void ConstantData::draw_gui()
{
	auto lines = 1;
	if (camera->get_aspect_ratio() < 1.0f)
	{
		// In portrait, show buttons below heading
		lines = lines * 2;
	}

	gui->show_options_window(
	    /* body = */ [this]() {
		    // Create a line for every config
		    ImGui::Text("Method of pushing MVP to shader:");

		    if (camera->get_aspect_ratio() > 1.0f)
		    {
			    // In landscape, show all options following the heading
			    ImGui::SameLine();
		    }

		    auto active_method = get_active_method();

		    // Create a radio button for every option
		    if (ImGui::BeginCombo("##constant-data-method", methods[active_method].description))
		    {
			    for (size_t i = 0; i < methods.size(); ++i)
			    {
				    auto &method = methods[static_cast<Method>(i)];
				    if (method.supported)
				    {
					    bool is_selected = active_method == static_cast<Method>(i);
					    if (ImGui::Selectable(method.description, is_selected))
					    {
						    gui_method_value = static_cast<int>(i);
					    }

					    if (is_selected)
					    {
						    ImGui::SetItemDefaultFocus();
					    }
				    }
			    }
			    ImGui::EndCombo();
		    }
	    },
	    /* lines = */ vkb::to_u32(lines));
}

std::unique_ptr<vkb::VulkanSample> create_constant_data()
{
	return std::make_unique<ConstantData>();
}

void ConstantData::ConstantDataSubpass::prepare()
{
	// Build all shader variance upfront
	auto &device = render_context.get_device();

	for (auto &mesh : meshes)
	{
		for (auto &sub_mesh : mesh->get_submeshes())
		{
			auto &variant = sub_mesh->get_mut_shader_variant();

			// Copied from vkb::ForwardSubpass
			variant.add_definitions({"SCENE_MESH_COUNT " + std::to_string(scene.get_components<vkb::sg::SubMesh>().size())});
			variant.add_definitions({"MAX_LIGHT_COUNT " + std::to_string(MAX_FORWARD_LIGHT_COUNT)});
			variant.add_definitions(vkb::light_type_definitions);

			// If struct size is 256 we add a definition so the uniform has more values
			if (struct_size == 256)
			{
				variant.add_definitions({"PUSH_CONSTANT_LIMIT_256"});
			}

			auto &vert_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), variant);
			auto &frag_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), variant);
		}
	}
}

void ConstantData::PushConstantSubpass::update_uniform(vkb::CommandBuffer &command_buffer, vkb::sg::Node &node, size_t thread_index)
{
	mvp_uniform = fill_mvp(node, camera);
}

vkb::PipelineLayout &ConstantData::PushConstantSubpass::prepare_pipeline_layout(vkb::CommandBuffer &command_buffer, const std::vector<vkb::ShaderModule *> &shader_modules)
{
	/**
	 * POI
	 * Since this pipeline doesn't use any custom descriptor set layouts, we just request a pipeline layout without modifying the modules
	 */
	return command_buffer.get_device().get_resource_cache().request_pipeline_layout(shader_modules);
}

void ConstantData::PushConstantSubpass::prepare_push_constants(vkb::CommandBuffer &command_buffer, vkb::sg::SubMesh &sub_mesh)
{
	/**
	 * POI
	 * The mvp_uniform variable contains the scene graph node mvp data.
	 * Here we just simply record the vkCmdPushConstants command
	 */

	// Push 128 bytes of data
	command_buffer.push_constants(mvp_uniform.model);                   // 64 bytes
	command_buffer.push_constants(mvp_uniform.camera_view_proj);        // 64 bytes

	// If we can push another 128 bytes, push more as this will make the delta more prominent
	if (struct_size == 256)
	{
		command_buffer.push_constants(mvp_uniform.scale);          // 64 bytes
		command_buffer.push_constants(mvp_uniform.padding);        // 64 bytes
	}
}

void ConstantData::DescriptorSetSubpass::update_uniform(vkb::CommandBuffer &command_buffer, vkb::sg::Node &node, size_t thread_index)
{
	MVPUniform mvp;

	auto &render_frame = get_render_context().get_active_frame();

	auto &transform = node.get_transform();

	auto allocation = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(MVPUniform), thread_index);

	mvp = fill_mvp(node, camera);

	// Ensure the container doesn't hold more bytes than are needed
	auto data = vkb::to_bytes(mvp);
	data.resize(struct_size);
	allocation.update(data);

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);
}

vkb::PipelineLayout &ConstantData::DescriptorSetSubpass::prepare_pipeline_layout(vkb::CommandBuffer &command_buffer, const std::vector<vkb::ShaderModule *> &shader_modules)
{
	/**
	 * POI
	 * Based on the UBO setting enabled by the sample, we mark the MVPUniform with that particular mode
	 * so when the descriptor state is flushed the corresponding API method pushes the data to the shaders
	 */
	for (auto &shader_module : shader_modules)
	{
		if (method == Method::DescriptorSets)
		{
			shader_module->set_resource_mode("MVPUniform", vkb::ShaderResourceMode::Static);
		}
		else if (method == Method::DynamicDescriptorSets)
		{
			shader_module->set_resource_mode("MVPUniform", vkb::ShaderResourceMode::Dynamic);
		}
		else if (method == Method::UpdateAfterBindDescriptorSets)
		{
			shader_module->set_resource_mode("MVPUniform", vkb::ShaderResourceMode::UpdateAfterBind);
		}
	}

	return command_buffer.get_device().get_resource_cache().request_pipeline_layout(shader_modules);
}

void ConstantData::DescriptorSetSubpass::prepare_push_constants(vkb::CommandBuffer &command_buffer, vkb::sg::SubMesh &sub_mesh)
{
	/**
	 * POI
	 * We want to disable push constants, so we override this function and intentionally do nothing (no-op)
	 */
	return;
}

void ConstantData::BufferArraySubpass::draw(vkb::CommandBuffer &command_buffer)
{
	auto &render_frame = get_render_context().get_active_frame();

	std::vector<MVPUniform> uniforms;

	// Update with all mvp scene data
	for (auto &mesh : meshes)
	{
		for (auto &node : mesh->get_nodes())
		{
			for (auto &submesh : mesh->get_submeshes())
			{
				uniforms.push_back(fill_mvp(*node, camera));
			}
		}
	}

	auto allocation = render_frame.allocate_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(MVPUniform) * uniforms.size());

	uint32_t offset = 0;
	for (size_t i = 0; i < uniforms.size(); ++i)
	{
		// Push 128 bytes of data
		allocation.update(uniforms[i].model, offset + 0);                    // Update bytes 0 - 63
		allocation.update(uniforms[i].camera_view_proj, offset + 64);        // Update bytes 64 - 127

		offset += 128;

		// If we can push another 128 bytes, push more as this will make the delta more prominent
		if (struct_size == 256)
		{
			allocation.update(uniforms[i].scale, offset);               // Update bytes 128 - 191
			allocation.update(uniforms[i].padding, offset + 64);        // Update bytes 192 - 255

			offset += 128;
		}
	}

	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 1, 0);

	// Reset the instance index back to 0 for each draw call
	instance_index = 0;

	allocate_lights<vkb::ForwardLights>(scene.get_components<vkb::sg::Light>(), MAX_FORWARD_LIGHT_COUNT);
	command_buffer.bind_lighting(get_lighting_state(), 0, 4);

	GeometrySubpass::draw(command_buffer);
}

void ConstantData::BufferArraySubpass::update_uniform(vkb::CommandBuffer &command_buffer, vkb::sg::Node &node, size_t thread_index)
{
	/**
	 * POI
	 * We fill all uniform data before the draw, so we want this function to do nothing (no-op).
	 */
	return;
}

vkb::PipelineLayout &ConstantData::BufferArraySubpass::prepare_pipeline_layout(vkb::CommandBuffer &command_buffer, const std::vector<vkb::ShaderModule *> &shader_modules)
{
	/**
	 * POI
	 * Since this pipeline doesn't use any custom descriptor set layouts, we just request a pipeline layout without modifying the modules
	 */
	return command_buffer.get_device().get_resource_cache().request_pipeline_layout(shader_modules);
}

void ConstantData::BufferArraySubpass::prepare_push_constants(vkb::CommandBuffer &command_buffer, vkb::sg::SubMesh &sub_mesh)
{
	/**
	 * POI
	 * We want to disable push constants, so we override this function and intentionally do nothing (no-op)
	 */
	return;
}

void ConstantData::BufferArraySubpass::draw_submesh_command(vkb::CommandBuffer &command_buffer, vkb::sg::SubMesh &sub_mesh)
{
	/**
	 * POI
	 * We control the shader `gl_InstanceIndex` value with the last argument of the draw commands.
	 * The BufferArraySubpass stores a value `instance_index` which is cleared to 0 before each
	 * pass, and is incremented for each mesh that we draw with this function.
	 *
	 * We bind a storage buffer object containing all the uniform data we require for the entire scene
	 * in the right order, so the index's have to match that order of how the individual uniform data
	 * structs are packed in the buffer.
	 */
	if (sub_mesh.vertex_indices != 0)
	{
		// Bind index buffer of submesh
		command_buffer.bind_index_buffer(*sub_mesh.index_buffer, sub_mesh.index_offset, sub_mesh.index_type);

		command_buffer.draw_indexed(sub_mesh.vertex_indices, 1, 0, 0, instance_index++);
	}
	else
	{
		command_buffer.draw(sub_mesh.vertices_count, 1, 0, instance_index++);
	}
}
