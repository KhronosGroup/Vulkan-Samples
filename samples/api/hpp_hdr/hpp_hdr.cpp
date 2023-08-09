/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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
 * High dynamic range rendering, using vulkan.hpp
 */

#include "hpp_hdr.h"

HPPHDR::HPPHDR()
{
	title = "HPP High dynamic range rendering";
}

HPPHDR::~HPPHDR()
{
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		bloom.destroy(device);
		composition.destroy(device);
		filter_pass.destroy(device);
		models.destroy(device, descriptor_pool);
		offscreen.destroy(device);
		textures.destroy(device);
	}
}

bool HPPHDR::prepare(const vkb::ApplicationOptions &options)
{
	if (!HPPApiVulkanSample::prepare(options))
	{
		return false;
	}

	prepare_camera();
	load_assets();
	prepare_uniform_buffers();
	prepare_offscreen_buffer();

	descriptor_pool = create_descriptor_pool();
	setup_bloom();
	setup_composition();
	setup_models();
	build_command_buffers();
	prepared = true;
	return true;
}

bool HPPHDR::resize(const uint32_t width, const uint32_t height)
{
	HPPApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

void HPPHDR::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

void HPPHDR::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		vk::CommandBuffer command_buffer = draw_cmd_buffers[i];
		command_buffer.begin(command_buffer_begin_info);

		{
			/*
			    First pass: Render scene to offscreen framebuffer
			*/
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
		}

		/*
		    Second render pass: First bloom pass
		*/
		if (bloom.enabled)
		{
			// Bloom filter
			vk::ClearValue          clear_value(vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})));
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
		}

		/*
		    Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
		*/

		/*
		    Third render pass: Scene rendering with applied second bloom pass (when enabled)
		*/
		{
			// Final composition
			std::array<vk::ClearValue, 2> clear_values = {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			                                               vk::ClearDepthStencilValue(0.0f, 0)}};
			vk::RenderPassBeginInfo       render_pass_begin_info(render_pass, framebuffers[i], {{0, 0}, extent}, clear_values);
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
		}

		command_buffer.end();
	}
}

void HPPHDR::on_update_ui_overlay(vkb::HPPDrawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("Object type", &models.object_index, object_names))
		{
			update_uniform_buffers();
			rebuild_command_buffers();
		}
		if (drawer.input_float("Exposure", &ubo_params.exposure, 0.025f, 3))
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
}

void HPPHDR::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

vk::DeviceMemory HPPHDR::allocate_memory(vk::Image image)
{
	vk::MemoryRequirements memory_requirements = get_device()->get_handle().getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size,
	                                            get_device()->get_gpu().get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	return get_device()->get_handle().allocateMemory(memory_allocate_info);
}

HPPHDR::FrameBufferAttachment HPPHDR::create_attachment(vk::Format format, vk::ImageUsageFlagBits usage)
{
	vk::Image        image  = create_image(format, usage);
	vk::DeviceMemory memory = allocate_memory(image);
	get_device()->get_handle().bindImageMemory(image, memory, 0);
	vk::ImageView view = create_image_view(format, usage, image);

	return {format, image, memory, view};
}

vk::DescriptorPool HPPHDR::create_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 2> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 4}, {vk::DescriptorType::eCombinedImageSampler, 6}}};
	return get_device()->get_handle().createDescriptorPool({{}, 4, pool_sizes});
}

vk::Pipeline HPPHDR::create_bloom_pipeline(uint32_t direction)
{
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
	    load_shader("hdr/bloom.vert", vk::ShaderStageFlagBits::eVertex),
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
	return vkb::common::create_graphics_pipeline(get_device()->get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
	                                             {},
	                                             vk::PrimitiveTopology::eTriangleList,
	                                             vk::CullModeFlagBits::eFront,
	                                             vk::FrontFace::eCounterClockwise,
	                                             {blend_attachment_state},
	                                             depth_stencil_state,
	                                             bloom.pipeline_layout,
	                                             render_pass);
}

vk::Pipeline HPPHDR::create_composition_pipeline()
{
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{load_shader("hdr/composition.vert", vk::ShaderStageFlagBits::eVertex),
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
	return vkb::common::create_graphics_pipeline(get_device()->get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
	                                             {},
	                                             vk::PrimitiveTopology::eTriangleList,
	                                             vk::CullModeFlagBits::eFront,
	                                             vk::FrontFace::eCounterClockwise,
	                                             {blend_attachment_state},
	                                             depth_stencil_state,
	                                             composition.pipeline_layout,
	                                             render_pass);
}

vk::RenderPass HPPHDR::create_filter_render_pass()
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

vk::Image HPPHDR::create_image(vk::Format format, vk::ImageUsageFlagBits usage)
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

	return get_device()->get_handle().createImage(image_create_info);
}

