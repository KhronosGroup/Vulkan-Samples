/* Copyright (c) 2023-2024, NVIDIA CORPORATION. All rights reserved.
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

/*
 * Timestamp queries (based on the HDR sample), using vulkan.hpp
 */

#include "hpp_timestamp_queries.h"

HPPTimestampQueries::HPPTimestampQueries()
{
	title = "Timestamp queries";
	// This sample uses vk::CommandBuffer::resetQueryPool to reset the timestamp query pool on the host, which requires VK_EXT_host_query_reset or Vulkan 1.2
	add_device_extension(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
	// This also requires us to enable the feature in the appropriate feature struct, see request_gpu_features()
}

HPPTimestampQueries::~HPPTimestampQueries()
{
	if (has_device() && get_device().get_handle())
	{
		vk::Device device = get_device().get_handle();

		time_stamps.destroy(device);
		bloom.destroy(device);
		composition.destroy(device);
		filter_pass.destroy(device);
		models.destroy(device, descriptor_pool);
		offscreen.destroy(device);
		textures.destroy(device);
	}
}

bool HPPTimestampQueries::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);
	if (HPPApiVulkanSample::prepare(options))
	{
		// Check if the selected device supports timestamps. A value of zero means no support.
		vk::PhysicalDeviceLimits const &device_limits = get_device().get_gpu().get_properties().limits;
		if (device_limits.timestampPeriod == 0)
		{
			throw std::runtime_error{"The selected device does not support timestamp queries!"};
		}

		// Check if all queues support timestamp queries, if not we need to check on a per-queue basis
		if (!device_limits.timestampComputeAndGraphics)
		{
			// Check if the graphics queue used in this sample supports time stamps
			vk::QueueFamilyProperties const &graphics_queue_family_properties = get_device().get_suitable_graphics_queue().get_properties();
			if (graphics_queue_family_properties.timestampValidBits == 0)
			{
				throw std::runtime_error{"The selected graphics queue family does not support timestamp queries!"};
			}
		}

		prepare_camera();
		load_assets();
		prepare_uniform_buffers();
		prepare_offscreen_buffer();
		descriptor_pool = create_descriptor_pool();
		prepare_bloom();
		prepare_composition();
		prepare_models();
		prepare_time_stamps();
		build_command_buffers();

		prepared = true;
	}

	return prepared;
}

