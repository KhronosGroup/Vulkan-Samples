/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include <hpp_hdr.h>

HPPHDR::HPPHDR()
{
	title = "HPP High dynamic range rendering";
}

HPPHDR::~HPPHDR()
{
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		device.destroyPipeline(pipelines.skybox);
		device.destroyPipeline(pipelines.reflect);
		device.destroyPipeline(pipelines.composition);
		device.destroyPipeline(pipelines.bloom[0]);
		device.destroyPipeline(pipelines.bloom[1]);

		device.destroyPipelineLayout(pipeline_layouts.models);
		device.destroyPipelineLayout(pipeline_layouts.composition);
		device.destroyPipelineLayout(pipeline_layouts.bloom_filter);

		device.destroyDescriptorSetLayout(descriptor_set_layouts.models);
		device.destroyDescriptorSetLayout(descriptor_set_layouts.composition);
		device.destroyDescriptorSetLayout(descriptor_set_layouts.bloom_filter);

		device.destroyRenderPass(offscreen.render_pass);
		device.destroyRenderPass(filter_pass.render_pass);

		device.destroyFramebuffer(offscreen.framebuffer);
		device.destroyFramebuffer(filter_pass.framebuffer);

		device.destroySampler(offscreen.sampler);
		device.destroySampler(filter_pass.sampler);

		offscreen.depth.destroy(device);
		offscreen.color[0].destroy(device);
		offscreen.color[1].destroy(device);

		filter_pass.color[0].destroy(device);

		device.destroySampler(textures.envmap.sampler);
	}
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
		draw_cmd_buffers[i].begin(command_buffer_begin_info);

		{
			/*
				First pass: Render scene to offscreen framebuffer
			*/

			std::array<vk::ClearValue, 3> clear_values =
			    {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			      vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			      vk::ClearDepthStencilValue(0.0f, 0)}};

			vk::RenderPassBeginInfo render_pass_begin_info(offscreen.render_pass, offscreen.framebuffer, {{0, 0}, offscreen.extent}, clear_values);

			draw_cmd_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(offscreen.extent.width), static_cast<float>(offscreen.extent.height), 0.0f, 1.0f);
			draw_cmd_buffers[i].setViewport(0, viewport);

			vk::Rect2D scissor({0, 0}, offscreen.extent);
			draw_cmd_buffers[i].setScissor(0, scissor);

			// Skybox
			if (display_skybox)
			{
				draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skybox);
				draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.models, 0, descriptor_sets.skybox, {});

				draw_model(models.skybox, draw_cmd_buffers[i]);
			}

			// 3D object
			draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.reflect);
			draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.models, 0, descriptor_sets.object, {});

			draw_model(models.objects[models.object_index], draw_cmd_buffers[i]);

			draw_cmd_buffers[i].endRenderPass();
		}

		/*
			Second render pass: First bloom pass
		*/
		if (bloom)
		{
			vk::ClearValue clear_value(vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})));

			// Bloom filter
			vk::RenderPassBeginInfo render_pass_begin_info(filter_pass.render_pass, filter_pass.framebuffer, {{0, 0}, filter_pass.extent}, clear_value);

			draw_cmd_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(filter_pass.extent.width), static_cast<float>(filter_pass.extent.height), 0.0f, 1.0f);
			draw_cmd_buffers[i].setViewport(0, viewport);

			vk::Rect2D scissor({0, 0}, filter_pass.extent);
			draw_cmd_buffers[i].setScissor(0, scissor);

			draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.bloom_filter, 0, descriptor_sets.bloom_filter, {});

			draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.bloom[1]);
			draw_cmd_buffers[i].draw(3, 1, 0, 0);

			draw_cmd_buffers[i].endRenderPass();
		}

		/*
			Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
		*/

		/*
			Third render pass: Scene rendering with applied second bloom pass (when enabled)
		*/
		{
			std::array<vk::ClearValue, 2> clear_values =
			    {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			      vk::ClearDepthStencilValue(0.0f, 0)}};

			// Final composition
			vk::RenderPassBeginInfo render_pass_begin_info(render_pass, framebuffers[i], {{0, 0}, extent}, clear_values);

			draw_cmd_buffers[i].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
			draw_cmd_buffers[i].setViewport(0, viewport);

			vk::Rect2D scissor({0, 0}, extent);
			draw_cmd_buffers[i].setScissor(0, scissor);

			draw_cmd_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.composition, 0, descriptor_sets.composition, {});

			// Scene
			draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.composition);
			draw_cmd_buffers[i].draw(3, 1, 0, 0);

			// Bloom
			if (bloom)
			{
				draw_cmd_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.bloom[0]);
				draw_cmd_buffers[i].draw(3, 1, 0, 0);
			}

			draw_ui(draw_cmd_buffers[i]);

			draw_cmd_buffers[i].endRenderPass();
		}

		draw_cmd_buffers[i].end();
	}
}