vk::ImageView HPPHDR::create_image_view(vk::Format format, vk::ImageUsageFlagBits usage, vk::Image image)
{
	vk::ImageAspectFlags aspect_mask = vkb::common::get_image_aspect_flags(usage, format);

	vk::ImageViewCreateInfo image_view_create_info({}, image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1});

	return get_device()->get_handle().createImageView(image_view_create_info);
}

vk::Pipeline HPPHDR::create_models_pipeline(uint32_t shaderType, vk::CullModeFlagBits cullMode, bool depthTestAndWrite)
{
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
	    load_shader("hdr/gbuffer.vert", vk::ShaderStageFlagBits::eVertex),
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

	return vkb::common::create_graphics_pipeline(get_device()->get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
	                                             vertex_input_state,
	                                             vk::PrimitiveTopology::eTriangleList,
	                                             cullMode,
	                                             vk::FrontFace::eCounterClockwise,
	                                             blend_attachment_states,
	                                             depth_stencil_state,
	                                             models.pipeline_layout,
	                                             offscreen.render_pass);
}

vk::RenderPass HPPHDR::create_offscreen_render_pass()
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

vk::RenderPass HPPHDR::create_render_pass(std::vector<vk::AttachmentDescription> const &attachment_descriptions, vk::SubpassDescription const &subpass_description)
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

	return get_device()->get_handle().createRenderPass(render_pass_create_info);
}

vk::Sampler HPPHDR::create_sampler()
{
	vk::SamplerCreateInfo sampler_create_info;
	sampler_create_info.magFilter     = vk::Filter::eNearest;
	sampler_create_info.minFilter     = vk::Filter::eNearest;
	sampler_create_info.mipmapMode    = vk::SamplerMipmapMode::eLinear;
	sampler_create_info.addressModeU  = vk::SamplerAddressMode::eClampToEdge;
	sampler_create_info.addressModeV  = sampler_create_info.addressModeU;
	sampler_create_info.addressModeW  = sampler_create_info.addressModeU;
	sampler_create_info.mipLodBias    = 0.0f;
	sampler_create_info.maxAnisotropy = 1.0f;
	sampler_create_info.minLod        = 0.0f;
	sampler_create_info.maxLod        = 1.0f;
	sampler_create_info.borderColor   = vk::BorderColor::eFloatOpaqueWhite;

	return get_device()->get_handle().createSampler(sampler_create_info);
}

void HPPHDR::draw()
{
	HPPApiVulkanSample::prepare_frame();

	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

void HPPHDR::load_assets()
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
	textures.envmap = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx", vkb::sg::Image::Color);
}

void HPPHDR::prepare_camera()
{
	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -4.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 256.0f, 0.1f);
}

// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
void HPPHDR::prepare_offscreen_buffer()
{
	// We need to select a format that supports the color attachment blending flag, so we iterate over multiple formats to find one that supports this flag
	const std::vector<vk::Format> float_format_priority_list = {vk::Format::eR32G32B32A32Sfloat, vk::Format::eR16G16B16A16Sfloat};

	auto formatIt = std::find_if(float_format_priority_list.begin(),
	                             float_format_priority_list.end(),
	                             [this](vk::Format format) {
		                             const vk::FormatProperties properties = get_device()->get_gpu().get_handle().getFormatProperties(format);
		                             return properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachmentBlend;
	                             });
	if (formatIt == float_format_priority_list.end())
	{
		throw std::runtime_error("No suitable float format could be determined");
	}

	vk::Format color_format = *formatIt;

	{
		offscreen.extent = extent;

		// Color attachments

		// We are using two 128-Bit RGBA floating point color buffers for this sample
		// In a performance or bandwidth-limited scenario you should consider using a format with lower precision
		offscreen.color[0] = create_attachment(color_format, vk::ImageUsageFlagBits::eColorAttachment);
		offscreen.color[1] = create_attachment(color_format, vk::ImageUsageFlagBits::eColorAttachment);
		// Depth attachment
		offscreen.depth = create_attachment(depth_format, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		offscreen.render_pass = create_offscreen_render_pass();

		offscreen.framebuffer = vkb::common::create_framebuffer(
		    get_device()->get_handle(), offscreen.render_pass, {offscreen.color[0].view, offscreen.color[1].view, offscreen.depth.view}, offscreen.extent);

		// Create sampler to sample from the color attachments
		offscreen.sampler = create_sampler();
	}

	// Bloom separable filter pass
	{
		filter_pass.extent = extent;

		// Color attachments

		// Floating point color attachment
		filter_pass.color       = create_attachment(color_format, vk::ImageUsageFlagBits::eColorAttachment);
		filter_pass.render_pass = create_filter_render_pass();
		filter_pass.framebuffer = vkb::common::create_framebuffer(get_device()->get_handle(), filter_pass.render_pass, {filter_pass.color.view}, filter_pass.extent);
		filter_pass.sampler     = create_sampler();
	}
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPHDR::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffers.matrices = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                                  sizeof(ubo_matrices),
	                                                                  vk::BufferUsageFlagBits::eUniformBuffer,
	                                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Params
	uniform_buffers.params = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                                sizeof(ubo_params),
	                                                                vk::BufferUsageFlagBits::eUniformBuffer,
	                                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
	update_params();
}

void HPPHDR::setup_bloom()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {{{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	                                                           {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::Device device           = get_device()->get_handle();
	bloom.descriptor_set_layout = device.createDescriptorSetLayout({{}, bindings});
	bloom.pipeline_layout       = device.createPipelineLayout({{}, bloom.descriptor_set_layout});
	bloom.pipelines[0]          = create_bloom_pipeline(1);
	bloom.pipelines[1]          = create_bloom_pipeline(0);
	bloom.descriptor_set        = vkb::common::allocate_descriptor_set(device, descriptor_pool, bloom.descriptor_set_layout);
	update_bloom_descriptor_set();
}

void HPPHDR::setup_composition()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {{{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	                                                           {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::Device device                 = get_device()->get_handle();
	composition.descriptor_set_layout = device.createDescriptorSetLayout({{}, bindings});
	composition.pipeline_layout       = device.createPipelineLayout({{}, composition.descriptor_set_layout});
	composition.pipeline              = create_composition_pipeline();
	composition.descriptor_set        = vkb::common::allocate_descriptor_set(device, descriptor_pool, composition.descriptor_set_layout);
	update_composition_descriptor_set();
}

void HPPHDR::setup_models()
{
	std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {{{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	                                                           {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	                                                           {2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::Device device            = get_device()->get_handle();
	models.descriptor_set_layout = device.createDescriptorSetLayout({{}, bindings});
	models.pipeline_layout       = device.createPipelineLayout({{}, models.descriptor_set_layout});

	models.objects.descriptor_set = vkb::common::allocate_descriptor_set(device, descriptor_pool, models.descriptor_set_layout);
	update_model_descriptor_set(models.objects.descriptor_set);
	models.objects.pipeline = create_models_pipeline(1, vk::CullModeFlagBits::eFront, true);

	models.skybox.descriptor_set = vkb::common::allocate_descriptor_set(device, descriptor_pool, models.descriptor_set_layout);
	update_model_descriptor_set(models.skybox.descriptor_set);
	models.skybox.pipeline = create_models_pipeline(0, vk::CullModeFlagBits::eBack, false);
}

void HPPHDR::update_composition_descriptor_set()
{
	std::array<vk::DescriptorImageInfo, 2> color_descriptors = {{{offscreen.sampler, offscreen.color[0].view, vk::ImageLayout::eShaderReadOnlyOptimal},
	                                                             {offscreen.sampler, filter_pass.color.view, vk::ImageLayout::eShaderReadOnlyOptimal}}};

	std::array<vk::WriteDescriptorSet, 2> sampler_write_descriptor_sets = {
	    {{composition.descriptor_set, 0, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[0]},
	     {composition.descriptor_set, 1, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[1]}}};

	get_device()->get_handle().updateDescriptorSets(sampler_write_descriptor_sets, {});
}

void HPPHDR::update_bloom_descriptor_set()
{
	std::array<vk::DescriptorImageInfo, 2> color_descriptors = {{{offscreen.sampler, offscreen.color[0].view, vk::ImageLayout::eShaderReadOnlyOptimal},
	                                                             {offscreen.sampler, offscreen.color[1].view, vk::ImageLayout::eShaderReadOnlyOptimal}}};

	std::array<vk::WriteDescriptorSet, 2> sampler_write_descriptor_sets = {
	    {{bloom.descriptor_set, 0, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[0]},
	     {bloom.descriptor_set, 1, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[1]}}};

	get_device()->get_handle().updateDescriptorSets(sampler_write_descriptor_sets, {});
}

void HPPHDR::update_model_descriptor_set(vk::DescriptorSet descriptor_set)
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

	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPHDR::update_params()
{
	uniform_buffers.params->convert_and_update(ubo_params);
}

void HPPHDR::update_uniform_buffers()
{
	ubo_matrices.projection       = camera.matrices.perspective;
	ubo_matrices.modelview        = camera.matrices.view * models.transforms[models.object_index];
	ubo_matrices.skybox_modelview = camera.matrices.view;
	uniform_buffers.matrices->convert_and_update(ubo_matrices);
}

std::unique_ptr<vkb::Application> create_hpp_hdr()
{
	return std::make_unique<HPPHDR>();
}
