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
 * Using HLSL shaders in Vulkan with the glslang library, using vulkan.hpp
 */

#include "hpp_hlsl_shaders.h"

VKBP_DISABLE_WARNINGS()
#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
VKBP_ENABLE_WARNINGS()

HPPHlslShaders::HPPHlslShaders()
{
	title = "HPP HLSL shaders";

	zoom     = -2.0f;
	rotation = {0.0f, 0.0f, 0.0f};
}

HPPHlslShaders::~HPPHlslShaders()
{
	if (get_device() && get_device()->get_handle())
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class

		vk::Device device = get_device()->get_handle();
		device.destroyPipeline(pipeline);
		device.destroyPipelineLayout(pipeline_layout);
		device.destroyDescriptorSetLayout(base_descriptor_set_layout);
		device.destroyDescriptorSetLayout(sampler_descriptor_set_layout);
		// Delete the implicitly created sampler for the texture loaded via the framework
		device.destroySampler(texture.sampler);
	}
}

bool HPPHlslShaders::prepare(const vkb::ApplicationOptions &options)
{
	assert(!prepared);

	if (HPPApiVulkanSample::prepare(options))
	{
		load_assets();
		generate_quad();

		// Vertex shader uniform buffer block
		uniform_buffer_vs = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
		                                                           sizeof(ubo_vs),
		                                                           vk::BufferUsageFlagBits::eUniformBuffer,
		                                                           VMA_MEMORY_USAGE_CPU_TO_GPU);
		update_uniform_buffers();

		// We separate the descriptor sets for the uniform buffer + image and samplers, so we don't need to duplicate the descriptors for the former
		base_descriptor_set_layout    = create_base_descriptor_set_layout();
		sampler_descriptor_set_layout = create_sampler_descriptor_set_layout();

		pipeline_layout = create_pipeline_layout();
		shader_modules.push_back(create_shader_module("hlsl_shaders/hlsl_shader.vert", vk::ShaderStageFlagBits::eVertex));
		shader_modules.push_back(create_shader_module("hlsl_shaders/hlsl_shader.frag", vk::ShaderStageFlagBits::eFragment));
		pipeline            = create_pipeline();
		descriptor_pool     = create_descriptor_pool();
		base_descriptor_set = vkb::common::allocate_descriptor_set(get_device()->get_handle(), descriptor_pool, base_descriptor_set_layout);
		update_descriptor_sets();
		build_command_buffers();

		prepared = true;
	}

	return prepared;
}

// Enable physical device features required for this example
void HPPHlslShaders::request_gpu_features(vkb::core::HPPPhysicalDevice &gpu)
{
	// Enable anisotropic filtering if supported
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

void HPPHlslShaders::build_command_buffers()
{
	vk::CommandBufferBeginInfo command_buffer_begin_info;

	std::array<vk::ClearValue, 2> clear_values = {{default_clear_color, vk::ClearDepthStencilValue(0.0f, 0)}};

	vk::RenderPassBeginInfo render_pass_begin_info(render_pass, {}, {{0, 0}, extent}, clear_values);

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		// Set target frame buffer
		render_pass_begin_info.framebuffer = framebuffers[i];

		auto command_buffer = draw_cmd_buffers[i];
		command_buffer.begin(command_buffer_begin_info);

		command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f);
		command_buffer.setViewport(0, viewport);

		vk::Rect2D scissor({0, 0}, extent);
		command_buffer.setScissor(0, scissor);

		// Bind the uniform buffer and sampled image to set 0
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, base_descriptor_set, {});
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		vk::DeviceSize offset = 0;
		command_buffer.bindVertexBuffers(0, vertex_buffer->get_handle(), offset);
		command_buffer.bindIndexBuffer(index_buffer->get_handle(), 0, vk::IndexType::eUint32);

		command_buffer.drawIndexed(static_cast<uint32_t>(index_buffer->get_size() / sizeof(uint32_t)), 1, 0, 0, 0);

		draw_ui(command_buffer);

		command_buffer.endRenderPass();

		command_buffer.end();
	}
}

