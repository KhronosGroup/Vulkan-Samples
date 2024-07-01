/* Copyright (c) 2024, Google
 * Copyright (c) 2024, NVIDIA
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

#include "hpp_oit_depth_peeling.h"

HPPOITDepthPeeling::HPPOITDepthPeeling()
{}

HPPOITDepthPeeling::~HPPOITDepthPeeling()
{
	if (has_device())
	{
		vk::Device device = get_device().get_handle();
		background.destroy(device);
		combinePass.destroy(device);
		for (auto &d : depths)
		{
			d.destroy();
		}
		device.destroyDescriptorPool(descriptor_pool);
		gatherPass.destroy(device);
		for (auto &l : layers)
		{
			l.destroy(device);
		}
		model.reset();
		device.destroySampler(point_sampler);
		scene_constants.reset();
	}
}

bool HPPOITDepthPeeling::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);

	if (HPPApiVulkanSample::prepare(options))
	{
		prepare_camera();
		load_assets();
		create_point_sampler();
		create_scene_constants_buffer();
		create_descriptor_pool();
		create_combine_pass();
		create_images(extent.width, extent.height);
		create_gather_pass();
		create_background_pipeline();
		update_scene_constants();
		update_descriptors();
		build_command_buffers();

		prepared = true;
	}
	return prepared;
}

bool HPPOITDepthPeeling::resize(const uint32_t width, const uint32_t height)
{
	create_images(width, height);
	create_gather_pass_framebuffers(width, height);
	update_descriptors();
	return HPPApiVulkanSample::resize(width, height);
}

void HPPOITDepthPeeling::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
	else
	{
		throw std::runtime_error("This sample requires support for anisotropic sampling");
	}
}

void HPPOITDepthPeeling::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	std::array<vk::ClearValue, 2> clear_values = {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
	                                               vk::ClearDepthStencilValue(0.0f, 0)}};

	vk::RenderPassBeginInfo render_pass_begin_info({}, {}, {{0, 0}, extent}, clear_values);
	vk::Viewport            viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
	vk::Rect2D              scissor({0, 0}, extent);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		auto const &command_buffer = draw_cmd_buffers[i];
		command_buffer.begin(command_buffer_begin_info);
		{
			// Gather passes
			// Each pass renders a single transparent layer into a layer texture.
			for (uint32_t l = 0; l <= gui.layer_index_back; ++l)
			{
				// Two depth textures are used.
				// Their roles alternates for each pass.
				// The first depth texture is used for fixed-function depth test.
				// The second one is the result of the depth test from the previous gatherPass pass.
				// It is bound as texture and read in the shader to discard fragments from the
				// previous layers.
				vk::ImageSubresourceRange depth_subresource_range = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};
				vkb::common::image_layout_transition(command_buffer,
				                                     depths[l % kDepthCount].image->get_handle(),
				                                     vk::PipelineStageFlagBits::eFragmentShader,
				                                     vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
				                                     vk::AccessFlagBits::eShaderRead,
				                                     vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
				                                     l <= 1 ? vk::ImageLayout::eUndefined : vk::ImageLayout::eDepthStencilReadOnlyOptimal,
				                                     vk::ImageLayout::eDepthStencilAttachmentOptimal,
				                                     depth_subresource_range);
				if (l > 0)
				{
					vkb::common::image_layout_transition(command_buffer,
					                                     depths[(l + 1) % kDepthCount].image->get_handle(),
					                                     vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
					                                     vk::PipelineStageFlagBits::eFragmentShader,
					                                     vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
					                                     vk::AccessFlagBits::eShaderRead,
					                                     vk::ImageLayout::eDepthStencilAttachmentOptimal,
					                                     vk::ImageLayout::eDepthStencilReadOnlyOptimal,
					                                     depth_subresource_range);
				}

				// Set one of the layer textures as color attachment, as the gatherPass pass will render to it.
				vk::ImageSubresourceRange layer_subresource_range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
				vkb::common::image_layout_transition(command_buffer,
				                                     layers[l].image->get_handle(),
				                                     vk::PipelineStageFlagBits::eFragmentShader,
				                                     vk::PipelineStageFlagBits::eColorAttachmentOutput,
				                                     vk::AccessFlagBits::eShaderRead,
				                                     vk::AccessFlagBits::eColorAttachmentWrite,
				                                     vk::ImageLayout::eUndefined,
				                                     vk::ImageLayout::eColorAttachmentOptimal,
				                                     layer_subresource_range);

				render_pass_begin_info.framebuffer = layers[l].gather_framebuffer;
				render_pass_begin_info.renderPass  = gatherPass.render_pass;
				command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
				{
					// Render the geometry into the layer texture.
					command_buffer.setViewport(0, viewport);
					command_buffer.setScissor(0, scissor);

					command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, gatherPass.pipeline_layout, 0, depths[l % kDepthCount].gather_descriptor_set, {});

					command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, l == 0 ? gatherPass.first_pipeline : gatherPass.pipeline);
					draw_model(model, command_buffer);
				}
				command_buffer.endRenderPass();

				// Get the layer texture ready to be read by the combinePass pass.
				vkb::common::image_layout_transition(command_buffer,
				                                     layers[l].image->get_handle(),
				                                     vk::PipelineStageFlagBits::eColorAttachmentOutput,
				                                     vk::PipelineStageFlagBits::eFragmentShader,
				                                     vk::AccessFlagBits::eColorAttachmentWrite,
				                                     vk::AccessFlagBits::eShaderRead,
				                                     vk::ImageLayout::eColorAttachmentOptimal,
				                                     vk::ImageLayout::eShaderReadOnlyOptimal,
				                                     layer_subresource_range);
			}

			// Combine pass
			// This pass blends all the layers into the final transparent color.
			// The final color is then alpha blended into the background.
			render_pass_begin_info.framebuffer = framebuffers[i];
			render_pass_begin_info.renderPass  = render_pass;
			command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
			{
				command_buffer.setViewport(0, viewport);
				command_buffer.setScissor(0, scissor);

				command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, combinePass.pipeline_layout, 0, combinePass.descriptor_set, {});

				command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, background.pipeline);
				command_buffer.draw(3, 1, 0, 0);

				command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, combinePass.pipeline);
				command_buffer.draw(3, 1, 0, 0);

				draw_ui(command_buffer);
			}
			command_buffer.endRenderPass();
		}
		command_buffer.end();
	}
}

void HPPOITDepthPeeling::on_update_ui_overlay(vkb::Drawer &drawer)
{
	drawer.checkbox("Camera auto-rotation", &gui.camera_auto_rotation);
	drawer.slider_float("Background grayscale", &gui.background_grayscale, kBackgroundGrayscaleMin, kBackgroundGrayscaleMax);
	drawer.slider_float("Object opacity", &gui.object_opacity, kObjectAlphaMin, kObjectAlphaMax);

	drawer.slider_int("Front layer index", &gui.layer_index_front, 0, gui.layer_index_back);
	drawer.slider_int("Back layer index", &gui.layer_index_back, gui.layer_index_front, kLayerMaxCount - 1);
}

void HPPOITDepthPeeling::render(float delta_time)
{
	if (prepared)
	{
		HPPApiVulkanSample::prepare_frame();
		submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);
		queue.submit(submit_info);
		HPPApiVulkanSample::submit_frame();

		if (gui.camera_auto_rotation)
		{
			camera.rotate({delta_time * 5.0f, delta_time * 5.0f, 0.0f});
		}
		update_scene_constants();
	}
}

////////////////////////////////////////////////////////////////////////////////

void HPPOITDepthPeeling::create_background_pipeline()
{
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {load_shader("oit_depth_peeling/fullscreen.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                load_shader("oit_depth_peeling/background.frag", vk::ShaderStageFlagBits::eFragment)};

	vk::VertexInputBindingDescription                  vertex_input_binding(0, sizeof(HPPVertex), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes = {{{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(HPPVertex, pos)},
	                                                                               {1, 0, vk::Format::eR32G32Sfloat, offsetof(HPPVertex, uv)}}};

	vk::PipelineColorBlendAttachmentState blend_attachment_state(false, {}, {}, {}, {}, {}, {}, vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state({}, false, false, vk::CompareOp::eGreater, {}, {}, {}, {{}, {}, {}, vk::CompareOp::eAlways});

	background.pipeline = vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                                            pipeline_cache,
	                                                            shader_stages,
	                                                            {},
	                                                            vk::PrimitiveTopology::eTriangleList,
	                                                            {},
	                                                            vk::PolygonMode::eFill,
	                                                            vk::CullModeFlagBits::eNone,
	                                                            vk::FrontFace::eCounterClockwise,
	                                                            {blend_attachment_state},
	                                                            depth_stencil_state,
	                                                            combinePass.pipeline_layout,
	                                                            render_pass);
}

void HPPOITDepthPeeling::create_combine_pass()
{
	vk::Device device = get_device().get_handle();

	std::array<vk::DescriptorSetLayoutBinding, 3> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	     {2, vk::DescriptorType::eCombinedImageSampler, kLayerMaxCount, vk::ShaderStageFlagBits::eFragment}}};
	combinePass.descriptor_set_layout = device.createDescriptorSetLayout({{}, set_layout_bindings});

	combinePass.descriptor_set = vkb::common::allocate_descriptor_set(device, descriptor_pool, combinePass.descriptor_set_layout);

	combinePass.pipeline_layout = device.createPipelineLayout({{}, combinePass.descriptor_set_layout});

	create_combine_pass_pipeline();
}

void HPPOITDepthPeeling::create_combine_pass_pipeline()
{
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {load_shader("oit_depth_peeling/fullscreen.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                load_shader("oit_depth_peeling/combine.frag", vk::ShaderStageFlagBits::eFragment)};

	vk::VertexInputBindingDescription                  vertex_input_binding(0, sizeof(HPPVertex), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes = {{{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(HPPVertex, pos)},
	                                                                               {1, 0, vk::Format::eR32G32Sfloat, offsetof(HPPVertex, uv)}}};

	vk::PipelineColorBlendAttachmentState blend_attachment_state(true,
	                                                             vk::BlendFactor::eSrcAlpha,
	                                                             vk::BlendFactor::eOneMinusSrcColor,
	                                                             vk::BlendOp::eAdd,
	                                                             vk::BlendFactor::eOne,
	                                                             vk::BlendFactor::eZero,
	                                                             vk::BlendOp::eAdd,
	                                                             vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state({}, false, false, vk::CompareOp::eGreater, {}, {}, {}, {{}, {}, {}, vk::CompareOp::eAlways});

	combinePass.pipeline = vkb::common::create_graphics_pipeline(get_device().get_handle(),
	                                                             pipeline_cache,
	                                                             shader_stages,
	                                                             {},
	                                                             vk::PrimitiveTopology::eTriangleList,
	                                                             {},
	                                                             vk::PolygonMode::eFill,
	                                                             vk::CullModeFlagBits::eNone,
	                                                             vk::FrontFace::eCounterClockwise,
	                                                             {blend_attachment_state},
	                                                             depth_stencil_state,
	                                                             combinePass.pipeline_layout,
	                                                             render_pass);
}

void HPPOITDepthPeeling::create_descriptor_pool()
{
	const uint32_t num_gather_pass_combined_image_sampler = kDepthCount;
	const uint32_t num_gather_pass_uniform_buffer         = kDepthCount;

	const uint32_t num_combine_pass_combined_image_sampler = kLayerMaxCount + 1;
	const uint32_t num_combine_pass_uniform_buffer         = 1;

	const uint32_t                        num_uniform_buffer_descriptors         = num_gather_pass_uniform_buffer + num_combine_pass_uniform_buffer;
	const uint32_t                        num_combined_image_sampler_descriptors = num_gather_pass_combined_image_sampler + num_combine_pass_combined_image_sampler;
	std::array<vk::DescriptorPoolSize, 2> pool_sizes                             = {{{vk::DescriptorType::eUniformBuffer, num_uniform_buffer_descriptors},
	                                                                                 {vk::DescriptorType::eCombinedImageSampler, num_combined_image_sampler_descriptors}}};
	const uint32_t                        num_gather_descriptor_sets             = 2;
	const uint32_t                        num_combine_descriptor_sets            = 1;
	const uint32_t                        num_descriptor_sets                    = num_gather_descriptor_sets + num_combine_descriptor_sets;
	vk::DescriptorPoolCreateInfo          descriptor_pool_create_info({}, num_descriptor_sets, pool_sizes);

	descriptor_pool = get_device().get_handle().createDescriptorPool(descriptor_pool_create_info);
}

void HPPOITDepthPeeling::create_gather_pass()
{
	create_gather_pass_descriptor_set_layout();
	create_gather_pass_render_pass();
	create_gather_pass_depth_descriptor_sets();
	create_gather_pass_framebuffers(extent.width, extent.height);
	gatherPass.pipeline_layout = get_device().get_handle().createPipelineLayout({{}, gatherPass.descriptor_set_layout});
	create_gather_pass_pipelines();
}

void HPPOITDepthPeeling::create_gather_pass_depth_descriptor_sets()
{
	vk::Device device = get_device().get_handle();
	for (auto &d : depths)
	{
		d.gather_descriptor_set = vkb::common::allocate_descriptor_set(device, descriptor_pool, gatherPass.descriptor_set_layout);
	}
}

void HPPOITDepthPeeling::create_gather_pass_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};
	gatherPass.descriptor_set_layout = get_device().get_handle().createDescriptorSetLayout({{}, set_layout_bindings});
}

void HPPOITDepthPeeling::create_gather_pass_framebuffers(const uint32_t width, const uint32_t height)
{
	vk::Device                device = get_device().get_handle();
	vk::FramebufferCreateInfo framebuffer_create_info({}, {}, {}, width, height, 1);
	for (uint32_t i = 0; i < kLayerMaxCount; ++i)
	{
		if (layers[i].gather_framebuffer)
		{
			device.destroyFramebuffer(layers[i].gather_framebuffer);
		}
		framebuffer_create_info.renderPass = gatherPass.render_pass;
		const std::array<vk::ImageView, 2> attachments{layers[i].image_view->get_handle(), depths[i % kDepthCount].image_view->get_handle()};
		framebuffer_create_info.setAttachments(attachments);
		layers[i].gather_framebuffer = device.createFramebuffer(framebuffer_create_info);
	}
}

void HPPOITDepthPeeling::create_gather_pass_pipelines()
{
	vk::Device device = get_device().get_handle();

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {load_shader("oit_depth_peeling/gather.vert", vk::ShaderStageFlagBits::eVertex),
	                                                                load_shader("oit_depth_peeling/gather_first.frag", vk::ShaderStageFlagBits::eFragment)};

	vk::VertexInputBindingDescription                  vertex_input_binding(0, sizeof(HPPVertex), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes = {{{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(HPPVertex, pos)},
	                                                                               {1, 0, vk::Format::eR32G32Sfloat, offsetof(HPPVertex, uv)}}};
	vk::PipelineVertexInputStateCreateInfo             vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

	vk::PipelineColorBlendAttachmentState blend_attachment_state(false, {}, {}, {}, {}, {}, {}, vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state({}, true, true, vk::CompareOp::eGreater, {}, {}, {}, {{}, {}, {}, vk::CompareOp::eAlways});

	gatherPass.first_pipeline = vkb::common::create_graphics_pipeline(device,
	                                                                  pipeline_cache,
	                                                                  shader_stages,
	                                                                  vertex_input_state,
	                                                                  vk::PrimitiveTopology::eTriangleList,
	                                                                  {},
	                                                                  vk::PolygonMode::eFill,
	                                                                  vk::CullModeFlagBits::eNone,
	                                                                  vk::FrontFace::eCounterClockwise,
	                                                                  {blend_attachment_state},
	                                                                  depth_stencil_state,
	                                                                  gatherPass.pipeline_layout,
	                                                                  gatherPass.render_pass);

	shader_stages[1] = load_shader("oit_depth_peeling/gather.frag", vk::ShaderStageFlagBits::eFragment);

	gatherPass.pipeline = vkb::common::create_graphics_pipeline(device,
	                                                            pipeline_cache,
	                                                            shader_stages,
	                                                            vertex_input_state,
	                                                            vk::PrimitiveTopology::eTriangleList,
	                                                            {},
	                                                            vk::PolygonMode::eFill,
	                                                            vk::CullModeFlagBits::eNone,
	                                                            vk::FrontFace::eCounterClockwise,
	                                                            {blend_attachment_state},
	                                                            depth_stencil_state,
	                                                            gatherPass.pipeline_layout,
	                                                            gatherPass.render_pass);
}

void HPPOITDepthPeeling::create_gather_pass_render_pass()
{
	std::array<vk::AttachmentDescription, 2> attachment_descriptions = {{{{},
	                                                                      vk::Format::eR8G8B8A8Unorm,
	                                                                      vk::SampleCountFlagBits::e1,
	                                                                      vk::AttachmentLoadOp::eClear,
	                                                                      vk::AttachmentStoreOp::eStore,
	                                                                      vk::AttachmentLoadOp::eDontCare,
	                                                                      vk::AttachmentStoreOp::eDontCare,
	                                                                      vk::ImageLayout::eUndefined,
	                                                                      vk::ImageLayout::eColorAttachmentOptimal},
	                                                                     {{},
	                                                                      vk::Format::eD32Sfloat,
	                                                                      vk::SampleCountFlagBits::e1,
	                                                                      vk::AttachmentLoadOp::eClear,
	                                                                      vk::AttachmentStoreOp::eStore,
	                                                                      vk::AttachmentLoadOp::eDontCare,
	                                                                      vk::AttachmentStoreOp::eDontCare,
	                                                                      vk::ImageLayout::eUndefined,
	                                                                      vk::ImageLayout::eDepthStencilAttachmentOptimal}}};

	vk::AttachmentReference color_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depth_attachment_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, color_attachment_reference, {}, &depth_attachment_reference);

	vk::RenderPassCreateInfo render_pass_create_info({}, attachment_descriptions, subpass);
	gatherPass.render_pass = get_device().get_handle().createRenderPass(render_pass_create_info);
}

void HPPOITDepthPeeling::create_images(const uint32_t width, const uint32_t height)
{
	const vk::Extent3D image_extent = {width, height, 1};
	for (uint32_t i = 0; i < kLayerMaxCount; ++i)
	{
		layers[i].image      = std::make_unique<vkb::core::HPPImage>(get_device(),
                                                                image_extent,
                                                                vk::Format::eR8G8B8A8Unorm,
                                                                vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
                                                                VMA_MEMORY_USAGE_GPU_ONLY,
                                                                vk::SampleCountFlagBits::e1);
		layers[i].image_view = std::make_unique<vkb::core::HPPImageView>(*layers[i].image, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm);
	}

	for (uint32_t i = 0; i < kDepthCount; ++i)
	{
		depths[i].image      = std::make_unique<vkb::core::HPPImage>(get_device(),
                                                                image_extent,
                                                                vk::Format::eD32Sfloat,
                                                                vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                                VMA_MEMORY_USAGE_GPU_ONLY,
                                                                vk::SampleCountFlagBits::e1);
		depths[i].image_view = std::make_unique<vkb::core::HPPImageView>(*depths[i].image, vk::ImageViewType::e2D, vk::Format::eD32Sfloat);
	}
}

void HPPOITDepthPeeling::create_point_sampler()
{
	point_sampler = vkb::common::create_sampler(
	    get_device().get_handle(), vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge, 1.0f, 1.0f);
}

void HPPOITDepthPeeling::create_scene_constants_buffer()
{
	scene_constants =
	    std::make_unique<vkb::core::HPPBuffer>(get_device(), sizeof(SceneConstants), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void HPPOITDepthPeeling::load_assets()
{
	model              = load_model("scenes/torusknot.gltf");
	background.texture = load_texture("textures/vulkan_logo_full.ktx", vkb::scene_graph::components::HPPImage::Color);
}

void HPPOITDepthPeeling::prepare_camera()
{
	camera.type = vkb::CameraType::LookAt;
	camera.set_position({0.0f, 0.0f, -4.0f});
	camera.set_rotation({0.0f, 0.0f, 0.0f});
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 16.0f, 0.1f);
}

void HPPOITDepthPeeling::update_descriptors()
{
	vk::Device device = get_device().get_handle();

	vk::DescriptorBufferInfo scene_constants_descriptor(scene_constants->get_handle(), 0, VK_WHOLE_SIZE);

	for (uint32_t i = 0; i < kDepthCount; ++i)
	{
		vk::DescriptorImageInfo depth_texture_descriptor(
		    point_sampler, depths[(i + 1) % kDepthCount].image_view->get_handle(), vk::ImageLayout::eDepthStencilReadOnlyOptimal);

		std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {
		    {{depths[i].gather_descriptor_set, 0, {}, vk::DescriptorType::eUniformBuffer, {}, scene_constants_descriptor},
		     {depths[i].gather_descriptor_set, 1, {}, vk::DescriptorType::eCombinedImageSampler, depth_texture_descriptor}}};
		device.updateDescriptorSets(write_descriptor_sets, {});
	}

	vk::DescriptorImageInfo background_texture_descriptor(
	    background.texture.sampler, background.texture.image->get_vk_image_view().get_handle(), vk::ImageLayout::eShaderReadOnlyOptimal);

	std::array<vk::DescriptorImageInfo, kLayerMaxCount> layer_texture_descriptor;
	for (uint32_t i = 0; i < kLayerMaxCount; ++i)
	{
		layer_texture_descriptor[i].sampler     = point_sampler;
		layer_texture_descriptor[i].imageView   = layers[i].image_view->get_handle();
		layer_texture_descriptor[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	}

	std::array<vk::WriteDescriptorSet, 3> write_descriptor_sets = {
	    {{combinePass.descriptor_set, 0, {}, vk::DescriptorType::eUniformBuffer, {}, scene_constants_descriptor},
	     {combinePass.descriptor_set, 1, {}, vk::DescriptorType::eCombinedImageSampler, background_texture_descriptor},
	     {combinePass.descriptor_set, 2, {}, vk::DescriptorType::eCombinedImageSampler, layer_texture_descriptor}}};

	device.updateDescriptorSets(write_descriptor_sets, {});
}

void HPPOITDepthPeeling::update_scene_constants()
{
	SceneConstants constants        = {};
	constants.model_view_projection = camera.matrices.perspective * camera.matrices.view * glm::scale(glm::mat4(1.0f), glm::vec3(0.08));
	constants.background_grayscale  = gui.background_grayscale;
	constants.object_opacity        = gui.object_opacity;
	constants.front_layer_index     = gui.layer_index_front;
	constants.back_layer_index      = gui.layer_index_back;
	scene_constants->convert_and_update(constants);
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::Cpp>> create_hpp_oit_depth_peeling()
{
	return std::make_unique<HPPOITDepthPeeling>();
}
