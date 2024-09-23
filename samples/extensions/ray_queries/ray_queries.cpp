/* Copyright (c) 2021-2024, Holochip Corporation
 * Copyright (c) 2024, Arm Limited and Contributors
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

#include "ray_queries.h"
#include "filesystem/legacy.h"
#include "gltf_loader.h"

#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/perspective_camera.h"

namespace
{
constexpr uint32_t MIN_THREAD_COUNT = 1;

template <typename T>
struct CopyBuffer
{
	std::vector<T> operator()(std::unordered_map<std::string, vkb::core::BufferC> &buffers, const char *buffer_name)
	{
		auto iter = buffers.find(buffer_name);
		if (iter == buffers.cend())
		{
			return {};
		}
		auto          &buffer = iter->second;
		std::vector<T> out;

		const size_t sz = buffer.get_size();
		out.resize(sz / sizeof(T));
		const bool already_mapped = buffer.get_data() != nullptr;
		if (!already_mapped)
		{
			buffer.map();
		}
		memcpy(&out[0], buffer.get_data(), sz);
		if (!already_mapped)
		{
			buffer.unmap();
		}
		return out;
	}
};
}        // namespace

RayQueries::RayQueries()
{
	title = "Ray queries";

	// SPIRV 1.4 requires Vulkan 1.1
	set_api_version(VK_API_VERSION_1_1);
	add_device_extension(VK_KHR_RAY_QUERY_EXTENSION_NAME);

	// Ray tracing related extensions required by this sample
	add_device_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

	// Required by VK_KHR_acceleration_structure
	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	add_device_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	// Required for ray queries
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// Required by VK_KHR_spirv_1_4
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

RayQueries::~RayQueries()
{
	if (has_device())
	{
		auto device_ptr = get_device().get_handle();
		vertex_buffer.reset();
		index_buffer.reset();
		uniform_buffer.reset();
		vkDestroyPipeline(device_ptr, pipeline, nullptr);
		vkDestroyPipelineLayout(device_ptr, pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(device_ptr, descriptor_set_layout, nullptr);
	}
}

void RayQueries::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	REQUEST_REQUIRED_FEATURE(gpu,
	                         VkPhysicalDeviceBufferDeviceAddressFeatures,
	                         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
	                         bufferDeviceAddress);
	REQUEST_REQUIRED_FEATURE(gpu,
	                         VkPhysicalDeviceAccelerationStructureFeaturesKHR,
	                         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
	                         accelerationStructure);
	REQUEST_REQUIRED_FEATURE(gpu,
	                         VkPhysicalDeviceRayQueryFeaturesKHR,
	                         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
	                         rayQuery);
}

void RayQueries::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	draw();

	update_uniform_buffers();
}

void RayQueries::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(draw_cmd_buffers[i], static_cast<uint32_t>(model.indices.size()) * 3, 1, 0, 0, 0);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

bool RayQueries::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	// Get the acceleration structure features, which we'll need later on in the sample
	acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	VkPhysicalDeviceFeatures2 device_features{};
	device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	device_features.pNext = &acceleration_structure_features;
	vkGetPhysicalDeviceFeatures2(get_device().get_gpu().get_handle(), &device_features);

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 512.0f);
	camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.set_translation(glm::vec3(0.0f, 1.5f, 0.f));

	load_scene();
	create_bottom_level_acceleration_structure();
	create_top_level_acceleration_structure();
	create_uniforms();
	create_descriptor_pool();
	prepare_pipelines();
	create_descriptor_sets();
	build_command_buffers();
	prepared = true;

	return true;
}

uint64_t RayQueries::get_buffer_device_address(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
	buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(get_device().get_handle(), &buffer_device_address_info);
}

void RayQueries::create_top_level_acceleration_structure()
{
	VkTransformMatrixKHR transform_matrix = {
	    1.0f, 0.0f, 0.0f, 0.0f,
	    0.0f, 1.0f, 0.0f, 0.0f,
	    0.0f, 0.0f, 1.0f, 0.0f};

	VkAccelerationStructureInstanceKHR acceleration_structure_instance{};
	acceleration_structure_instance.transform                              = transform_matrix;
	acceleration_structure_instance.instanceCustomIndex                    = 0;
	acceleration_structure_instance.mask                                   = 0xFF;
	acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;
	acceleration_structure_instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	acceleration_structure_instance.accelerationStructureReference         = bottom_level_acceleration_structure->get_device_address();

	std::unique_ptr<vkb::core::BufferC> instances_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                                                            sizeof(VkAccelerationStructureInstanceKHR),
	                                                                                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	                                                                                            VMA_MEMORY_USAGE_CPU_TO_GPU);
	instances_buffer->update(&acceleration_structure_instance, sizeof(VkAccelerationStructureInstanceKHR));

	// Top Level AS with single instance
	top_level_acceleration_structure = std::make_unique<vkb::core::AccelerationStructure>(get_device(), VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
	top_level_acceleration_structure->add_instance_geometry(instances_buffer, 1);
	top_level_acceleration_structure->build(queue);
}

void RayQueries::create_bottom_level_acceleration_structure()
{
	auto vertex_buffer_size = model.vertices.size() * sizeof(Vertex);
	auto index_buffer_size  = model.indices.size() * sizeof(model.indices[0]);

	// Create buffers for the bottom level geometry
	// For the sake of simplicity we won't stage the vertex data to the GPU memory

	// Note that the buffer usage flags for buffers consumed by the bottom level acceleration structure require special flags
	const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	vertex_buffer = std::make_unique<vkb::core::BufferC>(get_device(), vertex_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(model.vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::BufferC>(get_device(), index_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer->update(model.indices.data(), index_buffer_size);

	// Set up a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
	VkTransformMatrixKHR transform_matrix = {
	    1.0f, 0.0f, 0.0f, 0.0f,
	    0.0f, 1.0f, 0.0f, 0.0f,
	    0.0f, 0.0f, 1.0f, 0.0f};
	std::unique_ptr<vkb::core::BufferC> transform_matrix_buffer = std::make_unique<vkb::core::BufferC>(get_device(), sizeof(transform_matrix), buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	transform_matrix_buffer->update(&transform_matrix, sizeof(transform_matrix));

	if (bottom_level_acceleration_structure == nullptr)
	{
		bottom_level_acceleration_structure = std::make_unique<vkb::core::AccelerationStructure>(
		    get_device(), VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
		bottom_level_acceleration_structure->add_triangle_geometry(*vertex_buffer,
		                                                           *index_buffer,
		                                                           *transform_matrix_buffer,
		                                                           static_cast<uint32_t>(model.indices.size()),
		                                                           static_cast<uint32_t>(model.vertices.size()) - 1,
		                                                           sizeof(Vertex),
		                                                           0,
		                                                           VK_FORMAT_R32G32B32_SFLOAT,
		                                                           VK_INDEX_TYPE_UINT32,
		                                                           VK_GEOMETRY_OPAQUE_BIT_KHR,
		                                                           get_buffer_device_address(vertex_buffer->get_handle()),
		                                                           get_buffer_device_address(index_buffer->get_handle()));
	}
	bottom_level_acceleration_structure->build(queue, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR);
}

void RayQueries::load_scene()
{
	model = {};

	vkb::GLTFLoader loader{get_device()};
	auto            scene = loader.read_scene_from_file("scenes/sponza/Sponza01.gltf");

	for (auto &&mesh : scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &&sub_mesh : mesh->get_submeshes())
		{
			auto       pts_               = CopyBuffer<glm::vec3>{}(sub_mesh->vertex_buffers, "position");
			const auto normals_           = CopyBuffer<glm::vec3>{}(sub_mesh->vertex_buffers, "normal");
			const auto vertex_start_index = static_cast<uint32_t>(model.vertices.size());

			// Copy vertex data
			{
				model.vertices.resize(vertex_start_index + pts_.size());
				const float sponza_scale = 0.01f;
				for (size_t i = 0; i < pts_.size(); ++i)
				{
					model.vertices[vertex_start_index + i].position = sponza_scale * pts_[i].yzx;
					model.vertices[vertex_start_index + i].normal   = normals_[i].yzx;
				}
			}

			// Copy index data
			{
				auto index_buffer_ = sub_mesh->index_buffer.get();
				if (index_buffer_)
				{
					assert(sub_mesh->index_type == VkIndexType::VK_INDEX_TYPE_UINT16);
					const size_t sz                   = index_buffer_->get_size();
					const size_t nTriangles           = sz / sizeof(uint16_t) / 3;
					const auto   triangle_start_index = static_cast<uint32_t>(model.indices.size());
					model.indices.resize(triangle_start_index + nTriangles);
					auto ptr = index_buffer_->get_data();
					assert(!!ptr);
					std::vector<uint16_t> tempBuffer(nTriangles * 3);
					memcpy(&tempBuffer[0], ptr, sz);
					for (size_t i = 0; i < nTriangles; ++i)
					{
						model.indices[triangle_start_index + i] = {vertex_start_index + static_cast<uint32_t>(tempBuffer[3 * i]),
						                                           vertex_start_index + static_cast<uint32_t>(tempBuffer[3 * i + 1]),
						                                           vertex_start_index + static_cast<uint32_t>(tempBuffer[3 * i + 2])};
					}
				}
			}
		}
	}
}

void RayQueries::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
	    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 1);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));

	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings =
	    {
	        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1)};

	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void RayQueries::create_descriptor_sets()
{
	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_set));

	// Set up the descriptor for binding our top level acceleration structure to the ray tracing shaders
	VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
	descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptor_acceleration_structure_info.accelerationStructureCount = 1;
	auto rhs                                                          = top_level_acceleration_structure->get_handle();
	descriptor_acceleration_structure_info.pAccelerationStructures    = &rhs;

	VkWriteDescriptorSet acceleration_structure_write{};
	acceleration_structure_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	acceleration_structure_write.dstSet          = descriptor_set;
	acceleration_structure_write.dstBinding      = 0;
	acceleration_structure_write.descriptorCount = 1;
	acceleration_structure_write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	// The acceleration structure descriptor has to be chained via pNext
	acceleration_structure_write.pNext = &descriptor_acceleration_structure_info;

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer);

	VkWriteDescriptorSet uniform_buffer_write = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &buffer_descriptor);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    acceleration_structure_write,
	    uniform_buffer_write,
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
}

void RayQueries::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);

	VkPipelineColorBlendAttachmentState blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	depth_stencil_state.depthBoundsTestEnable                 = VK_FALSE;
	depth_stencil_state.minDepthBounds                        = 0.f;
	depth_stencil_state.maxDepthBounds                        = 1.f;

	VkPipelineViewportStateCreateInfo viewport_state = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	VkPipelineMultisampleStateCreateInfo multisample_state = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pRasterizationState          = &rasterization_state;
	pipeline_create_info.pColorBlendState             = &color_blend_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;

	const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
	    load_shader("ray_queries", "ray_shadow.vert", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("ray_queries", "ray_shadow.frag", VK_SHADER_STAGE_FRAGMENT_BIT)};

	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

void RayQueries::create_uniforms()
{
	// Note that in contrast to a typical pipeline, our vertex/index buffer requires the acceleration structure build flag
	static constexpr VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	const auto vertex_buffer_size = model.vertices.size() * sizeof(model.vertices[0]);
	const auto index_buffer_size  = model.indices.size() * sizeof(model.indices[0]);
	vertex_buffer                 = std::make_unique<vkb::core::BufferC>(get_device(),
                                                         vertex_buffer_size,
                                                         buffer_usage_flags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer                  = std::make_unique<vkb::core::BufferC>(get_device(),
                                                        index_buffer_size,
                                                        buffer_usage_flags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);
	if (vertex_buffer_size)
	{
		vertex_buffer->update(model.vertices.data(), vertex_buffer_size);
	}
	if (index_buffer_size)
	{
		index_buffer->update(model.indices.data(), index_buffer_size);
	}

	uniform_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
	                                                      sizeof(global_uniform),
	                                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                      VMA_MEMORY_USAGE_CPU_TO_GPU);
	update_uniform_buffers();
}

void RayQueries::update_uniform_buffers()
{
	assert(!!uniform_buffer);
	global_uniform.camera_position = camera.position;
	global_uniform.proj            = camera.matrices.perspective;
	global_uniform.view            = camera.matrices.view;

	const float R                 = 1.f;
	const float P                 = 2.f * 3.14159f / 5000.f;
	const float t                 = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count()) / 1000.f;
	global_uniform.light_position = glm::vec3(2 * R * cosf(t * P), R * sinf(t * P), -10.f);

	uniform_buffer->update(&global_uniform, sizeof(global_uniform));
}

void RayQueries::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

std::unique_ptr<vkb::VulkanSampleC> create_ray_queries()
{
	return std::make_unique<RayQueries>();
}