HPPHDR::FrameBufferAttachment HPPHDR::create_attachment(vk::Format format, vk::ImageUsageFlagBits usage)
{
	vk::ImageAspectFlags aspect_mask;
	vk::ImageLayout      image_layout;

	switch (usage)
	{
		case vk::ImageUsageFlagBits::eColorAttachment:
			aspect_mask  = vk::ImageAspectFlagBits::eColor;
			image_layout = vk::ImageLayout::eColorAttachmentOptimal;
			break;
		case vk::ImageUsageFlagBits::eDepthStencilAttachment:
			aspect_mask = vk::ImageAspectFlagBits::eDepth;
			// Stencil aspect should only be set on depth + stencil formats (vk::Format::eD16UnormS8Uint...vk::Format::eD32SfloatS8Uint)
			if ((format == vk::Format::eD16UnormS8Uint) || (format == vk::Format::eD24UnormS8Uint) || (format == vk::Format::eD32SfloatS8Uint))
			{
				aspect_mask |= vk::ImageAspectFlagBits::eStencil;
			}
			image_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			break;
		default:
			assert(false);
	}

	vk::ImageCreateInfo image_create_info;
	image_create_info.imageType   = vk::ImageType::e2D;
	image_create_info.format      = format;
	image_create_info.extent      = vk::Extent3D(offscreen.extent, 1);
	image_create_info.mipLevels   = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples     = vk::SampleCountFlagBits::e1;
	image_create_info.tiling      = vk::ImageTiling::eOptimal;
	image_create_info.usage       = usage | vk::ImageUsageFlagBits::eSampled;
	vk::Image image               = get_device()->get_handle().createImage(image_create_info);

	vk::MemoryRequirements memory_requirements = get_device()->get_handle().getImageMemoryRequirements(image);
	vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size,
	                                            get_device()->get_gpu().get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
	vk::DeviceMemory       mem = get_device()->get_handle().allocateMemory(memory_allocate_info);
	get_device()->get_handle().bindImageMemory(image, mem, 0);

	vk::ImageViewCreateInfo image_view_create_info({}, image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1});
	vk::ImageView           view = get_device()->get_handle().createImageView(image_view_create_info);

	return FrameBufferAttachment(image, mem, view, format);
}

// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
void HPPHDR::prepare_offscreen_buffer()
{
	// We need to select a format that supports the color attachment blending flag, so we iterate over multiple formats to find one that supports this flag
	vk::Format color_format{vk::Format::eUndefined};

	const std::vector<vk::Format> float_format_priority_list = {
	    vk::Format::eR32G32B32A32Sfloat,
	    vk::Format::eR16G16B16A16Sfloat};

	for (auto &format : float_format_priority_list)
	{
		const vk::FormatProperties properties = get_device()->get_gpu().get_handle().getFormatProperties(format);
		if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachmentBlend)
		{
			color_format = format;
			break;
		}
	}

	if (color_format == vk::Format::eUndefined)
	{
		throw std::runtime_error("No suitable float format could be determined");
	}

	{
		offscreen.extent = extent;

		// Color attachments

		// We are using two 128-Bit RGBA floating point color buffers for this sample
		// In a performance or bandwith-limited scenario you should consider using a format with lower precision
		offscreen.color[0] = create_attachment(color_format, vk::ImageUsageFlagBits::eColorAttachment);
		offscreen.color[1] = create_attachment(color_format, vk::ImageUsageFlagBits::eColorAttachment);
		// Depth attachment
		offscreen.depth = create_attachment(depth_format, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		// Set up separate renderpass with references to the colorand depth attachments
		std::array<vk::AttachmentDescription, 3> attachment_descriptions;

		// Init attachment properties
		for (uint32_t i = 0; i < 3; ++i)
		{
			attachment_descriptions[i].samples        = vk::SampleCountFlagBits::e1;
			attachment_descriptions[i].loadOp         = vk::AttachmentLoadOp::eClear;
			attachment_descriptions[i].storeOp        = vk::AttachmentStoreOp::eStore;
			attachment_descriptions[i].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
			attachment_descriptions[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			if (i == 2)
			{
				attachment_descriptions[i].initialLayout = vk::ImageLayout::eUndefined;
				attachment_descriptions[i].finalLayout   = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			}
			else
			{
				attachment_descriptions[i].initialLayout = vk::ImageLayout::eUndefined;
				attachment_descriptions[i].finalLayout   = vk::ImageLayout::eShaderReadOnlyOptimal;
			}
		}

		// Formats
		attachment_descriptions[0].format = offscreen.color[0].format;
		attachment_descriptions[1].format = offscreen.color[1].format;
		attachment_descriptions[2].format = offscreen.depth.format;

		std::array<vk::AttachmentReference, 2> color_references{
		    {{0, vk::ImageLayout::eColorAttachmentOptimal},
		     {1, vk::ImageLayout::eColorAttachmentOptimal}}};

		vk::AttachmentReference depth_reference(2, vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, nullptr, color_references, nullptr, &depth_reference);

		// Use subpass dependencies for attachment layput transitions
		std::array<vk::SubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass      = 0;
		dependencies[0].srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
		dependencies[0].dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencies[0].srcAccessMask   = vk::AccessFlagBits::eMemoryRead;
		dependencies[0].dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		dependencies[1].srcSubpass      = 0;
		dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencies[1].dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
		dependencies[1].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
		dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		vk::RenderPassCreateInfo render_pass_create_info({}, attachment_descriptions, subpass, dependencies);

		offscreen.render_pass = get_device()->get_handle().createRenderPass(render_pass_create_info);

		std::array<vk::ImageView, 3> attachments{offscreen.color[0].view, offscreen.color[1].view, offscreen.depth.view};

		vk::FramebufferCreateInfo framebuffer_create_info({}, offscreen.render_pass, attachments, offscreen.extent.width, offscreen.extent.height, 1);
		offscreen.framebuffer = get_device()->get_handle().createFramebuffer(framebuffer_create_info);

		// Create sampler to sample from the color attachments
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
		offscreen.sampler                 = get_device()->get_handle().createSampler(sampler_create_info);
	}

	// Bloom separable filter pass
	{
		filter_pass.extent = extent;

		// Color attachments

		// Floating point color attachment
		filter_pass.color[0] = create_attachment(color_format, vk::ImageUsageFlagBits::eColorAttachment);

		// Set up separate renderpass with references to the colorand depth attachments
		vk::AttachmentDescription attachment_description;

		// Init attachment properties
		attachment_description.samples        = vk::SampleCountFlagBits::e1;
		attachment_description.loadOp         = vk::AttachmentLoadOp::eClear;
		attachment_description.storeOp        = vk::AttachmentStoreOp::eStore;
		attachment_description.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
		attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachment_description.initialLayout  = vk::ImageLayout::eUndefined;
		attachment_description.finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;
		attachment_description.format         = filter_pass.color[0].format;

		vk::AttachmentReference color_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, color_reference);

		// Use subpass dependencies for attachment layput transitions
		std::array<vk::SubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass      = 0;
		dependencies[0].srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
		dependencies[0].dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencies[0].srcAccessMask   = vk::AccessFlagBits::eMemoryRead;
		dependencies[0].dstAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		dependencies[1].srcSubpass      = 0;
		dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencies[1].dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;
		dependencies[1].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
		dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		vk::RenderPassCreateInfo render_pass_create_info({}, attachment_description, subpass, dependencies);

		filter_pass.render_pass = get_device()->get_handle().createRenderPass(render_pass_create_info);

		vk::FramebufferCreateInfo framebuffer_create_info(
		    {}, filter_pass.render_pass, filter_pass.color[0].view, filter_pass.extent.width, filter_pass.extent.height, 1);
		filter_pass.framebuffer = get_device()->get_handle().createFramebuffer(framebuffer_create_info);

		// Create sampler to sample from the color attachments
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
		filter_pass.sampler               = get_device()->get_handle().createSampler(sampler_create_info);
	}
}