void HPPHlslShaders::render(float delta_time)
{
	if (prepared)
	{
		draw();
	}
}

void HPPHlslShaders::view_changed()
{
	update_uniform_buffers();
}

vk::DescriptorSetLayout HPPHlslShaders::create_base_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> base_set_layout_bindings = {
	    {{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},                   // Binding 0 : Vertex shader uniform buffer
	     {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}};        // Binding 1 : Fragment shader combined sampler

	return get_device()->get_handle().createDescriptorSetLayout({{}, base_set_layout_bindings});
}

vk::DescriptorPool HPPHlslShaders::create_descriptor_pool()
{
	std::array<vk::DescriptorPoolSize, 3> pool_sizes = {
	    {{vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eCombinedImageSampler, 1}, {vk::DescriptorType::eSampler, 2}}};
	return get_device()->get_handle().createDescriptorPool({{}, 3, pool_sizes});
}

vk::Pipeline HPPHlslShaders::create_pipeline()
{
	size_t                                         vertex_shader_index = shader_modules.size() - 2;
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{{{}, vk::ShaderStageFlagBits::eVertex, shader_modules[vertex_shader_index], "main"},
	                                                             {{}, vk::ShaderStageFlagBits::eFragment, shader_modules[vertex_shader_index + 1], "main"}};

	// Vertex bindings and attributes
	vk::VertexInputBindingDescription                  vertex_input_binding(0, sizeof(VertexStructure), vk::VertexInputRate::eVertex);
	std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {
	    {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexStructure, pos)},             // Location 0 : Position
	     {1, 0, vk::Format::eR32G32Sfloat, offsetof(VertexStructure, uv)},                 // Location 1: Texture Coordinates
	     {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexStructure, normal)}}};        // Location 2 : Normal
	vk::PipelineVertexInputStateCreateInfo vertex_input_state({}, vertex_input_binding, vertex_input_attributes);

	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask =
	    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state({}, true, true, vk::CompareOp::eGreater);
	depth_stencil_state.back.compareOp = vk::CompareOp::eAlways;

	return vkb::common::create_graphics_pipeline(get_device()->get_handle(),
	                                             pipeline_cache,
	                                             shader_stages,
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
}

vk::PipelineLayout HPPHlslShaders::create_pipeline_layout()
{
	// Set layout for the base descriptors in set 0 and set layout for the sampler descriptors in set 1
	std::array<vk::DescriptorSetLayout, 2> set_layouts = {{base_descriptor_set_layout, sampler_descriptor_set_layout}};
	return get_device()->get_handle().createPipelineLayout({{}, set_layouts});
}

vk::DescriptorSetLayout HPPHlslShaders::create_sampler_descriptor_set_layout()
{
	vk::DescriptorSetLayoutBinding sampler_set_layout_binding(0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment);        // Binding 0: Fragment shader sampler

	return get_device()->get_handle().createDescriptorSetLayout({{}, sampler_set_layout_binding});
}

