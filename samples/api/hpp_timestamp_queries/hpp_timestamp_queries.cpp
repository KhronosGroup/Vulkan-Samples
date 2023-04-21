/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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
	if (get_device() && get_device()->get_handle())
	{
		vk::Device device = get_device()->get_handle();

		device.destroyQueryPool(time_stamps_query_pool);

		device.destroyPipeline(pipelines.bloom[0]);
		device.destroyPipeline(pipelines.bloom[1]);
		device.destroyPipeline(pipelines.composition);
		device.destroyPipeline(pipelines.reflect);
		device.destroyPipeline(pipelines.skybox);

		device.destroyPipelineLayout(pipeline_layouts.bloom_filter);
		device.destroyPipelineLayout(pipeline_layouts.composition);
		device.destroyPipelineLayout(pipeline_layouts.models);

		device.destroyDescriptorSetLayout(descriptor_set_layouts.bloom_filter);
		device.destroyDescriptorSetLayout(descriptor_set_layouts.composition);
		device.destroyDescriptorSetLayout(descriptor_set_layouts.models);

		device.destroyRenderPass(filter_pass.render_pass);
		device.destroyRenderPass(offscreen.render_pass);

		device.destroyFramebuffer(filter_pass.framebuffer);
		device.destroyFramebuffer(offscreen.framebuffer);

		device.destroySampler(filter_pass.sampler);
		device.destroySampler(offscreen.sampler);

		offscreen.depth.destroy(device);
		offscreen.color[0].destroy(device);
		offscreen.color[1].destroy(device);

		filter_pass.color.destroy(device);

		device.destroySampler(textures.envmap.sampler);
	}
}