bool HPPTimestampQueries::resize(const uint32_t width, const uint32_t height)
{
	HPPApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

void HPPTimestampQueries::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// We need to enable the command pool reset feature in the extension struct
	auto &requested_extension_features          = gpu.request_extension_features<vk::PhysicalDeviceHostQueryResetFeaturesEXT>();
	requested_extension_features.hostQueryReset = true;

	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

void HPPTimestampQueries::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		vk::CommandBuffer command_buffer = draw_cmd_buffers[i];
		command_buffer.begin(command_buffer_begin_info);

		// Reset the timestamp query pool, so we can start fetching new values into it
		command_buffer.resetQueryPool(time_stamps.query_pool, 0, static_cast<uint32_t>(time_stamps.values.size()));

		{
			/*
			    First pass: Render scene to offscreen framebuffer
			*/
			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, time_stamps.query_pool, 0);

			std::array<vk::ClearValue, 3> clear_values = {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			                                               vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			                                               vk::ClearDepthStencilValue(0.0f, 0)}};
			vk::RenderPassBeginInfo       render_pass_begin_info(offscreen.render_pass, offscreen.framebuffer, {{0, 0}, offscreen.extent}, clear_values);
			command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(offscreen.extent.width), static_cast<float>(offscreen.extent.height), 0.0f, 1.0f);
			command_buffer.setViewport(0, viewport);

			vk::Rect2D scissor({0, 0}, offscreen.extent);
			command_buffer.setScissor(0, scissor);

			// Skybox
			if (display_skybox)
			{
				command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, models.skybox.pipeline);
				command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, models.pipeline_layout, 0, models.skybox.descriptor_set, {});
				draw_model(models.skybox.meshes[0], command_buffer);
			}

			// 3D object
			command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, models.objects.pipeline);
			command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, models.pipeline_layout, 0, models.objects.descriptor_set, {});
			draw_model(models.objects.meshes[models.object_index], command_buffer);

			command_buffer.endRenderPass();

			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, time_stamps.query_pool, 1);
		}

		/*
		    Second render pass: First bloom pass
		*/
		if (bloom.enabled)
		{
			vk::ClearValue clear_value(vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})));

			// Bloom filter
			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, time_stamps.query_pool, 2);

			vk::RenderPassBeginInfo render_pass_begin_info(filter_pass.render_pass, filter_pass.framebuffer, {{0, 0}, filter_pass.extent}, clear_value);
			command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(filter_pass.extent.width), static_cast<float>(filter_pass.extent.height), 0.0f, 1.0f);
			command_buffer.setViewport(0, viewport);

			vk::Rect2D scissor({0, 0}, filter_pass.extent);
			command_buffer.setScissor(0, scissor);

			command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, bloom.pipeline_layout, 0, bloom.descriptor_set, {});
			command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, bloom.pipelines[1]);
			command_buffer.draw(3, 1, 0, 0);

			command_buffer.endRenderPass();

			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, time_stamps.query_pool, 3);
		}

		/*
		    Note: Explicit synchronization is not required between the render pass, as this is done implicitly via sub pass dependencies
		*/

		/*
		    Third render pass: Scene rendering with applied second bloom pass (when enabled)
		*/
		{
			std::array<vk::ClearValue, 2> clear_values = {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			                                               vk::ClearDepthStencilValue(0.0f, 0)}};

			// Final composition
			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, time_stamps.query_pool, bloom.enabled ? 4 : 2);

			vk::RenderPassBeginInfo render_pass_begin_info(render_pass, framebuffers[i], {{0, 0}, extent}, clear_values);
			command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
			command_buffer.setViewport(0, viewport);

			vk::Rect2D scissor({0, 0}, extent);
			command_buffer.setScissor(0, scissor);

			command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, composition.pipeline_layout, 0, composition.descriptor_set, {});

			// Scene
			command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, composition.pipeline);
			command_buffer.draw(3, 1, 0, 0);

			// Bloom
			if (bloom.enabled)
			{
				command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, bloom.pipelines[0]);
				command_buffer.draw(3, 1, 0, 0);
			}

			draw_ui(command_buffer);

			command_buffer.endRenderPass();

			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, time_stamps.query_pool, bloom.enabled ? 5 : 3);
		}

		command_buffer.end();
	}
}

void HPPTimestampQueries::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("Object type", &models.object_index, object_names))
		{
			update_uniform_buffers();
			rebuild_command_buffers();
		}
		if (drawer.input_float("Exposure", &ubo_params.exposure, 0.025f, "%0.3f"))
		{
			update_params();
		}
		if (drawer.checkbox("Bloom", &bloom.enabled))
		{
			rebuild_command_buffers();
		}
		if (drawer.checkbox("Skybox", &display_skybox))
		{
			rebuild_command_buffers();
		}
	}
	if (drawer.header("timing"))
	{
		// Timestamps don't have a time unit themselves, but are read as timesteps
		// The timestampPeriod property of the device tells how many nanoseconds such a timestep translates to on the selected device
		float timestampFrequency = get_device().get_gpu().get_properties().limits.timestampPeriod;

		drawer.text("Pass 1: Offscreen scene rendering: %.3f ms", float(time_stamps.values[1] - time_stamps.values[0]) * timestampFrequency / 1000000.0f);
		drawer.text("Pass 2: %s %.3f ms", (bloom.enabled ? "First bloom pass" : "Scene display"), float(time_stamps.values[3] - time_stamps.values[2]) * timestampFrequency / 1000000.0f);
		if (bloom.enabled)
		{
			drawer.text("Pass 3: Second bloom pass %.3f ms", float(time_stamps.values[5] - time_stamps.values[4]) * timestampFrequency / 1000000.0f);
			drawer.set_dirty(true);
		}
	}
}

void HPPTimestampQueries::render(float delta_time)
{
	if (prepared)
	{
		draw();
		if (camera.updated)
		{
			update_uniform_buffers();
		}
	}
}

