/* Copyright (c) 2023-2024, NVIDIA
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

#include "hpp_oit_linked_lists.h"

HPPOITLinkedLists::HPPOITLinkedLists()
{
	title = "HPP OIT linked lists";
}

HPPOITLinkedLists::~HPPOITLinkedLists()
{
	if (has_device() && get_device().get_handle())
	{
		vk::Device device = get_device().get_handle();

		device.destroyPipeline(combine_pipeline);
		device.destroyPipeline(background_pipeline);
		device.destroyPipeline(gather_pipeline);
		device.destroyPipelineLayout(pipeline_layout);

		device.destroyDescriptorPool(descriptor_pool);
		device.destroyDescriptorSetLayout(descriptor_set_layout);

		destroy_sized_objects();

		device.destroySampler(background_texture.sampler);
	}
}

bool HPPOITLinkedLists::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);

	if (HPPApiVulkanSample::prepare(options))
	{
		initialize_camera();
		load_assets();
		create_constant_buffers();
		create_descriptors();
		create_sized_objects(extent);
		create_pipelines();
		update_scene_constants();
		fill_instance_data();
		update_descriptors();
		build_command_buffers();

		prepared = true;
	}
	return prepared;
}

bool HPPOITLinkedLists::resize(const uint32_t width, const uint32_t height)
{
	if ((width != extent.width) || (height != extent.height))
	{
		destroy_sized_objects();
		create_sized_objects({width, height});
		update_descriptors();
	}
	return HPPApiVulkanSample::resize(width, height);
}

void HPPOITLinkedLists::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	auto       &requested_features = gpu.get_mutable_requested_features();
	auto const &features           = gpu.get_features();

	if (features.fragmentStoresAndAtomics)
	{
		requested_features.fragmentStoresAndAtomics = true;
	}
	else
	{
		throw std::runtime_error("This sample requires support for buffers and images stores and atomic operations in the fragment shader stage");
	}

	// Enable anisotropic filtering if supported
	if (features.samplerAnisotropy)
	{
		requested_features.samplerAnisotropy = true;
	}
}

void HPPOITLinkedLists::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	vk::RenderPassBeginInfo gather_render_pass_begin_info(gather_render_pass, gather_framebuffer, {{0, 0}, extent});

	std::array<vk::ClearValue, 2> combine_clear_values =
	    {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
	      vk::ClearDepthStencilValue(0.0f, 0)}};
	vk::RenderPassBeginInfo combine_render_pass_begin_info(render_pass, {}, {{0, 0}, extent}, combine_clear_values);

	vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
	vk::Rect2D   scissor({0, 0}, extent);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		auto const &command_buffer = draw_cmd_buffers[i];
		command_buffer.begin(command_buffer_begin_info);
		{
			// Gather pass
			command_buffer.beginRenderPass(gather_render_pass_begin_info, vk::SubpassContents::eInline);
			{
				command_buffer.setViewport(0, viewport);
				command_buffer.setScissor(0, scissor);

				command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set, {});
				command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gather_pipeline);
				draw_model(object, command_buffer, kInstanceCount);
			}
			command_buffer.endRenderPass();

			vkb::common::image_layout_transition(command_buffer,
			                                     linked_list_head_image->get_handle(),
			                                     vk::PipelineStageFlagBits::eFragmentShader,
			                                     vk::PipelineStageFlagBits::eFragmentShader,
			                                     vk::AccessFlagBits::eShaderWrite,
			                                     vk::AccessFlagBits::eShaderRead,
			                                     vk::ImageLayout::eGeneral,
			                                     vk::ImageLayout::eGeneral,
			                                     {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

			// Combine pass
			combine_render_pass_begin_info.framebuffer = framebuffers[i];
			command_buffer.beginRenderPass(combine_render_pass_begin_info, vk::SubpassContents::eInline);
			{
				command_buffer.setViewport(0, viewport);
				command_buffer.setScissor(0, scissor);

				command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptor_set, {});
				command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, background_pipeline);
				command_buffer.draw(3, 1, 0, 0);

				command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, combine_pipeline);
				command_buffer.draw(3, 1, 0, 0);

				draw_ui(command_buffer);
			}
			command_buffer.endRenderPass();
		}
		command_buffer.end();
	}
}

void HPPOITLinkedLists::on_update_ui_overlay(vkb::Drawer &drawer)
{
	drawer.checkbox("Sort fragments", &sort_fragments);
	drawer.checkbox("Camera auto-rotation", &camera_auto_rotation);
	drawer.slider_int("Sorted fragments per pixel", &sorted_fragment_count, kSortedFragmentMinCount, kSortedFragmentMaxCount);
	drawer.slider_float("Background grayscale", &background_grayscale, kBackgroundGrayscaleMin, kBackgroundGrayscaleMax);
}

void HPPOITLinkedLists::render(float delta_time)
{
	if (prepared)
	{
		HPPApiVulkanSample::prepare_frame();
		submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);
		queue.submit(submit_info);
		HPPApiVulkanSample::submit_frame();

		if (camera_auto_rotation)
		{
			camera.rotate({delta_time * 5.0f, delta_time * 5.0f, 0.0f});
		}
		update_scene_constants();
	}
}

////////////////////////////////////////////////////////////////////////////////

void HPPOITLinkedLists::clear_sized_resources()
{
	vk::CommandBuffer command_buffer = vkb::common::allocate_command_buffer(get_device().get_handle(), cmd_pool);

	vk::CommandBufferBeginInfo command_buffer_begin_info;
	command_buffer.begin(command_buffer_begin_info);
	{
		command_buffer.fillBuffer(fragment_counter->get_handle(), 0, sizeof(glm::uint), 0);

		vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
		vkb::common::image_layout_transition(command_buffer,
		                                     linked_list_head_image->get_handle(),
		                                     vk::PipelineStageFlagBits::eBottomOfPipe,
		                                     vk::PipelineStageFlagBits::eTransfer,
		                                     vk::AccessFlagBits::eMemoryWrite,
		                                     vk::AccessFlagBits::eTransferWrite,
		                                     vk::ImageLayout::eUndefined,
		                                     vk::ImageLayout::eGeneral,
		                                     subresource_range);

		vk::ClearColorValue linked_lists_clear_value(kLinkedListEndSentinel, kLinkedListEndSentinel, kLinkedListEndSentinel, kLinkedListEndSentinel);
		command_buffer.clearColorImage(linked_list_head_image->get_handle(), vk::ImageLayout::eGeneral, linked_lists_clear_value, subresource_range);
	}
	command_buffer.end();

	{
		vk::SubmitInfo submit_info({}, {}, command_buffer);
		queue.submit(submit_info);
		queue.waitIdle();
	}

	get_device().get_handle().freeCommandBuffers(cmd_pool, command_buffer);
}

void HPPOITLinkedLists::create_constant_buffers()
{
	scene_constants =
	    std::make_unique<vkb::core::HPPBuffer>(get_device(), sizeof(SceneConstants), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
	instance_data = std::make_unique<vkb::core::HPPBuffer>(
	    get_device(), sizeof(Instance) * kInstanceCount, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

vk::DescriptorPool HPPOITLinkedLists::create_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 4> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 2},
	                                                     {vk::DescriptorType::eStorageImage, 1},
	                                                     {vk::DescriptorType::eStorageBuffer, 2},
	                                                     {vk::DescriptorType::eCombinedImageSampler, 1}}};

	vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 1, pool_sizes);
	return get_device().get_handle().createDescriptorPool(descriptor_pool_create_info);
}

vk::DescriptorSetLayout HPPOITLinkedLists::create_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 6> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
	     {1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	     {2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eFragment},
	     {3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment},
	     {4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment},
	     {5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};
	return get_device().get_handle().createDescriptorSetLayout({{}, set_layout_bindings});
}

void HPPOITLinkedLists::create_descriptors()
{
	descriptor_set_layout = create_descriptor_set_layout();
	descriptor_pool       = create_descriptor_pool();
	descriptor_set        = vkb::common::allocate_descriptor_set(get_device().get_handle(), descriptor_pool, descriptor_set_layout);
}

void HPPOITLinkedLists::create_fragment_resources(vk::Extent2D const &extent)
{
	const vk::Extent3D image_extent{extent, 1};
	const vk::Format   image_format{vk::Format::eR32Uint};
	linked_list_head_image      = std::make_unique<vkb::core::HPPImage>(get_device(),
                                                                   image_extent,
                                                                   image_format,
                                                                   vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
                                                                   VMA_MEMORY_USAGE_GPU_ONLY,
                                                                   vk::SampleCountFlagBits::e1);
	linked_list_head_image_view = std::make_unique<vkb::core::HPPImageView>(*linked_list_head_image, vk::ImageViewType::e2D, image_format);

	fragment_max_count                  = extent.width * extent.height * kFragmentsPerPixelAverage;
	const uint32_t fragment_buffer_size = sizeof(glm::uvec3) * fragment_max_count;
	fragment_buffer =
	    std::make_unique<vkb::core::HPPBuffer>(get_device(), fragment_buffer_size, vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY);

	fragment_counter = std::make_unique<vkb::core::HPPBuffer>(
	    get_device(), sizeof(glm::uint), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);
}

void HPPOITLinkedLists::create_gather_pass_objects(vk::Extent2D const &extent)
{
	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics);
	gather_render_pass = get_device().get_handle().createRenderPass({{}, nullptr, subpass});

	gather_framebuffer = vkb::common::create_framebuffer(get_device().get_handle(), gather_render_pass, {}, extent);
}

void HPPOITLinkedLists::create_pipelines()
{
	pipeline_layout = get_device().get_handle().createPipelineLayout({{}, descriptor_set_layout});

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.depthTestEnable  = false;
	depth_stencil_state.depthWriteEnable = false;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eAlways;
	depth_stencil_state.front            = depth_stencil_state.back;

	std::vector<vk::PipelineShaderStageCreateInfo> gather_shader_stages = {load_shader("oit_linked_lists/gather.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                       load_shader("oit_linked_lists/gather.frag", vk::ShaderStageFlagBits::eFragment)};

	vk::VertexInputBindingDescription      gather_vertex_input_binding(0, sizeof(HPPVertex), vk::VertexInputRate::eVertex);
	vk::VertexInputAttributeDescription    gather_vertex_input_attribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(HPPVertex, pos));
	vk::PipelineVertexInputStateCreateInfo gather_vertex_input_state({}, gather_vertex_input_binding, gather_vertex_input_attribute);

	gather_pipeline = vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                                        pipeline_cache,
	                                                        gather_shader_stages,
	                                                        gather_vertex_input_state,
	                                                        vk::PrimitiveTopology::eTriangleList,
	                                                        0,
	                                                        vk::PolygonMode::eFill,
	                                                        vk::CullModeFlagBits::eNone,
	                                                        vk::FrontFace::eCounterClockwise,
	                                                        {blend_attachment_state},
	                                                        depth_stencil_state,
	                                                        pipeline_layout,
	                                                        gather_render_pass);

	std::vector<vk::PipelineShaderStageCreateInfo> background_shader_stages = {load_shader("oit_linked_lists/fullscreen.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                           load_shader("oit_linked_lists/background.frag", vk::ShaderStageFlagBits::eFragment)};
	vk::PipelineVertexInputStateCreateInfo         vertex_input_state;

	background_pipeline = vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                                            pipeline_cache,
	                                                            background_shader_stages,
	                                                            vertex_input_state,
	                                                            vk::PrimitiveTopology::eTriangleList,
	                                                            0,
	                                                            vk::PolygonMode::eFill,
	                                                            vk::CullModeFlagBits::eNone,
	                                                            vk::FrontFace::eCounterClockwise,
	                                                            {blend_attachment_state},
	                                                            depth_stencil_state,
	                                                            pipeline_layout,
	                                                            render_pass);

	std::vector<vk::PipelineShaderStageCreateInfo> combine_shader_stages = {load_shader("oit_linked_lists/combine.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                        load_shader("oit_linked_lists/combine.frag", vk::ShaderStageFlagBits::eFragment)};

	vk::PipelineColorBlendAttachmentState combine_blend_attachment_state(true,
	                                                                     vk::BlendFactor::eSrcAlpha,
	                                                                     vk::BlendFactor::eOneMinusSrcAlpha,
	                                                                     vk::BlendOp::eAdd,
	                                                                     vk::BlendFactor::eOne,
	                                                                     vk::BlendFactor::eZero,
	                                                                     vk::BlendOp::eAdd,
	                                                                     vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                                                         vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	combine_pipeline = vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                                         pipeline_cache,
	                                                         combine_shader_stages,
	                                                         vertex_input_state,
	                                                         vk::PrimitiveTopology::eTriangleList,
	                                                         0,
	                                                         vk::PolygonMode::eFill,
	                                                         vk::CullModeFlagBits::eNone,
	                                                         vk::FrontFace::eCounterClockwise,
	                                                         {combine_blend_attachment_state},
	                                                         depth_stencil_state,
	                                                         pipeline_layout,
	                                                         render_pass);
}

void HPPOITLinkedLists::create_sized_objects(vk::Extent2D const &extent)
{
	create_gather_pass_objects(extent);
	create_fragment_resources(extent);
	clear_sized_resources();
}

void HPPOITLinkedLists::destroy_sized_objects()
{
	get_device().get_handle().destroyFramebuffer(gather_framebuffer);
	get_device().get_handle().destroyRenderPass(gather_render_pass);

	fragment_counter.reset();
	fragment_buffer.reset();
	fragment_max_count = 0;
	linked_list_head_image_view.reset();
	linked_list_head_image.reset();
}

void HPPOITLinkedLists::fill_instance_data()
{
	Instance instances[kInstanceCount] = {};

	auto get_random_float = []() { return static_cast<float>(rand()) / (RAND_MAX); };

	for (uint32_t l = 0, instance_index = 0; l < kInstanceLayerCount; ++l)
	{
		for (uint32_t c = 0; c < kInstanceColumnCount; ++c)
		{
			for (uint32_t r = 0; r < kInstanceRowCount; ++r, ++instance_index)
			{
				const float x                     = static_cast<float>(r) - ((kInstanceRowCount - 1) * 0.5f);
				const float y                     = static_cast<float>(c) - ((kInstanceColumnCount - 1) * 0.5f);
				const float z                     = static_cast<float>(l) - ((kInstanceLayerCount - 1) * 0.5f);
				const float scale                 = 0.02f;
				instances[instance_index].model   = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)), glm::vec3(scale));
				instances[instance_index].color.r = get_random_float();
				instances[instance_index].color.g = get_random_float();
				instances[instance_index].color.b = get_random_float();
				instances[instance_index].color.a = get_random_float() * 0.8f + 0.2f;
			}
		}
	}

	instance_data->convert_and_update(instances);
}

void HPPOITLinkedLists::initialize_camera()
{
	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 0.0f, -4.0f});
	camera.set_rotation({0.0f, 0.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 256.0f, 0.1f);
}

void HPPOITLinkedLists::load_assets()
{
	object             = load_model("scenes/geosphere.gltf");
	background_texture = load_texture("textures/vulkan_logo_full.ktx", vkb::scene_graph::components::HPPImage::Color);
}

void HPPOITLinkedLists::update_descriptors()
{
	vk::DescriptorBufferInfo scene_constants_descriptor(scene_constants->get_handle(), 0, vk::WholeSize);
	vk::DescriptorBufferInfo instance_data_descriptor(instance_data->get_handle(), 0, vk::WholeSize);
	vk::DescriptorImageInfo  linked_list_head_image_view_descriptor(nullptr, linked_list_head_image_view->get_handle(), vk::ImageLayout::eGeneral);
	vk::DescriptorBufferInfo fragment_buffer_descriptor(fragment_buffer->get_handle(), 0, vk::WholeSize);
	vk::DescriptorBufferInfo fragment_counter_descriptor(fragment_counter->get_handle(), 0, vk::WholeSize);
	vk::DescriptorImageInfo  background_texture_descriptor(
        background_texture.sampler,
        background_texture.image->get_vk_image_view().get_handle(),
        descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, background_texture.image->get_vk_image_view().get_format()));

	std::array<vk::WriteDescriptorSet, 6> write_descriptor_sets = {
	    {{descriptor_set, 0, 0, vk::DescriptorType::eUniformBuffer, {}, scene_constants_descriptor},
	     {descriptor_set, 1, 0, vk::DescriptorType::eUniformBuffer, {}, instance_data_descriptor},
	     {descriptor_set, 2, 0, vk::DescriptorType::eStorageImage, linked_list_head_image_view_descriptor},
	     {descriptor_set, 3, 0, vk::DescriptorType::eStorageBuffer, {}, fragment_buffer_descriptor},
	     {descriptor_set, 4, 0, vk::DescriptorType::eStorageBuffer, {}, fragment_counter_descriptor},
	     {descriptor_set, 5, 0, vk::DescriptorType::eCombinedImageSampler, background_texture_descriptor}}};
	get_device().get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPOITLinkedLists::update_scene_constants()
{
	SceneConstants constants        = {};
	constants.projection            = camera.matrices.perspective;
	constants.view                  = camera.matrices.view;
	constants.background_grayscale  = background_grayscale;
	constants.sort_fragments        = sort_fragments ? 1U : 0U;
	constants.fragment_max_count    = fragment_max_count;
	constants.sorted_fragment_count = sorted_fragment_count;
	scene_constants->convert_and_update(constants);
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<vkb::Application> create_hpp_oit_linked_lists()
{
	return std::make_unique<HPPOITLinkedLists>();
}