void HPPHDR::load_assets()
{
	// Models
	models.skybox                      = load_model("scenes/cube.gltf");
	std::vector<std::string> filenames = {"geosphere.gltf", "teapot.gltf", "torusknot.gltf"};
	object_names                       = {"Sphere", "Teapot", "Torusknot"};
	for (auto file : filenames)
	{
		auto object = load_model("scenes/" + file);
		models.objects.emplace_back(std::move(object));
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

void HPPHDR::setup_descriptor_pool()
{
	uint32_t                              num_descriptor_sets = 4;
	std::array<vk::DescriptorPoolSize, 2> pool_sizes          = {{{vk::DescriptorType::eUniformBuffer, 4}, {vk::DescriptorType::eCombinedImageSampler, 6}}};
	vk::DescriptorPoolCreateInfo          descriptor_pool_create_info({}, num_descriptor_sets, pool_sizes);
	descriptor_pool = get_device()->get_handle().createDescriptorPool(descriptor_pool_create_info);
}

void HPPHDR::setup_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 3> set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	     {2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}};

	vk::DescriptorSetLayoutCreateInfo descriptor_layout_create_info({}, set_layout_bindings);

	descriptor_set_layouts.models = get_device()->get_handle().createDescriptorSetLayout(descriptor_layout_create_info);

#if defined(ANDROID)
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, 1, &descriptor_set_layouts.models);
#else
	vk::PipelineLayoutCreateInfo  pipeline_layout_create_info({}, descriptor_set_layouts.models);
#endif

	pipeline_layouts.models = get_device()->get_handle().createPipelineLayout(pipeline_layout_create_info);

	// Bloom filter
	std::array<vk::DescriptorSetLayoutBinding, 2> bloom_set_layout_bindings = {
	    {{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

	descriptor_layout_create_info.setBindings(bloom_set_layout_bindings);
	descriptor_set_layouts.bloom_filter = get_device()->get_handle().createDescriptorSetLayout(descriptor_layout_create_info);

	pipeline_layout_create_info.setSetLayouts(descriptor_set_layouts.bloom_filter);
	pipeline_layouts.bloom_filter = get_device()->get_handle().createPipelineLayout(pipeline_layout_create_info);

	// G-Buffer composition
	std::array<vk::DescriptorSetLayoutBinding, 2> &g_buffer_composition_set_layout_bindings = bloom_set_layout_bindings;

	descriptor_layout_create_info.setBindings(g_buffer_composition_set_layout_bindings);
	descriptor_set_layouts.composition = get_device()->get_handle().createDescriptorSetLayout(descriptor_layout_create_info);

	pipeline_layout_create_info.setSetLayouts(descriptor_set_layouts.composition);
	pipeline_layouts.composition = get_device()->get_handle().createPipelineLayout(pipeline_layout_create_info);
}

void HPPHDR::setup_descriptor_sets()
{
#if defined(ANDROID)
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, 1, &descriptor_set_layouts.models);
#else
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, descriptor_set_layouts.models);
#endif

	// 3D object descriptor set
	descriptor_sets.object = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

	vk::DescriptorBufferInfo matrix_buffer_descriptor(uniform_buffers.matrices->get_handle(), 0, VK_WHOLE_SIZE);
	vk::DescriptorImageInfo  environment_image_descriptor(
        textures.envmap.sampler,
        textures.envmap.image->get_vk_image_view().get_handle(),
        descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, textures.envmap.image->get_vk_image_view().get_format()));
	vk::DescriptorBufferInfo              params_buffer_descriptor(uniform_buffers.params->get_handle(), 0, VK_WHOLE_SIZE);
	std::array<vk::WriteDescriptorSet, 3> write_descriptor_sets = {
	    {{descriptor_sets.object, 0, 0, vk::DescriptorType::eUniformBuffer, {}, matrix_buffer_descriptor},
	     {descriptor_sets.object, 1, 0, vk::DescriptorType::eCombinedImageSampler, environment_image_descriptor},
	     {descriptor_sets.object, 2, 0, vk::DescriptorType::eUniformBuffer, {}, params_buffer_descriptor}}};
	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});

	// Sky box descriptor set
	descriptor_sets.skybox = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

	write_descriptor_sets[0].dstSet = descriptor_sets.skybox;
	write_descriptor_sets[1].dstSet = descriptor_sets.skybox;
	write_descriptor_sets[2].dstSet = descriptor_sets.skybox;
	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});

	// Bloom filter
	alloc_info.setSetLayouts(descriptor_set_layouts.bloom_filter);
	descriptor_sets.bloom_filter = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

	std::array<vk::DescriptorImageInfo, 2> color_descriptors = {{{offscreen.sampler, offscreen.color[0].view, vk::ImageLayout::eShaderReadOnlyOptimal},
	                                                             {offscreen.sampler, offscreen.color[1].view, vk::ImageLayout::eShaderReadOnlyOptimal}}};

	std::array<vk::WriteDescriptorSet, 2> sampler_write_descriptor_sets = {
	    {{descriptor_sets.bloom_filter, 0, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[0]},
	     {descriptor_sets.bloom_filter, 1, 0, vk::DescriptorType::eCombinedImageSampler, color_descriptors[1]}}};
	get_device()->get_handle().updateDescriptorSets(sampler_write_descriptor_sets, {});

	// Composition descriptor set
	alloc_info.setSetLayouts(descriptor_set_layouts.composition);
	descriptor_sets.composition = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

	color_descriptors[1].imageView = filter_pass.color[0].view;

	sampler_write_descriptor_sets[0].dstSet = descriptor_sets.composition;
	sampler_write_descriptor_sets[1].dstSet = descriptor_sets.composition;
	get_device()->get_handle().updateDescriptorSets(sampler_write_descriptor_sets, {});
}