bool HPPTimestampQueries::prepare(vkb::platform::HPPPlatform &platform)
{
	if (!HPPApiVulkanSample::prepare(platform))
	{
		return false;
	}

	// Check if the selected device supports timestamps. A value of zero means no support.
	vk::PhysicalDeviceLimits const &device_limits = device->get_gpu().get_properties().limits;
	if (device_limits.timestampPeriod == 0)
	{
		throw std::runtime_error{"The selected device does not support timestamp queries!"};
	}

	// Check if all queues support timestamp queries, if not we need to check on a per-queue basis
	if (!device_limits.timestampComputeAndGraphics)
	{
		// Check if the graphics queue used in this sample supports time stamps
		vk::QueueFamilyProperties const &graphics_queue_family_properties = device->get_suitable_graphics_queue().get_properties();
		if (graphics_queue_family_properties.timestampValidBits == 0)
		{
			throw std::runtime_error{"The selected graphics queue family does not support timestamp queries!"};
		}
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -4.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, (float) extent.width / (float) extent.height, 256.0f, 0.1f);

	load_assets();
	prepare_uniform_buffers();
	prepare_offscreen_buffer();
	prepare_descriptor_set_layout();
	prepare_pipelines();
	prepare_descriptor_pool();
	prepare_descriptor_sets();
	prepare_time_stamp_queries();
	build_command_buffers();
	prepared = true;
	return true;
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

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		vk::CommandBuffer const &command_buffer = draw_cmd_buffers[i];

		command_buffer.begin(command_buffer_begin_info);

		// Reset the timestamp query pool, so we can start fetching new values into it
		command_buffer.resetQueryPool(time_stamps_query_pool, 0, static_cast<uint32_t>(time_stamps.size()));

		{
			/*
			    First pass: Render scene to offscreen framebuffer
			*/

			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, time_stamps_query_pool, 0);

			std::array<vk::ClearValue, 3> clear_values = {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			                                               vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			                                               vk::ClearDepthStencilValue(0.0f, 0)}};

			vk::RenderPassBeginInfo render_pass_begin_info(offscreen.render_pass, offscreen.framebuffer, {{0, 0}, offscreen.extent}, clear_values);
			command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(offscreen.extent.width), static_cast<float>(offscreen.extent.height), 0.0f, 1.0f);
			command_buffer.setViewport(0, viewport);

			vk::Rect2D scissor({0, 0}, offscreen.extent);
			command_buffer.setScissor(0, scissor);

			// Skybox
			if (display_skybox)
			{
				command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skybox);
				command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.models, 0, descriptor_sets.skybox, {});

				draw_model(models.skybox, command_buffer);
			}

			// 3D object
			command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.reflect);
			command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.models, 0, descriptor_sets.object, {});

			draw_model(models.objects[models.object_index], command_buffer);

			command_buffer.endRenderPass();

			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, time_stamps_query_pool, 1);
		}

		/*
		    Second render pass: First bloom pass
		*/
		if (bloom)
		{
			std::array<vk::ClearValue, 2> clear_values = {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			                                               vk::ClearDepthStencilValue(0.0f, 0)}};

			// Bloom filter
			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, time_stamps_query_pool, 2);

			vk::RenderPassBeginInfo render_pass_begin_info(filter_pass.render_pass, filter_pass.framebuffer, {{0, 0}, filter_pass.extent}, clear_values);
			command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(filter_pass.extent.width), static_cast<float>(filter_pass.extent.height), 0.0f, 1.0f);
			command_buffer.setViewport(0, viewport);

			vk::Rect2D scissor({0, 0}, filter_pass.extent);
			command_buffer.setScissor(0, scissor);

			command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.bloom_filter, 0, descriptor_sets.bloom_filter, {});

			command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.bloom[1]);
			command_buffer.draw(3, 1, 0, 0);

			command_buffer.endRenderPass();

			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, time_stamps_query_pool, 3);
		}

		/*
		    Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
		*/

		/*
		    Third render pass: Scene rendering with applied second bloom pass (when enabled)
		*/
		{
			std::array<vk::ClearValue, 2> clear_values = {{vk::ClearColorValue(std::array<float, 4>({{0.0f, 0.0f, 0.0f, 0.0f}})),
			                                               vk::ClearDepthStencilValue(0.0f, 0)}};

			// Final composition
			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, time_stamps_query_pool, bloom ? 4 : 2);

			vk::RenderPassBeginInfo render_pass_begin_info(render_pass, framebuffers[i], {{0, 0}, extent}, clear_values);
			command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
			command_buffer.setViewport(0, viewport);

			vk::Rect2D scissor({0, 0}, extent);
			command_buffer.setScissor(0, scissor);

			command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.composition, 0, descriptor_sets.composition, {});

			// Scene
			command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.composition);
			command_buffer.draw(3, 1, 0, 0);

			// Bloom
			if (bloom)
			{
				command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.bloom[0]);
				command_buffer.draw(3, 1, 0, 0);
			}

			draw_ui(command_buffer);

			command_buffer.endRenderPass();

			command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, time_stamps_query_pool, bloom ? 5 : 3);
		}

		command_buffer.end();
	}
}

void HPPTimestampQueries::on_update_ui_overlay(vkb::HPPDrawer &drawer)
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
	if (drawer.header("timing"))
	{
		// Timestamps don't have a time unit themselves, but are read as timesteps
		// The timestampPeriod property of the device tells how many nanoseconds such a timestep translates to on the selected device
		float timestampFrequency = device->get_gpu().get_properties().limits.timestampPeriod;

		drawer.text("Pass 1: Offscreen scene rendering: %.3f ms", float(time_stamps[1] - time_stamps[0]) * timestampFrequency / 1000000.0f);
		drawer.text("Pass 2: %s %.3f ms", (bloom ? "First bloom pass" : "Scene display"), float(time_stamps[3] - time_stamps[2]) * timestampFrequency / 1000000.0f);
		if (bloom)
		{
			drawer.text("Pass 3: Second bloom pass %.3f ms", float(time_stamps[5] - time_stamps[4]) * timestampFrequency / 1000000.0f);
		}
	}
}

void HPPTimestampQueries::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