vk::DeviceMemory HPPTimestampQueries::allocate_memory(vk::Image image)
{
	vk::MemoryRequirements memory_requirements = get_device().get_handle().getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size,
	                                            get_device().get_gpu().get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	return get_device().get_handle().allocateMemory(memory_allocate_info);
}

HPPTimestampQueries::FramebufferAttachment HPPTimestampQueries::create_attachment(vk::Format format, vk::ImageUsageFlagBits usage)
{
	vk::Image        image  = create_image(format, usage);
	vk::DeviceMemory memory = allocate_memory(image);
	get_device().get_handle().bindImageMemory(image, memory, 0);
	vk::ImageView view =
	    vkb::common::create_image_view(get_device().get_handle(), image, vk::ImageViewType::e2D, format, vkb::common::get_image_aspect_flags(usage, format));

	return {format, image, memory, view};
}

vk::DescriptorPool HPPTimestampQueries::create_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 2> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 4}, {vk::DescriptorType::eCombinedImageSampler, 6}}};
	return get_device().get_handle().createDescriptorPool({{}, 4, pool_sizes});
}

vk::Pipeline HPPTimestampQueries::create_bloom_pipeline(uint32_t direction)
{
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{load_shader("hdr/bloom.vert", vk::ShaderStageFlagBits::eVertex),
	                                                             load_shader("hdr/bloom.frag", vk::ShaderStageFlagBits::eFragment)};

	// Set constant parameters via specialization constants
	vk::SpecializationMapEntry specialization_map_entry(0, 0, sizeof(uint32_t));

	vk::SpecializationInfo specialization_info(1, &specialization_map_entry, sizeof(uint32_t), &direction);
	shader_stages[1].pSpecializationInfo = &specialization_info;

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.blendEnable         = true;
	blend_attachment_state.colorBlendOp        = vk::BlendOp::eAdd;
	blend_attachment_state.srcColorBlendFactor = vk::BlendFactor::eOne;
	blend_attachment_state.dstColorBlendFactor = vk::BlendFactor::eOne;
	blend_attachment_state.alphaBlendOp        = vk::BlendOp::eAdd;
	blend_attachment_state.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	blend_attachment_state.dstAlphaBlendFactor = vk::BlendFactor::eDstAlpha;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp = vk::CompareOp::eGreater;
	depth_stencil_state.back.compareOp = vk::CompareOp::eAlways;
	depth_stencil_state.front          = depth_stencil_state.back;

	// Empty vertex input state, full screen triangles are generated by the vertex shader
	return vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
	                                             {},
	                                             vk::PrimitiveTopology::eTriangleList,
	                                             0,
	                                             vk::PolygonMode::eFill,
	                                             vk::CullModeFlagBits::eFront,
	                                             vk::FrontFace::eCounterClockwise,
	                                             {blend_attachment_state},
	                                             depth_stencil_state,
	                                             bloom.pipeline_layout,
	                                             direction == 1 ? render_pass : filter_pass.render_pass);
}

vk::Pipeline HPPTimestampQueries::create_composition_pipeline()
{
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{load_shader("hdr/composition.vert", vk::ShaderStageFlagBits::eVertex),
	                                                             load_shader("hdr/composition.frag", vk::ShaderStageFlagBits::eFragment)};

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp = vk::CompareOp::eGreater;
	depth_stencil_state.back.compareOp = vk::CompareOp::eAlways;
	depth_stencil_state.front          = depth_stencil_state.back;

	// Empty vertex input state, full screen triangles are generated by the vertex shader
	return vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
	                                             {},
	                                             vk::PrimitiveTopology::eTriangleList,
	                                             0,
	                                             vk::PolygonMode::eFill,
	                                             vk::CullModeFlagBits::eFront,
	                                             vk::FrontFace::eCounterClockwise,
	                                             {blend_attachment_state},
	                                             depth_stencil_state,
	                                             composition.pipeline_layout,
	                                             render_pass);
}

vk::RenderPass HPPTimestampQueries::create_filter_render_pass()
{
	// Set up separate renderpass with references to the color and depth attachments
	vk::AttachmentDescription attachment_description;
	attachment_description.samples        = vk::SampleCountFlagBits::e1;
	attachment_description.loadOp         = vk::AttachmentLoadOp::eClear;
	attachment_description.storeOp        = vk::AttachmentStoreOp::eStore;
	attachment_description.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
	attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachment_description.initialLayout  = vk::ImageLayout::eUndefined;
	attachment_description.finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;
	attachment_description.format         = filter_pass.color.format;

	vk::AttachmentReference color_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
	vk::SubpassDescription  subpass({}, vk::PipelineBindPoint::eGraphics, {}, color_reference);

	return create_render_pass({attachment_description}, subpass);
}