void HPPHDR::prepare_pipelines()
{
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
	    load_shader("hdr/composition.vert", vk::ShaderStageFlagBits::eVertex),
	    load_shader("hdr/composition.frag", vk::ShaderStageFlagBits::eFragment)};

	// Empty vertex input state, full screen triangles are generated by the vertex shader
	vk::PipelineVertexInputStateCreateInfo empty_input_state;

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, vk::PrimitiveTopology::eTriangleList, false);

	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

	vk::PipelineRasterizationStateCreateInfo rasterization_state;
	rasterization_state.polygonMode = vk::PolygonMode::eFill;
	rasterization_state.cullMode    = vk::CullModeFlagBits::eFront;
	rasterization_state.frontFace   = vk::FrontFace::eCounterClockwise;
	rasterization_state.lineWidth   = 1.0f;

	vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
	depth_stencil_state.depthCompareOp = vk::CompareOp::eGreater;
	depth_stencil_state.back.compareOp = vk::CompareOp::eAlways;
	depth_stencil_state.front          = depth_stencil_state.back;

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo color_blend_state({}, false, {}, blend_attachment_state);

	std::array<vk::DynamicState, 2>    dynamic_state_enables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

	// Final fullscreen composition pass pipeline
	vk::GraphicsPipelineCreateInfo pipeline_create_info({},
	                                                    shader_stages,
	                                                    &empty_input_state,
	                                                    &input_assembly_state,
	                                                    {},
	                                                    &viewport_state,
	                                                    &rasterization_state,
	                                                    &multisample_state,
	                                                    &depth_stencil_state,
	                                                    &color_blend_state,
	                                                    &dynamic_state,
	                                                    pipeline_layouts.composition,
	                                                    render_pass,
	                                                    {},
	                                                    {},
	                                                    -1);

	vk::Result result;
	std::tie(result, pipelines.composition) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Bloom pass
	shader_stages[0]                           = load_shader("hdr/bloom.vert", vk::ShaderStageFlagBits::eVertex);
	shader_stages[1]                           = load_shader("hdr/bloom.frag", vk::ShaderStageFlagBits::eFragment);
	blend_attachment_state.blendEnable         = true;
	blend_attachment_state.colorBlendOp        = vk::BlendOp::eAdd;
	blend_attachment_state.srcColorBlendFactor = vk::BlendFactor::eOne;
	blend_attachment_state.dstColorBlendFactor = vk::BlendFactor::eOne;
	blend_attachment_state.alphaBlendOp        = vk::BlendOp::eAdd;
	blend_attachment_state.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	blend_attachment_state.dstAlphaBlendFactor = vk::BlendFactor::eDstAlpha;

	// Set constant parameters via specialization constants
	vk::SpecializationMapEntry specialization_map_entry(0, 0, sizeof(uint32_t));

	uint32_t               dir = 1;
	vk::SpecializationInfo specialization_info(1, &specialization_map_entry, sizeof(uint32_t), &dir);
	shader_stages[1].pSpecializationInfo = &specialization_info;

	std::tie(result, pipelines.bloom[0]) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Second blur pass (into separate framebuffer)
	pipeline_create_info.renderPass      = filter_pass.render_pass;
	dir                                  = 0;
	std::tie(result, pipelines.bloom[1]) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Object rendering pipelines
	rasterization_state.cullMode = vk::CullModeFlagBits::eBack;

	// Vertex bindings an attributes for model rendering
	// Binding description
	vk::VertexInputBindingDescription vertex_input_binding(0, sizeof(HPPVertex), vk::VertexInputRate::eVertex);

	// Attribute descriptions
	std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes = {{{0, 0, vk::Format::eR32G32B32Sfloat, 0},
	                                                                               {1, 0, vk::Format::eR32G32B32Sfloat, 3 * sizeof(float)}}};

	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	// Skybox pipeline (background cube)
	blend_attachment_state.blendEnable = false;

	std::array<vk::PipelineColorBlendAttachmentState, 2> blend_attachment_states;
	blend_attachment_states[0].colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	blend_attachment_states[1].colorWriteMask = blend_attachment_states[0].colorWriteMask;

	color_blend_state.setAttachments(blend_attachment_states);
	pipeline_create_info.layout     = pipeline_layouts.models;
	pipeline_create_info.renderPass = offscreen.render_pass;

	shader_stages[0] = load_shader("hdr/gbuffer.vert", vk::ShaderStageFlagBits::eVertex);
	shader_stages[1] = load_shader("hdr/gbuffer.frag", vk::ShaderStageFlagBits::eFragment);

	// Set constant parameters via specialization constants
	uint32_t shadertype                  = 0;
	specialization_info                  = vk::SpecializationInfo(1, &specialization_map_entry, sizeof(uint32_t), &shadertype);
	shader_stages[0].pSpecializationInfo = &specialization_info;
	shader_stages[1].pSpecializationInfo = &specialization_info;

	std::tie(result, pipelines.skybox) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Object rendering pipeline
	shadertype = 1;

	// Enable depth test and write
	depth_stencil_state.depthWriteEnable = true;
	depth_stencil_state.depthTestEnable  = true;
	// Flip cull mode
	rasterization_state.cullMode        = vk::CullModeFlagBits::eFront;
	std::tie(result, pipelines.reflect) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
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