void HPPTimestampQueries::create_attachment(vk::Format format, vk::ImageUsageFlagBits usage, FramebufferAttachment *attachment)
{
	attachment->format = format;

	vk::ImageAspectFlags aspect_mask;
	switch (usage)
	{
		case vk::ImageUsageFlagBits::eColorAttachment:
			aspect_mask = vk::ImageAspectFlagBits::eColor;
			break;
		case vk::ImageUsageFlagBits::eDepthStencilAttachment:
			aspect_mask = vk::ImageAspectFlagBits::eDepth;
			// Stencil aspect should only be set on depth + stencil formats
			if (vkb::common::is_depth_stencil_format(format) && !vkb::common::is_depth_only_format(format))
			{
				aspect_mask |= vk::ImageAspectFlagBits::eStencil;
			}
			break;
		default:
			assert(false);
	}

	vk::ImageCreateInfo image_create_info({},
	                                      vk::ImageType::e2D,
	                                      format,
	                                      vk::Extent3D(offscreen.extent, 1),
	                                      1,
	                                      1,
	                                      vk::SampleCountFlagBits::e1,
	                                      vk::ImageTiling::eOptimal,
	                                      usage | vk::ImageUsageFlagBits::eSampled,
	                                      vk::SharingMode::eExclusive,
	                                      {},
	                                      vk::ImageLayout::eUndefined);
	attachment->image = get_device()->get_handle().createImage(image_create_info);

	vk::MemoryRequirements memory_requirements = get_device()->get_handle().getImageMemoryRequirements(attachment->image);
	vk::MemoryAllocateInfo memory_allocate_info(
	    memory_requirements.size, get_device()->get_gpu().get_memory_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
	attachment->mem = get_device()->get_handle().allocateMemory(memory_allocate_info);
	get_device()->get_handle().bindImageMemory(attachment->image, attachment->mem, 0);

	vk::ImageViewCreateInfo image_view_create_info({},
	                                               attachment->image,
	                                               vk::ImageViewType::e2D,
	                                               format,
	                                               {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
	                                               {aspect_mask, 0, 1, 0, 1});
	attachment->view = get_device()->get_handle().createImageView(image_view_create_info);
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
	uint32_t count = static_cast<uint32_t>(bloom ? time_stamps.size() : time_stamps.size() - 2);

	// Fetch the time stamp results written in the command buffer submissions
	// A note on the flags used:
	//	vk::QueryResultFlagBits::e64: Results will have 64 bits. As time stamp values are on nano-seconds, this flag should always be used to avoid 32 bit overflows
	//  vk::QueryResultFlagBits::eWait: Since we want to immediately display the results, we use this flag to have the CPU wait until the results are available
	vk::Result result = device->get_handle().getQueryPoolResults(time_stamps_query_pool,
	                                                             0,
	                                                             count,
	                                                             time_stamps.size() * sizeof(uint64_t),
	                                                             time_stamps.data(),
	                                                             sizeof(uint64_t),
	                                                             vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);
	assert(result == vk::Result::eSuccess);
}

void HPPTimestampQueries::load_assets()
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
	auto teapot_matrix    = glm::mat4(1.0f);
	teapot_matrix         = glm::scale(teapot_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	teapot_matrix         = glm::rotate(teapot_matrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	auto torus_matrix     = glm::mat4(1.0f);
	models.transforms.push_back(geosphere_matrix);
	models.transforms.push_back(teapot_matrix);
	models.transforms.push_back(torus_matrix);

	// Load HDR cube map
	textures.envmap = load_texture_cubemap("textures/uffizi_rgba16f_cube.ktx", vkb::sg::Image::Color);
}

void HPPTimestampQueries::prepare_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 2> pool_sizes = {{{vk::DescriptorType::eUniformBuffer, 4}, {vk::DescriptorType::eCombinedImageSampler, 6}}};
	vk::DescriptorPoolCreateInfo          descriptor_pool_create_info({}, 4, pool_sizes);
	descriptor_pool = get_device()->get_handle().createDescriptorPool(descriptor_pool_create_info);
}

void HPPTimestampQueries::prepare_descriptor_set_layout()
{
	{
		std::array<vk::DescriptorSetLayoutBinding, 3> models_descriptor_set_layout_bindings = {
		    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
		     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		     {2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}}};

		vk::DescriptorSetLayoutCreateInfo models_descriptor_set_layout_create_info({}, models_descriptor_set_layout_bindings);

		descriptor_set_layouts.models = get_device()->get_handle().createDescriptorSetLayout(models_descriptor_set_layout_create_info);

#if defined(ANDROID)
		vk::PipelineLayoutCreateInfo models_pipeline_layout_create_info({}, 1, &descriptor_set_layouts.models);
#else
		vk::PipelineLayoutCreateInfo models_pipeline_layout_create_info({}, descriptor_set_layouts.models);
#endif

		pipeline_layouts.models = get_device()->get_handle().createPipelineLayout(models_pipeline_layout_create_info);
	}

	// Bloom filter
	{
		std::array<vk::DescriptorSetLayoutBinding, 2> bloom_descriptor_set_layout_bindings = {
		    {{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

		vk::DescriptorSetLayoutCreateInfo bloom_descriptor_set_layout_create_info({}, bloom_descriptor_set_layout_bindings);
		descriptor_set_layouts.bloom_filter = get_device()->get_handle().createDescriptorSetLayout(bloom_descriptor_set_layout_create_info);

#if defined(ANDROID)
		vk::PipelineLayoutCreateInfo bloom_pipeline_layout_create_info({}, 1, &descriptor_set_layouts.bloom_filter);
#else
		vk::PipelineLayoutCreateInfo bloom_pipeline_layout_create_info({}, descriptor_set_layouts.bloom_filter);
#endif
		pipeline_layouts.bloom_filter = get_device()->get_handle().createPipelineLayout(bloom_pipeline_layout_create_info);
	}

	// G-Buffer composition
	{
		std::array<vk::DescriptorSetLayoutBinding, 2> composition_descriptor_set_layout_bindings = {
		    {{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
		     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};

		vk::DescriptorSetLayoutCreateInfo composition_descriptor_set_layout_create_info({}, composition_descriptor_set_layout_bindings);

		descriptor_set_layouts.composition = get_device()->get_handle().createDescriptorSetLayout(composition_descriptor_set_layout_create_info);

#if defined(ANDROID)
		vk::PipelineLayoutCreateInfo composition_pipeline_layout_create_info({}, 1, &descriptor_set_layouts.composition);
#else
		vk::PipelineLayoutCreateInfo composition_pipeline_layout_create_info({}, descriptor_set_layouts.composition);
#endif
		pipeline_layouts.composition = get_device()->get_handle().createPipelineLayout(composition_pipeline_layout_create_info);
	}
}

void HPPTimestampQueries::prepare_descriptor_sets()
{
#if defined(ANDROID)
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, 1, &descriptor_set_layouts.models);
#else
	vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool, descriptor_set_layouts.models);
#endif
	vk::DescriptorBufferInfo matrix_buffer_descriptor(uniform_buffers.matrices->get_handle(), 0, VK_WHOLE_SIZE);
	vk::DescriptorImageInfo  environment_image_descriptor(textures.envmap.sampler, textures.envmap.image->get_vk_image_view().get_handle(), vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::DescriptorBufferInfo params_buffer_descriptor(uniform_buffers.params->get_handle(), 0, VK_WHOLE_SIZE);

	// 3D object descriptor set
	{
		descriptor_sets.object = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

		std::array<vk::WriteDescriptorSet, 3> write_descriptor_sets = {
		    {{descriptor_sets.object, 0, {}, vk::DescriptorType::eUniformBuffer, {}, matrix_buffer_descriptor},
		     {descriptor_sets.object, 1, {}, vk::DescriptorType::eCombinedImageSampler, environment_image_descriptor},
		     {descriptor_sets.object, 2, {}, vk::DescriptorType::eUniformBuffer, {}, params_buffer_descriptor}}};

		get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
	}

	// Sky box descriptor set
	{
		descriptor_sets.skybox = get_device()->get_handle().allocateDescriptorSets(alloc_info).front();

		std::array<vk::WriteDescriptorSet, 3> write_descriptor_sets = {
		    {{descriptor_sets.skybox, 0, {}, vk::DescriptorType::eUniformBuffer, {}, matrix_buffer_descriptor},
		     {descriptor_sets.skybox, 1, {}, vk::DescriptorType::eCombinedImageSampler, environment_image_descriptor},
		     {descriptor_sets.skybox, 2, {}, vk::DescriptorType::eUniformBuffer, {}, params_buffer_descriptor}}};

		get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
	}

	// Bloom filter
	{
#if defined(ANDROID)
		vk::DescriptorSetAllocateInfo bloom_alloc_info(descriptor_pool, 1, &descriptor_set_layouts.bloom_filter);
#else
		vk::DescriptorSetAllocateInfo bloom_alloc_info(descriptor_pool, descriptor_set_layouts.bloom_filter);
#endif

		descriptor_sets.bloom_filter = get_device()->get_handle().allocateDescriptorSets(bloom_alloc_info).front();

		std::array<vk::DescriptorImageInfo, 2> color_descriptors = {{{offscreen.sampler, offscreen.color[0].view, vk::ImageLayout::eShaderReadOnlyOptimal},
		                                                             {offscreen.sampler, offscreen.color[1].view, vk::ImageLayout::eShaderReadOnlyOptimal}}};

		std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {
		    {{descriptor_sets.bloom_filter, 0, {}, vk::DescriptorType::eCombinedImageSampler, color_descriptors[0]},
		     {descriptor_sets.bloom_filter, 1, {}, vk::DescriptorType::eCombinedImageSampler, color_descriptors[1]}}};

		get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
	}

	// Composition descriptor set
	{
#if defined(ANDROID)
		vk::DescriptorSetAllocateInfo composition_alloc_info(descriptor_pool, 1, &descriptor_set_layouts.composition);
#else
		vk::DescriptorSetAllocateInfo composition_alloc_info(descriptor_pool, descriptor_set_layouts.composition);
#endif

		descriptor_sets.composition = get_device()->get_handle().allocateDescriptorSets(composition_alloc_info).front();

		std::array<vk::DescriptorImageInfo, 2> color_descriptors = {{{offscreen.sampler, offscreen.color[0].view, vk::ImageLayout::eShaderReadOnlyOptimal},
		                                                             {offscreen.sampler, filter_pass.color.view, vk::ImageLayout::eShaderReadOnlyOptimal}}};

		std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {
		    {{descriptor_sets.composition, 0, {}, vk::DescriptorType::eCombinedImageSampler, color_descriptors[0]},
		     {descriptor_sets.composition, 1, {}, vk::DescriptorType::eCombinedImageSampler, color_descriptors[1]}}};

		get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
	}
}

// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
void HPPTimestampQueries::prepare_offscreen_buffer()
{
	{
		offscreen.extent = extent;

		// Color attachments

		// We are using two 128-Bit RGBA floating point color buffers for this sample
		// In a performance or bandwidth-limited scenario you should consider using a format with lower precision
		create_attachment(vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eColorAttachment, &offscreen.color[0]);
		create_attachment(vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eColorAttachment, &offscreen.color[1]);
		// Depth attachment
		create_attachment(depth_format, vk::ImageUsageFlagBits::eDepthStencilAttachment, &offscreen.depth);

		// Init attachment properties
		std::array<vk::AttachmentDescription, 3> attachment_descriptions = {{{{},
		                                                                      offscreen.color[0].format,
		                                                                      vk::SampleCountFlagBits::e1,
		                                                                      vk::AttachmentLoadOp::eClear,
		                                                                      vk::AttachmentStoreOp::eStore,
		                                                                      vk::AttachmentLoadOp::eDontCare,
		                                                                      vk::AttachmentStoreOp::eDontCare,
		                                                                      vk::ImageLayout::eUndefined,
		                                                                      vk::ImageLayout::eShaderReadOnlyOptimal},
		                                                                     {{},
		                                                                      offscreen.color[1].format,
		                                                                      vk::SampleCountFlagBits::e1,
		                                                                      vk::AttachmentLoadOp::eClear,
		                                                                      vk::AttachmentStoreOp::eStore,
		                                                                      vk::AttachmentLoadOp::eDontCare,
		                                                                      vk::AttachmentStoreOp::eDontCare,
		                                                                      vk::ImageLayout::eUndefined,
		                                                                      vk::ImageLayout::eShaderReadOnlyOptimal},
		                                                                     {{},
		                                                                      offscreen.depth.format,
		                                                                      vk::SampleCountFlagBits::e1,
		                                                                      vk::AttachmentLoadOp::eClear,
		                                                                      vk::AttachmentStoreOp::eStore,
		                                                                      vk::AttachmentLoadOp::eDontCare,
		                                                                      vk::AttachmentStoreOp::eDontCare,
		                                                                      vk::ImageLayout::eUndefined,
		                                                                      vk::ImageLayout::eDepthStencilAttachmentOptimal}}};

		// Set up separate renderpass with references to the color and depth attachments
		std::array<vk::AttachmentReference, 2> color_references = {{{0, vk::ImageLayout::eColorAttachmentOptimal},
		                                                            {1, vk::ImageLayout::eColorAttachmentOptimal}}};

		vk::AttachmentReference depth_reference(2, vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, color_references, {}, &depth_reference);

		// Use subpass dependencies for attachment layout transitions
		std::array<vk::SubpassDependency, 2> dependencies = {{{VK_SUBPASS_EXTERNAL,
		                                                       0,
		                                                       vk::PipelineStageFlagBits::eBottomOfPipe,
		                                                       vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                                       vk::AccessFlagBits::eMemoryRead,
		                                                       vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
		                                                       vk::DependencyFlagBits::eByRegion},
		                                                      {0,
		                                                       VK_SUBPASS_EXTERNAL,
		                                                       vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                                       vk::PipelineStageFlagBits::eBottomOfPipe,
		                                                       vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
		                                                       vk::AccessFlagBits::eMemoryRead,
		                                                       vk::DependencyFlagBits::eByRegion}}};

		vk::RenderPassCreateInfo render_pass_create_info({}, attachment_descriptions, subpass, dependencies);

		offscreen.render_pass = get_device()->get_handle().createRenderPass(render_pass_create_info);

		std::array<vk::ImageView, 3> attachments = {{offscreen.color[0].view, offscreen.color[1].view, offscreen.depth.view}};

		vk::FramebufferCreateInfo framebuffer_create_info({}, offscreen.render_pass, attachments, offscreen.extent.width, offscreen.extent.height, 1);

		offscreen.framebuffer = get_device()->get_handle().createFramebuffer(framebuffer_create_info);

		// Create sampler to sample from the color attachments
		vk::SamplerCreateInfo sampler_create_info({},
		                                          vk::Filter::eNearest,
		                                          vk::Filter::eNearest,
		                                          vk::SamplerMipmapMode::eLinear,
		                                          vk::SamplerAddressMode::eClampToEdge,
		                                          vk::SamplerAddressMode::eClampToEdge,
		                                          vk::SamplerAddressMode::eClampToEdge,
		                                          0.0f,
		                                          {},
		                                          1.0f,
		                                          {},
		                                          {},
		                                          0.0f,
		                                          1.0f,
		                                          vk::BorderColor::eFloatOpaqueWhite);

		offscreen.sampler = get_device()->get_handle().createSampler(sampler_create_info);
	}

	// Bloom separable filter pass
	{
		filter_pass.extent = extent;

		// Color attachments

		// One floating point color buffer
		create_attachment(vk::Format::eR32G32B32A32Sfloat, vk::ImageUsageFlagBits::eColorAttachment, &filter_pass.color);

		// Set up separate renderpass with references to the color attachment
		// Init attachment properties
		vk::AttachmentDescription attachment_description({},
		                                                 filter_pass.color.format,
		                                                 vk::SampleCountFlagBits::e1,
		                                                 vk::AttachmentLoadOp::eClear,
		                                                 vk::AttachmentStoreOp::eStore,
		                                                 vk::AttachmentLoadOp::eDontCare,
		                                                 vk::AttachmentStoreOp::eDontCare,
		                                                 vk::ImageLayout::eUndefined,
		                                                 vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::AttachmentReference color_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, color_reference);

		// Use subpass dependencies for attachment layout transitions
		std::array<vk::SubpassDependency, 2> dependencies = {{{VK_SUBPASS_EXTERNAL,
		                                                       0,
		                                                       vk::PipelineStageFlagBits::eBottomOfPipe,
		                                                       vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                                       vk::AccessFlagBits::eMemoryRead,
		                                                       vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
		                                                       vk::DependencyFlagBits::eByRegion},
		                                                      {0,
		                                                       VK_SUBPASS_EXTERNAL,
		                                                       vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                                       vk::PipelineStageFlagBits::eBottomOfPipe,
		                                                       vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
		                                                       vk::AccessFlagBits::eMemoryRead,
		                                                       vk::DependencyFlagBits::eByRegion}}};

		vk::RenderPassCreateInfo render_pass_create_info({}, attachment_description, subpass, dependencies);

		filter_pass.render_pass = get_device()->get_handle().createRenderPass(render_pass_create_info);

		vk::ImageView attachment = filter_pass.color.view;

		vk::FramebufferCreateInfo framebuffer_create_info({}, filter_pass.render_pass, attachment, filter_pass.extent.width, filter_pass.extent.height, 1);

		filter_pass.framebuffer = get_device()->get_handle().createFramebuffer(framebuffer_create_info);

		// Create sampler to sample from the color attachments
		vk::SamplerCreateInfo sampler_create_info({},
		                                          vk::Filter::eNearest,
		                                          vk::Filter::eNearest,
		                                          vk::SamplerMipmapMode::eLinear,
		                                          vk::SamplerAddressMode::eClampToEdge,
		                                          vk::SamplerAddressMode::eClampToEdge,
		                                          vk::SamplerAddressMode::eClampToEdge,
		                                          0.0f,
		                                          {},
		                                          1.0f,
		                                          {},
		                                          {},
		                                          0.0f,
		                                          1.0f,
		                                          vk::BorderColor::eFloatOpaqueWhite);

		filter_pass.sampler = get_device()->get_handle().createSampler(sampler_create_info);
	}
}

void HPPTimestampQueries::prepare_pipelines()
{
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages;

	// Empty vertex input state, full screen triangles are generated by the vertex shader
	vk::PipelineVertexInputStateCreateInfo empty_input_state;

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, vk::PrimitiveTopology::eTriangleList);

	vk::PipelineViewportStateCreateInfo viewport_state({}, 1, {}, 1, {});

	vk::PipelineRasterizationStateCreateInfo rasterization_state(
	    {}, false, {}, vk::PolygonMode::eFill, {}, vk::FrontFace::eCounterClockwise, {}, {}, {}, {}, 1.0f);

	vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state({}, false, false, vk::CompareOp::eGreater, {}, {}, {}, {{}, {}, {}, vk::CompareOp::eAlways});

	vk::PipelineColorBlendStateCreateInfo color_blend_state;

	std::array<vk::DynamicState, 2> dynamic_state_enables = {{vk::DynamicState::eViewport, vk::DynamicState::eScissor}};

	vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

	vk::GraphicsPipelineCreateInfo pipeline_create_info({},
	                                                    shader_stages,
	                                                    &empty_input_state,
	                                                    &input_assembly_state,
	                                                    nullptr,
	                                                    &viewport_state,
	                                                    &rasterization_state,
	                                                    &multisample_state,
	                                                    &depth_stencil_state,
	                                                    &color_blend_state,
	                                                    &dynamic_state,
	                                                    {},
	                                                    {},
	                                                    {},
	                                                    nullptr,
	                                                    -1);

	std::array<vk::PipelineColorBlendAttachmentState, 2> blend_attachment_states = {{{false,
	                                                                                  {},
	                                                                                  {},
	                                                                                  {},
	                                                                                  {},
	                                                                                  {},
	                                                                                  {},
	                                                                                  vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA},
	                                                                                 {false,
	                                                                                  {},
	                                                                                  {},
	                                                                                  {},
	                                                                                  {},
	                                                                                  {},
	                                                                                  {},
	                                                                                  vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA}}};

	// Full screen pipelines

	// Final fullscreen composition pass pipeline
	shader_stages[0]                  = load_shader("hdr/composition.vert", vk::ShaderStageFlagBits::eVertex);
	shader_stages[1]                  = load_shader("hdr/composition.frag", vk::ShaderStageFlagBits::eFragment);
	pipeline_create_info.layout       = pipeline_layouts.composition;
	pipeline_create_info.renderPass   = render_pass;
	rasterization_state.cullMode      = vk::CullModeFlagBits::eFront;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments    = blend_attachment_states.data();

	vk::Result result;
	std::tie(result, pipelines.composition) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Bloom pass
	shader_stages[0] = static_cast<VkPipelineShaderStageCreateInfo>(load_shader("hdr/bloom.vert", vk::ShaderStageFlagBits::eVertex));
	shader_stages[1] = static_cast<VkPipelineShaderStageCreateInfo>(load_shader("hdr/bloom.frag", vk::ShaderStageFlagBits::eFragment));

	vk::PipelineColorBlendAttachmentState blend_attachment_state(true,
	                                                             vk::BlendFactor::eOne,
	                                                             vk::BlendFactor::eOne,
	                                                             vk::BlendOp::eAdd,
	                                                             vk::BlendFactor::eSrcAlpha,
	                                                             vk::BlendFactor::eDstAlpha,
	                                                             vk::BlendOp::eAdd,
	                                                             vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                                                                 vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
	color_blend_state.pAttachments = &blend_attachment_state;

	// Set constant parameters via specialization constants
	vk::SpecializationMapEntry specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t                   dir = 1;
	vk::SpecializationInfo     specialization_info(1, &specialization_map_entry, sizeof(dir), &dir);
	shader_stages[1].pSpecializationInfo = &specialization_info;

	std::tie(result, pipelines.bloom[0]) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Second blur pass (into separate framebuffer)
	pipeline_create_info.renderPass = filter_pass.render_pass;
	dir                             = 0;

	std::tie(result, pipelines.bloom[1]) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);

	// Object rendering pipelines
	rasterization_state.cullMode = vk::CullModeFlagBits::eBack;

	// Vertex bindings an attributes for model rendering
	// Binding description
	vk::VertexInputBindingDescription vertex_input_bindings(0, sizeof(HPPVertex), vk::VertexInputRate::eVertex);

	// Attribute descriptions
	std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes = {{{0, 0, vk::Format::eR32G32B32Sfloat, 0},                          // Position
	                                                                               {1, 0, vk::Format::eR32G32B32Sfloat, 3 * sizeof(float)}}};        // Normal

	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_bindings, vertex_input_attributes);

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	// Skybox pipeline (background cube)
	blend_attachment_state.blendEnable = false;
	pipeline_create_info.layout        = pipeline_layouts.models;
	pipeline_create_info.renderPass    = offscreen.render_pass;
	color_blend_state.attachmentCount  = 2;
	color_blend_state.pAttachments     = blend_attachment_states.data();

	shader_stages[0] = static_cast<VkPipelineShaderStageCreateInfo>(load_shader("hdr/gbuffer.vert", vk::ShaderStageFlagBits::eVertex));
	shader_stages[1] = static_cast<VkPipelineShaderStageCreateInfo>(load_shader("hdr/gbuffer.frag", vk::ShaderStageFlagBits::eFragment));

	// Set constant parameters via specialization constants
	uint32_t shadertype                  = 0;
	specialization_info.pData            = &shadertype;
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
	rasterization_state.cullMode = vk::CullModeFlagBits::eFront;

	std::tie(result, pipelines.reflect) = get_device()->get_handle().createGraphicsPipeline(pipeline_cache, pipeline_create_info);
	assert(result == vk::Result::eSuccess);
}

// Prepare and initialize uniform buffer containing shader uniforms
void HPPTimestampQueries::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffers.matrices = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                                  sizeof(ubo_vs),
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

void HPPTimestampQueries::prepare_time_stamp_queries()
{
	// Create the query pool object used to get the GPU time tamps
	vk::QueryPoolCreateInfo query_pool_create_info({}, vk::QueryType::eTimestamp, static_cast<uint32_t>(time_stamps.size()));
	time_stamps_query_pool = get_device()->get_handle().createQueryPool(query_pool_create_info);
}

void HPPTimestampQueries::update_params()
{
	uniform_buffers.params->convert_and_update(ubo_params);
}

void HPPTimestampQueries::update_uniform_buffers()
{
	ubo_vs.projection       = camera.matrices.perspective;
	ubo_vs.modelview        = camera.matrices.view * models.transforms[models.object_index];
	ubo_vs.skybox_modelview = camera.matrices.view;
	uniform_buffers.matrices->convert_and_update(ubo_vs);
}

std::unique_ptr<vkb::Application> create_hpp_timestamp_queries()
{
	return std::make_unique<HPPTimestampQueries>();
}