vk::Image HPPTimestampQueries::create_image(vk::Format format, vk::ImageUsageFlagBits usage)
{
	vk::ImageCreateInfo image_create_info;
	image_create_info.imageType   = vk::ImageType::e2D;
	image_create_info.format      = format;
	image_create_info.extent      = vk::Extent3D(offscreen.extent, 1);
	image_create_info.mipLevels   = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples     = vk::SampleCountFlagBits::e1;
	image_create_info.tiling      = vk::ImageTiling::eOptimal;
	image_create_info.usage       = usage | vk::ImageUsageFlagBits::eSampled;

	return get_device().get_handle().createImage(image_create_info);
}

vk::Pipeline HPPTimestampQueries::create_models_pipeline(uint32_t shaderType, vk::CullModeFlagBits cullMode, bool depthTestAndWrite)
{
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{load_shader("hdr/gbuffer.vert", vk::ShaderStageFlagBits::eVertex),
	                                                             load_shader("hdr/gbuffer.frag", vk::ShaderStageFlagBits::eFragment)};

	// Set constant parameters via specialization constants
	vk::SpecializationMapEntry specialization_map_entry(0, 0, sizeof(uint32_t));

	// Set constant parameters via specialization constants
	vk::SpecializationInfo specialization_info = vk::SpecializationInfo(1, &specialization_map_entry, sizeof(uint32_t), &shaderType);
	shader_stages[0].pSpecializationInfo       = &specialization_info;
	shader_stages[1].pSpecializationInfo       = &specialization_info;

	// Vertex bindings an attributes for model rendering
	// Binding description
	vk::VertexInputBindingDescription vertex_input_binding(0, sizeof(HPPVertex), vk::VertexInputRate::eVertex);

	// Attribute descriptions
	std::vector<vk::VertexInputAttributeDescription> vertex_input_attributes = {{0, 0, vk::Format::eR32G32B32Sfloat, 0},
	                                                                            {1, 0, vk::Format::eR32G32B32Sfloat, 3 * sizeof(float)}};

	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

	std::vector<vk::PipelineColorBlendAttachmentState> blend_attachment_states(2);
	blend_attachment_states[0].colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	blend_attachment_states[1].colorWriteMask = blend_attachment_states[0].colorWriteMask;

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp   = vk::CompareOp::eGreater;
	depth_stencil_state.depthWriteEnable = depthTestAndWrite;
	depth_stencil_state.depthTestEnable  = depthTestAndWrite;
	depth_stencil_state.back.compareOp   = vk::CompareOp::eAlways;
	depth_stencil_state.front            = depth_stencil_state.back;

	return vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
	                                             vertex_input_state,
	                                             vk::PrimitiveTopology::eTriangleList,
	                                             0,
	                                             vk::PolygonMode::eFill,
	                                             cullMode,
	                                             vk::FrontFace::eCounterClockwise,
	                                             blend_attachment_states,
	                                             depth_stencil_state,
	                                             models.pipeline_layout,
	                                             offscreen.render_pass);
}