vk::ShaderModule HPPHlslShaders::create_shader_module(const std::string &file, vk::ShaderStageFlagBits stage)
{
	std::string info_log;

	// Compile HLSL to SPIR-V

	// Initialize glslang library
	glslang::InitializeProcess();

	auto messages = static_cast<EShMessages>(EShMsgReadHlsl | EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);

	EShLanguage language{};
	switch (stage)
	{
		case vk::ShaderStageFlagBits::eVertex:
			language = EShLangVertex;
			break;
		case vk::ShaderStageFlagBits::eFragment:
			language = EShLangFragment;
			break;
		default:
			assert(false);
	}

	std::string source        = vkb::fs::read_shader(file);
	const char *shader_source = source.data();

	glslang::TShader shader(language);
	shader.setStringsWithLengths(&shader_source, nullptr, 1);
	shader.setEnvInput(glslang::EShSourceHlsl, language, glslang::EShClientVulkan, 1);
	shader.setEntryPoint("main");
	shader.setSourceEntryPoint("main");
	shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
	shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);

	if (!shader.parse(GetDefaultResources(), 100, false, messages))
	{
		LOGE("Failed to parse HLSL shader, Error: {}", std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()));
		throw std::runtime_error("Failed to parse HLSL shader");
	}

	// Add shader to new program object
	glslang::TProgram program;
	program.addShader(&shader);

	// Link program
	if (!program.link(messages))
	{
		LOGE("Failed to compile HLSL shader, Error: {}", std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog()));
		throw std::runtime_error("Failed to compile HLSL shader");
	}

	if (shader.getInfoLog())
	{
		info_log += std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()) + "\n";
	}
	if (program.getInfoLog())
	{
		info_log += std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
	}

	// Translate to SPIRV
	glslang::TIntermediate *intermediate = program.getIntermediate(language);
	if (!intermediate)
	{
		LOGE("Failed to get shared intermediate code.");
		throw std::runtime_error("Failed to compile HLSL shader");
	}

	spv::SpvBuildLogger logger;

	std::vector<uint32_t> spirvCode;
	glslang::GlslangToSpv(*intermediate, spirvCode, &logger);

	info_log += logger.getAllMessages() + "\n";

	glslang::FinalizeProcess();

	// Create shader module from generated SPIR-V
	return get_device()->get_handle().createShaderModule({{}, spirvCode});
}

void HPPHlslShaders::draw()
{
	HPPApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.setCommandBuffers(draw_cmd_buffers[current_buffer]);

	// Submit to queue
	queue.submit(submit_info);

	HPPApiVulkanSample::submit_frame();
}

void HPPHlslShaders::generate_quad()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	std::vector<VertexStructure> vertices =
	    {
	        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	        {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

	// Setup indices
	std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

	auto vertex_buffer_size = vkb::to_u32(vertices.size() * sizeof(VertexStructure));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                       vertex_buffer_size,
	                                                       vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
	                                                       VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::HPPBuffer>(*get_device(),
	                                                      index_buffer_size,
	                                                      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
	                                                      VMA_MEMORY_USAGE_CPU_TO_GPU);

	index_buffer->update(indices.data(), index_buffer_size);
}

void HPPHlslShaders::load_assets()
{
	texture = load_texture("textures/metalplate01_rgba.ktx", vkb::scene_graph::components::HPPImage::Color);
}

void HPPHlslShaders::update_descriptor_sets()
{
	vk::DescriptorBufferInfo buffer_descriptor(uniform_buffer_vs->get_handle(), 0, VK_WHOLE_SIZE);

	// Combined image descriptor for the texture
	vk::DescriptorImageInfo image_descriptor(
	    texture.sampler,
	    texture.image->get_vk_image_view().get_handle(),
	    descriptor_type_to_image_layout(vk::DescriptorType::eCombinedImageSampler, texture.image->get_vk_image_view().get_format()));

	std::array<vk::WriteDescriptorSet, 2> write_descriptor_sets = {
	    {{base_descriptor_set, 0, 0, vk::DescriptorType::eUniformBuffer, {}, buffer_descriptor},            // Binding 0 : Vertex shader uniform buffer
	     {base_descriptor_set, 1, 0, vk::DescriptorType::eCombinedImageSampler, image_descriptor}}};        // Binding 1 : Color map

	get_device()->get_handle().updateDescriptorSets(write_descriptor_sets, {});
}

void HPPHlslShaders::update_uniform_buffers()
{
	// Vertex shader
	ubo_vs.projection     = glm::perspective(glm::radians(60.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.001f, 256.0f);
	glm::mat4 view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

	ubo_vs.model = view_matrix * glm::translate(glm::mat4(1.0f), camera_pos);
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_vs.model = glm::rotate(ubo_vs.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo_vs.view_pos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

	uniform_buffer_vs->convert_and_update(ubo_vs);
}

std::unique_ptr<vkb::Application> create_hpp_hlsl_shaders()
{
	return std::make_unique<HPPHlslShaders>();
}