void HPPHDR::update_uniform_buffers()
{
	ubo_matrices.projection       = camera.matrices.perspective;
	ubo_matrices.modelview        = camera.matrices.view * models.transforms[models.object_index];
	ubo_matrices.skybox_modelview = camera.matrices.view;
	uniform_buffers.matrices->convert_and_update(ubo_matrices);
}

void HPPHDR::update_params()
{
	uniform_buffers.params->convert_and_update(ubo_params);
}

void HPPHDR::draw()
{
	HPPApiVulkanSample::prepare_frame();

	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

bool HPPHDR::prepare(vkb::platform::HPPPlatform &platform)
{
	if (!HPPApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -4.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));

	// Note: Using Revsered depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, (float) extent.width / (float) extent.height, 256.0f, 0.1f);

	load_assets();
	prepare_uniform_buffers();
	prepare_offscreen_buffer();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();
	prepared = true;
	return true;
}

void HPPHDR::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

void HPPHDR::on_update_ui_overlay(vkb::HPPDrawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("Object type", &models.object_index, object_names))
		{
			update_uniform_buffers();
			build_command_buffers();
		}
		if (drawer.input_float("Exposure", &ubo_params.exposure, 0.025f, 3))
		{
			update_params();
		}
		if (drawer.checkbox("Bloom", &bloom))
		{
			build_command_buffers();
		}
		if (drawer.checkbox("Skybox", &display_skybox))
		{
			build_command_buffers();
		}
	}
}

bool HPPHDR::resize(const uint32_t width, const uint32_t height)
{
	HPPApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

std::unique_ptr<vkb::Application> create_hpp_hdr()
{
	return std::make_unique<HPPHDR>();
}