vk::RenderPass HPPTimestampQueries::create_offscreen_render_pass()
{
	// Set up separate renderpass with references to the color and depth attachments
	std::vector<vk::AttachmentDescription> attachment_descriptions(3);

	// Init attachment properties
	for (uint32_t i = 0; i < 3; ++i)
	{
		attachment_descriptions[i].samples        = vk::SampleCountFlagBits::e1;
		attachment_descriptions[i].loadOp         = vk::AttachmentLoadOp::eClear;
		attachment_descriptions[i].storeOp        = vk::AttachmentStoreOp::eStore;
		attachment_descriptions[i].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
		attachment_descriptions[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachment_descriptions[i].initialLayout  = vk::ImageLayout::eUndefined;
		attachment_descriptions[i].finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;
	}
	attachment_descriptions[2].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	// Formats
	attachment_descriptions[0].format = offscreen.color[0].format;
	attachment_descriptions[1].format = offscreen.color[1].format;
	attachment_descriptions[2].format = offscreen.depth.format;

	std::array<vk::AttachmentReference, 2> color_references{{{0, vk::ImageLayout::eColorAttachmentOptimal},
	                                                         {1, vk::ImageLayout::eColorAttachmentOptimal}}};

	vk::AttachmentReference depth_reference(2, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, nullptr, color_references, nullptr, &depth_reference);

	return create_render_pass(attachment_descriptions, subpass);
}

vk::RenderPass HPPTimestampQueries::create_render_pass(std::vector<vk::AttachmentDescription> const &attachment_descriptions, vk::SubpassDescription const &subpass_description)
{
	// Use subpass dependencies for attachment layout transitions
	std::array<vk::SubpassDependency, 2> subpass_dependencies;

	subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependencies[0].dstSubpass = 0;
	// End of previous commands
	subpass_dependencies[0].srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
	subpass_dependencies[0].srcAccessMask = vk::AccessFlagBits::eNoneKHR;
	// Read/write from/to depth
	subpass_dependencies[0].dstStageMask  = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	subpass_dependencies[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	// Write to attachment
	subpass_dependencies[0].dstStageMask |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
	subpass_dependencies[0].dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;

	subpass_dependencies[1].srcSubpass = 0;
	subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	// End of write to attachment
	subpass_dependencies[1].srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	subpass_dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	// Attachment later read using sampler in 'bloom[0]' pipeline
	subpass_dependencies[1].dstStageMask  = vk::PipelineStageFlagBits::eFragmentShader;
	subpass_dependencies[1].dstAccessMask = vk::AccessFlagBits::eShaderRead;

	vk::RenderPassCreateInfo render_pass_create_info({}, attachment_descriptions, subpass_description, subpass_dependencies);

	return get_device().get_handle().createRenderPass(render_pass_create_info);
}

void HPPTimestampQueries::draw()
{
	HPPApiVulkanSample::prepare_frame();

	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();

	// Read back the time stamp query results after the frame is finished
	get_time_stamp_results();
}

void HPPTimestampQueries::get_time_stamp_results()
{
	// The number of timestamps changes if the bloom pass is disabled
	uint32_t count = static_cast<uint32_t>(bloom.enabled ? time_stamps.values.size() : time_stamps.values.size() - 2);

	// Fetch the time stamp results written in the command buffer submissions
	// A note on the flags used:
	//	vk::QueryResultFlagBits::e64: Results will have 64 bits. As time stamp values are on nano-seconds, this flag should always be used to avoid 32 bit overflows
	//  vk::QueryResultFlagBits::eWait: Since we want to immediately display the results, we use this flag to have the CPU wait until the results are available
	vk::Result result = get_device().get_handle().getQueryPoolResults(time_stamps.query_pool,
	                                                                  0,
	                                                                  count,
	                                                                  time_stamps.values.size() * sizeof(uint64_t),
	                                                                  time_stamps.values.data(),
	                                                                  sizeof(uint64_t),
	                                                                  vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);
	assert(result == vk::Result::eSuccess);
}

void HPPTimestampQueries::load_assets()
{
	// Models
	models.skybox.meshes.emplace_back(load_model("scenes/cube.gltf"));
	std::vector<std::string> filenames = {"geosphere.gltf", "teapot.gltf", "torusknot.gltf"};
	object_names                       = {"Sphere", "Teapot", "Torusknot"};
	for (auto file : filenames)
	{
		models.objects.meshes.emplace_back(load_model("scenes/" + file));
	}

	// Transforms
	auto geosphere_matrix = glm::mat4(1.0f);
	models.transforms.push_back(geosphere_matrix);

	auto teapot_matrix = glm::mat4(1.0f);
	teapot_matrix      = glm::scale(teapot_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	teapot_matrix      = glm::rotate(teapot_matrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	models.transforms.push_back(teapot_matrix);

	auto torus_matrix = glm::mat4(1.0f);
	models.transforms.push_back(torus_matrix);

	// Load HDR cube map
	textures.envmap = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx", vkb::scene_graph::components::HPPImage::Color);
}

void HPPTimestampQueries::prepare_bloom()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {{{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	                                                           {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::Device device           = get_device().get_handle();
	bloom.descriptor_set_layout = device.createDescriptorSetLayout({{}, bindings});
	bloom.pipeline_layout       = device.createPipelineLayout({{}, bloom.descriptor_set_layout});
	bloom.pipelines[0]          = create_bloom_pipeline(1);
	bloom.pipelines[1]          = create_bloom_pipeline(0);
	bloom.descriptor_set        = vkb::common::allocate_descriptor_set(device, descriptor_pool, bloom.descriptor_set_layout);
	update_bloom_descriptor_set();
}

void HPPTimestampQueries::prepare_camera()
{
	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -4.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 256.0f, 0.1f);
}

void HPPTimestampQueries::prepare_composition()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {{{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	                                                           {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::Device device                 = get_device().get_handle();
	composition.descriptor_set_layout = device.createDescriptorSetLayout({{}, bindings});
	composition.pipeline_layout       = device.createPipelineLayout({{}, composition.descriptor_set_layout});
	composition.pipeline              = create_composition_pipeline();
	composition.descriptor_set        = vkb::common::allocate_descriptor_set(device, descriptor_pool, composition.descriptor_set_layout);
	update_composition_descriptor_set();
}

void HPPTimestampQueries::prepare_models()
{
	std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {{{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	                                                           {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	                                                           {2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::Device device            = get_device().get_handle();
	models.descriptor_set_layout = device.createDescriptorSetLayout({{}, bindings});
	models.pipeline_layout       = device.createPipelineLayout({{}, models.descriptor_set_layout});

	models.objects.descriptor_set = vkb::common::allocate_descriptor_set(device, descriptor_pool, models.descriptor_set_layout);
	update_model_descriptor_set(models.objects.descriptor_set);
	models.objects.pipeline = create_models_pipeline(1, vk::CullModeFlagBits::eFront, true);

	models.skybox.descriptor_set = vkb::common::allocate_descriptor_set(device, descriptor_pool, models.descriptor_set_layout);
	update_model_descriptor_set(models.skybox.descriptor_set);
	models.skybox.pipeline = create_models_pipeline(0, vk::CullModeFlagBits::eBack, false);
}

// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
void HPPTimestampQueries::prepare_offscreen_buffer()
{
	{
		offscreen.extent = extent;

		// Color attachments

		// We are using two 128-Bit RGBA floating point color buffers for this sample
		// In a performance or bandwidth-limited scenario you should consider using a format with lower precision
		offscreen.color[0] = create_attachment(vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eColorAttachment);
		offscreen.color[1] = create_attachment(vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eColorAttachment);
		// Depth attachment
		offscreen.depth = create_attachment(depth_format, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		offscreen.render_pass = create_offscreen_render_pass();

		offscreen.framebuffer = vkb::common::create_framebuffer(
		    get_device().get_handle(), offscreen.render_pass, {offscreen.color[0].view, offscreen.color[1].view, offscreen.depth.view}, offscreen.extent);

		// Create sampler to sample from the color attachments
		offscreen.sampler = vkb::common::create_sampler(get_device().get_handle(), vk::Filter::eNearest, vk::SamplerAddressMode::eClampToEdge, 1.0f, 1.0f);
	}

	// Bloom separable filter pass
	{
		filter_pass.extent = extent;

		// Color attachments - needs to be a blendable format, so choose from a priority ordered list
		const std::vector<vk::Format> float_format_priority_list = {
		    vk::Format::eR32G32B32A32Sfloat,
		    vk::Format::eR16G16B16A16Sfloat        // Guaranteed blend support for this
		};

		vk::Format color_format = vkb::common::choose_blendable_format(get_device().get_gpu().get_handle(), float_format_priority_list);

		// One floating point color buffer
		filter_pass.color = create_attachment(color_format, vk::ImageUsageFlagBits::eColorAttachment);

		filter_pass.render_pass = create_filter_render_pass();
		filter_pass.framebuffer =
		    vkb::common::create_framebuffer(get_device().get_handle(), filter_pass.render_pass, {filter_pass.color.view}, filter_pass.extent);
		filter_pass.sampler = vkb::common::create_sampler(get_device().get_handle(), vk::Filter::eNearest, vk::SamplerAddressMode::eClampToEdge, 1.0f, 1.0f);
	}
}

void HPPTimestampQueries::prepare_time_stamps()
{
	// Create the query pool object used to get the GPU time tamps
	time_stamps.query_pool =
	    vkb::common::create_query_pool(get_device().get_handle(), vk::QueryType::eTimestamp, static_cast<uint32_t>(time_stamps.values.size()));
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPTimestampQueries::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffers.matrices = std::make_unique<vkb::core::HPPBuffer>(get_device(),
	                                                                  sizeof(ubo_matrices),
	                                                                  vk::BufferUsageFlagBits::eUniformBuffer,
	                                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Params
	uniform_buffers.params = std::make_unique<vkb::core::HPPBuffer>(get_device(),
	                                                                sizeof(ubo_params),
	                                                                vk::BufferUsageFlagBits::eUniformBuffer,
	                                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
	update_params();
}

void HPPTimestampQueries::update_composition_descriptor_set()
{
	std::array<vk::DescriptorImageInfo, 2> color_descriptors = {{{offscreen.sampler, offscreen.color[0].view, vk::ImageLayout::eShaderReadOnlyOptimal},
	                                                             {offscreen.sampler, filter_pass.color.view, vk::ImageLayout::eShaderReadOnlyOptimal}}};

	std::array<vk::WriteDescriptorSet, 2> sampler_write_descriptor_sets = {
	    {{composition.descriptor_set, 0, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[0]},
	     {composition.descriptor_set, 1, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[1]}}};

	get_device().get_handle().updateDescriptorSets(sampler_write_descriptor_sets, {});
}

void HPPTimestampQueries::update_bloom_descriptor_set()
{
	std::array<vk::DescriptorImageInfo, 2> color_descriptors = {{{offscreen.sampler, offscreen.color[0].view, vk::ImageLayout::eShaderReadOnlyOptimal},
	                                                             {offscreen.sampler, offscreen.color[1].view, vk::ImageLayout::eShaderReadOnlyOptimal}}};

	std::array<vk::WriteDescriptorSet, 2> sampler_write_descriptor_sets = {
	    {{bloom.descriptor_set, 0, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[0]},
	     {bloom.descriptor_set, 1, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[1]}}};

	get_device().get_handle().updateDescriptorSets(sampler_write_descriptor_sets, {});
}

void HPPTimestampQueries::update_model_descriptor_set(vk::DescriptorSet descriptor_set)
{
	vk::DescriptorBufferInfo matrix_buffer_descriptor(uniform_buffers.matrices->get_handle(), 0, VK_WHOLE_SIZE);

	vk::DescriptorImageInfo environment_image_descriptor(
	    textures.envmap.sampler,
	    textures.envmap.image->get_vk_image_view().get_handle(),
	    descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, textures.envmap.image->get_vk_image_view().get_format()));

	vk::DescriptorBufferInfo params_buffer_descriptor(uniform_buffers.params->get_handle(), 0, VK_WHOLE_SIZE);

	std::array<vk::WriteDescriptorSet, 3> write_descriptor_sets = {
	    {{descriptor_set, 0, 0, vk::DescriptorType::eUniformBuffer, {}, matrix_buffer_descriptor},
	     {descriptor_set, 1, 0, vk::DescriptorType::eCombinedImageSampler, environment_image_descriptor},
	     {descriptor_set, 2, 0, vk::DescriptorType::eUniformBuffer, {}, params_buffer_descriptor}}};

	get_device().get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPTimestampQueries::update_params()
{
	uniform_buffers.params->convert_and_update(ubo_params);
}

void HPPTimestampQueries::update_uniform_buffers()
{
	ubo_matrices.projection       = camera.matrices.perspective;
	ubo_matrices.modelview        = camera.matrices.view * models.transforms[models.object_index];
	ubo_matrices.skybox_modelview = camera.matrices.view;
	uniform_buffers.matrices->convert_and_update(ubo_matrices);
}

std::unique_ptr<vkb::Application> create_hpp_timestamp_queries()
{
	return std::make_unique<HPPTimestampQueries>();
}
