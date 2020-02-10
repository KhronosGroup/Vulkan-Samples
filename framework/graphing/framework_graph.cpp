/* Copyright (c) 2019-2020, Arm Limited and Contributors
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

#include "graphing/framework_graph.h"

#include "common/strings.h"

namespace vkb
{
namespace graphing
{
namespace framework_graph
{
bool generate(RenderContext &context)
{
	Graph graph("Framework");
	graph.new_style("Core", "#00BCD4");
	graph.new_style("Rendering", "#4CAF50");
	graph.new_style("Framework", "#FFC107");
	graph.new_style("Vulkan", "#F44336");

	size_t device_id = device_node(graph, context.get_device());

	//Device
	auto &device = context.get_device();

	auto & resource_cache    = device.get_resource_cache();
	size_t resource_cache_id = resource_cache_node(graph, resource_cache);
	graph.add_edge(device_id, resource_cache_id);

	const auto &resource_cache_state = resource_cache.get_internal_state();

	auto it_pipeline_layouts = resource_cache_state.pipeline_layouts.begin();
	while (it_pipeline_layouts != resource_cache_state.pipeline_layouts.end())
	{
		size_t pipeline_layouts_id = pipeline_layout_node(graph, it_pipeline_layouts->second);
		graph.add_edge(resource_cache_id, pipeline_layouts_id);

		auto &modules = it_pipeline_layouts->second.get_shader_modules();
		for (const auto &shader_module : modules)
		{
			size_t shader_modules_id = shader_module_node(graph, *shader_module);
			graph.add_edge(pipeline_layouts_id, shader_modules_id);

			const auto &resources = shader_module->get_resources();
			for (const auto &resource : resources)
			{
				size_t resource_id = shader_resource_node(graph, resource);
				graph.add_edge(shader_modules_id, resource_id);
			}
		}

		it_pipeline_layouts++;
	}

	auto it_descriptor_set_layouts = resource_cache_state.descriptor_set_layouts.begin();
	while (it_descriptor_set_layouts != resource_cache_state.descriptor_set_layouts.end())
	{
		size_t descriptor_set_layouts_id = descriptor_set_layout_node(graph, it_descriptor_set_layouts->second);
		graph.add_edge(resource_cache_id, descriptor_set_layouts_id);
		it_descriptor_set_layouts++;
	}
	auto it_graphics_pipelines = resource_cache_state.graphics_pipelines.begin();
	while (it_graphics_pipelines != resource_cache_state.graphics_pipelines.end())
	{
		size_t pipeline_layout = pipeline_layout_node(graph, it_graphics_pipelines->second.get_state().get_pipeline_layout());
		graph.add_edge(resource_cache_id, pipeline_layout);

		size_t graphics_pipelines_id = graphics_pipeline_node(graph, it_graphics_pipelines->second);
		graph.add_edge(pipeline_layout, graphics_pipelines_id);

		size_t graphics_pipelines_state_id = pipeline_state_node(graph, it_graphics_pipelines->second.get_state());
		graph.add_edge(graphics_pipelines_id, graphics_pipelines_state_id);

		size_t render_pass = render_pass_node(graph, *it_graphics_pipelines->second.get_state().get_render_pass());
		graph.add_edge(graphics_pipelines_state_id, render_pass);

		size_t specialization_constant_state = specialization_constant_state_node(graph, it_graphics_pipelines->second.get_state().get_specialization_constant_state());
		graph.add_edge(graphics_pipelines_state_id, specialization_constant_state);

		size_t vertex_input_state = vertex_input_state_node(graph, it_graphics_pipelines->second.get_state().get_vertex_input_state());
		graph.add_edge(graphics_pipelines_state_id, vertex_input_state);

		size_t input_assembly_state = input_assembly_state_node(graph, it_graphics_pipelines->second.get_state().get_input_assembly_state());
		graph.add_edge(graphics_pipelines_state_id, input_assembly_state);

		size_t rasterization_state = rasterization_state_node(graph, it_graphics_pipelines->second.get_state().get_rasterization_state());
		graph.add_edge(graphics_pipelines_state_id, rasterization_state);

		size_t viewport_state = viewport_state_node(graph, it_graphics_pipelines->second.get_state().get_viewport_state());
		graph.add_edge(graphics_pipelines_state_id, viewport_state);

		size_t multisample_state = multisample_state_node(graph, it_graphics_pipelines->second.get_state().get_multisample_state());
		graph.add_edge(graphics_pipelines_state_id, multisample_state);

		size_t depth_stencil_state = depth_stencil_state_node(graph, it_graphics_pipelines->second.get_state().get_depth_stencil_state());
		graph.add_edge(graphics_pipelines_state_id, depth_stencil_state);

		size_t color_blend_state = color_blend_state_node(graph, it_graphics_pipelines->second.get_state().get_color_blend_state());
		graph.add_edge(graphics_pipelines_state_id, color_blend_state);
		it_graphics_pipelines++;
	}
	auto it_compute_pipelines = resource_cache_state.compute_pipelines.begin();
	while (it_compute_pipelines != resource_cache_state.compute_pipelines.end())
	{
		size_t compute_pipelines_id = compute_pipeline_node(graph, it_compute_pipelines->second);
		graph.add_edge(resource_cache_id, compute_pipelines_id);
		it_compute_pipelines++;
	}

	auto it_framebuffers = resource_cache_state.framebuffers.begin();
	while (it_framebuffers != resource_cache_state.framebuffers.end())
	{
		size_t framebuffers_id = framebuffer_node(graph, it_framebuffers->second);
		graph.add_edge(resource_cache_id, framebuffers_id);
		it_framebuffers++;
	}

	size_t render_context_id = render_context_node(graph, context);
	graph.add_edge(device_id, render_context_id);
	size_t swapchain_id = swapchain_node(graph, context.get_swapchain());

	for (auto image : context.get_swapchain().get_images())
	{
		size_t vkimage_id = create_vk_image(graph, image);
		graph.add_edge(vkimage_id, swapchain_id);
	}

	const auto &const_context = context;
	const auto &frames        = context.get_render_frames();
	uint32_t    i             = 0;
	for (auto &frame : frames)
	{
		std::string label = "Render Frame";
		if (i == const_context.get_active_frame_index())
		{
			label = "Last " + label;
		}
		i++;

		size_t frame_id = render_frame_node(graph, frame, label);
		graph.add_edge(render_context_id, frame_id);

		size_t semaphore_pool_id = semaphore_pool_node(graph, frame->get_semaphore_pool());
		size_t fence_pool_id     = fence_pool_node(graph, frame->get_fence_pool());
		size_t render_target_id  = render_target_node(graph, frame->get_render_target_const());
		graph.add_edge(frame_id, semaphore_pool_id);
		graph.add_edge(frame_id, fence_pool_id);
		graph.add_edge(frame_id, render_target_id);

		for (const auto &view : frame->get_render_target_const().get_views())
		{
			size_t      image_view_id = image_view_node(graph, view);
			const auto &image         = view.get_image();
			size_t      image_id      = image_node(graph, image);

			graph.add_edge(render_target_id, image_view_id);
			graph.add_edge(image_view_id, image_id);

			size_t vkimage_id = create_vk_image(graph, image.get_handle());
			graph.add_edge(image_id, vkimage_id);

			size_t vkimageview_id = create_vk_image_view(graph, view.get_handle());
			graph.add_edge(image_view_id, vkimageview_id);
		}
	}

	return graph.dump_to_file("framework.json");
}

size_t create_vk_image(Graph &graph, const VkImage &image)
{
	return create_vk_node(graph, "VkImage", image);
}

size_t create_vk_image_view(Graph &graph, const VkImageView &image)
{
	return create_vk_node(graph, "VkImageView", image);
}

size_t device_node(Graph &graph, const Device &device)
{
	auto pd_props = device.get_gpu().get_properties();

	nlohmann::json device_properties = {
	    {"deviceID", pd_props.deviceID},
	    {"deviceName", pd_props.deviceName},
	    {"deviceType", to_string(pd_props.deviceType)},
	    {"driverVersion", pd_props.driverVersion},
	    {"apiVersion", pd_props.apiVersion},
	    {"vendorID", pd_props.vendorID}};

	nlohmann::json data = {{"VkPhysicalDeviceProperties", device_properties}};

	return graph.create_node("Device", "Core", data);
}

size_t render_context_node(Graph &graph, const RenderContext &context)
{
	auto surface = context.get_surface_extent();

	nlohmann::json data = {{"VkExtent2D", {{"width", surface.width}, {"height", surface.height}}},
	                       {"active_frame_index", context.get_active_frame_index()}};

	return graph.create_node("Render Context", "Rendering", data);
}

size_t semaphore_pool_node(Graph &graph, const SemaphorePool &semaphore_pool)
{
	nlohmann::json data = {{"active_semaphore_count", semaphore_pool.get_active_semaphore_count()}};
	return graph.create_node("Semaphore Pool", "Framework", data);
}

size_t fence_pool_node(Graph &graph, const FencePool &fence_pool)
{
	return graph.create_node("Fence Pool", "Framework");
}

size_t render_frame_node(Graph &graph, const std::unique_ptr<RenderFrame> &frame, std::string label)
{
	return graph.create_node(label.c_str(), "Rendering");
}

size_t render_target_node(Graph &graph, const RenderTarget &render_target)
{
	VkExtent2D surface = render_target.get_extent();

	nlohmann::json data = {{"VkExtent2D", {{"width", surface.width}, {"height", surface.height}}},
	                       {"ImageView_count", render_target.get_views().size()},
	                       {"Attachment_count", render_target.get_attachments().size()},
	                       {"output_attachment_count", render_target.get_output_attachments().size()}};

	return graph.create_node("Render Target", "Rendering", data);
}

size_t image_view_node(Graph &graph, const core::ImageView &image_view)
{
	auto subresource_range  = image_view.get_subresource_range();
	auto subresource_layers = image_view.get_subresource_layers();

	nlohmann::json data = {{"VkFormat", to_string(image_view.get_format())},
	                       {"VkImageSubresourceRange",
	                        {{"VkImageAspectFlags", to_string_vk_image_aspect_flags(subresource_range.aspectMask)},
	                         {"base_mip_level", subresource_range.baseMipLevel},
	                         {"level_count", subresource_range.levelCount},
	                         {"base_array_layer", subresource_range.baseArrayLayer},
	                         {"layer_count", subresource_range.layerCount}}},
	                       {"VkImageSubresourceLayers",
	                        {{"VkImageAspectFlags", to_string_vk_image_aspect_flags(subresource_layers.aspectMask)},
	                         {"mip_level", subresource_layers.mipLevel},
	                         {"base_array_layer", subresource_layers.baseArrayLayer},
	                         {"layer_count", subresource_layers.layerCount}}}};

	return graph.create_node("Image View", "Core", data);
}

size_t image_node(Graph &graph, const core::Image &image)
{
	std::string result{""};
	bool        append = false;
	auto        flags  = image.get_usage();
	if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		result += append ? " / " : "";
		result += "COLOR";
		append = true;
	}
	if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		result += append ? " / " : "";
		result += "DEPTH STENCIL";
		append = true;
	}
	auto subresource = image.get_subresource();

	nlohmann::json data = {{"VkExtent2D", {{"width", image.get_extent().width}, {"height", image.get_extent().height}}},
	                       {"VkFormat", to_string(image.get_format())},
	                       {"VkImageUsageFlags", to_string_vk_image_usage_flags(image.get_usage())},
	                       {"VkSampleCountFlagBits", to_string(image.get_sample_count())},
	                       {"VkImageTiling", to_string(image.get_tiling())},
	                       {"VkImageType", to_string(image.get_type())},
	                       {"VkSubresource", {{"VkImageAspectFlags", to_string_vk_image_aspect_flags(subresource.aspectMask)}, {"mip_level", subresource.mipLevel}, {"array_layer", subresource.arrayLayer}}}};

	return graph.create_node(result.c_str(), "Core", data);
}

size_t swapchain_node(Graph &graph, const Swapchain &swapchain)
{
	auto surface     = swapchain.get_extent();
	auto format      = swapchain.get_format();
	auto image_count = swapchain.get_images().size();

	nlohmann::json data = {{"VkExtent2D", {{"width", surface.width}, {"height", surface.height}}},
	                       {"VkFormat", to_string(format)},
	                       {"image_count", image_count},
	                       {"VkSurfaceTransformFlagBitsKHR", to_string(swapchain.get_transform())},
	                       {"VkPresentModeKHR", to_string(swapchain.get_present_mode())},
	                       {"VkImageUsageFlags", to_string_vk_image_usage_flags(swapchain.get_usage())}};

	return graph.create_node("Swapchain", "Core", data);
}

size_t resource_cache_node(Graph &graph, const ResourceCache &resource_cache)
{
	return graph.create_node("Resource Cache", "Core");
}

size_t descriptor_set_layout_node(Graph &graph, const DescriptorSetLayout &descriptor_set_layout)
{
	std::vector<nlohmann::json> bindings;

	auto it = descriptor_set_layout.get_bindings().begin();
	while (it != descriptor_set_layout.get_bindings().end())
	{
		bindings.push_back({{"binding", it->binding},
		                    {"descriptorCount", it->descriptorCount},
		                    {"stageFlags", to_string(it->stageFlags)}});
		it++;
	}

	nlohmann::json data = {
	    {"handle", Node::handle_to_uintptr_t(descriptor_set_layout.get_handle())},
	    {"VkDescriptorSetLayoutBinding", bindings}};

	return graph.create_node("Descriptor Set Layout", "Core", data);
}

size_t framebuffer_node(Graph &graph, const Framebuffer &framebuffer)
{
	nlohmann::json data = {
	    {"handle", Node::handle_to_uintptr_t(framebuffer.get_handle())}};

	return graph.create_node("Frame Buffer", "Core", data);
}

size_t render_pass_node(Graph &graph, const RenderPass &render_pass)
{
	nlohmann::json data = {
	    {"handle", Node::handle_to_uintptr_t(render_pass.get_handle())}};

	return graph.create_node("Render Pass", "Rendering", data);
}

size_t shader_module_node(Graph &graph, const ShaderModule &shader_module)
{
	std::string stage = to_string_vk_shader_stage_flags(shader_module.get_stage());
	std::transform(stage.begin(), stage.end(), stage.begin(),
	               [](unsigned char c) { return std::tolower(c); });

	nlohmann::json data = {{"stage", stage},
	                       {"infoLog", shader_module.get_info_log()},
	                       {"entry_point", shader_module.get_entry_point()},
	                       {"id", shader_module.get_id()}};

	stage = "Shader Module: " + stage;

	return graph.create_node(stage.c_str(), "Rendering", data);
}

size_t shader_resource_node(Graph &graph, const ShaderResource &shader_resource)
{
	std::string label = fmt::format("{}: {}", to_string(shader_resource.type), shader_resource.name);

	nlohmann::json data = {{"ShaderResourceType", to_string(shader_resource.type)},
	                       {"VkShaderStageFlags", to_string(shader_resource.stages)},
	                       {"set", shader_resource.set},
	                       {"binding", shader_resource.binding},
	                       {"location", shader_resource.location},
	                       {"input_attachment_index", shader_resource.input_attachment_index},
	                       {"vec_size", shader_resource.vec_size},
	                       {"columns", shader_resource.columns},
	                       {"array_size", shader_resource.array_size},
	                       {"offset", shader_resource.offset},
	                       {"size", shader_resource.size},
	                       {"constant_id", shader_resource.constant_id},
	                       {"dynamic", shader_resource.dynamic},
	                       {"name", shader_resource.name}};

	return graph.create_node(label.c_str(), "Rendering", data);
}

size_t pipeline_layout_node(Graph &graph, const PipelineLayout &pipeline_layout)
{
	nlohmann::json data = {{"handle", Node::handle_to_uintptr_t(pipeline_layout.get_handle())}};

	return graph.create_node("Pipeline Layout", "Core", data);
}

size_t graphics_pipeline_node(Graph &graph, const GraphicsPipeline &graphics_pipeline)
{
	nlohmann::json data = {
	    {"handle", Node::handle_to_uintptr_t(graphics_pipeline.get_handle())}};

	return graph.create_node("Graphics Pipeline", "Core", data);
}

size_t compute_pipeline_node(Graph &graph, const ComputePipeline &compute_pipeline)
{
	nlohmann::json data = {
	    {"handle", Node::handle_to_uintptr_t(compute_pipeline.get_handle())}};

	return graph.create_node("Compute Pipeline", "Core", data);
}

size_t pipeline_state_node(Graph &graph, const PipelineState &pipeline_state)
{
	nlohmann::json data = {{"subpass_index", pipeline_state.get_subpass_index()}};

	return graph.create_node("Pipeline State", "Core", data);
}

size_t descriptor_set_node(Graph &graph, const DescriptorSet &descriptor_set)
{
	nlohmann::json data = {
	    {"handle", Node::handle_to_uintptr_t(descriptor_set.get_handle())}};

	return graph.create_node("Descriptor Set", "Core", data);
}

size_t specialization_constant_state_node(Graph &graph, const SpecializationConstantState &specialization_constant_state)
{
	nlohmann::json data = {};

	const auto &state = specialization_constant_state.get_specialization_constant_state();
	auto        it    = state.begin();
	while (it != state.end())
	{
		std::stringstream str;
		str << it->first;
		data.push_back({str.str(), it->second});
	}

	return graph.create_node("Specialization Constant state", "Core", data);
}

size_t vertex_input_state_node(Graph &graph, const VertexInputState &vertex_input_state)
{
	std::vector<nlohmann::json> bindings;

	for (auto binding : vertex_input_state.bindings)
	{
		bindings.push_back({{"binding", binding.binding},
		                    {"stride", binding.stride},
		                    {"VkVertexInputRate", to_string(binding.inputRate)}});
	}

	std::vector<nlohmann::json> binding;

	for (auto attribute : vertex_input_state.attributes)
	{
		binding.push_back({{"location", attribute.location},
		                   {"binding", attribute.binding},
		                   {"format", to_string(attribute.format)},
		                   {"offset", attribute.offset}});
	}

	nlohmann::json data = {{"VkVertexInputBindingDescription", bindings},
	                       {"VkVertexInputAttributeDescription", binding}};

	return graph.create_node("Vertex Input State", "Core", data);
}

size_t input_assembly_state_node(Graph &graph, const InputAssemblyState &input_assembly_state)
{
	nlohmann::json data = {
	    {"VkPrimitiveTopology", to_string(input_assembly_state.topology)},
	    {"primitive_restart_enabled", to_string_vk_bool(input_assembly_state.primitive_restart_enable)}};

	return graph.create_node("Input Assembly State", "Core", data);
}

size_t rasterization_state_node(Graph &graph, const RasterizationState &rasterization_state)
{
	nlohmann::json data = {
	    {"depth_clamp_enable", to_string_vk_bool(rasterization_state.depth_clamp_enable)},
	    {"rasterizer_discard_enable", to_string_vk_bool(rasterization_state.rasterizer_discard_enable)},
	    {"polygon_mode", to_string(rasterization_state.polygon_mode)},
	    {"cull_mode", to_string_vk_cull_mode_flags(rasterization_state.cull_mode)},
	    {"front_face", to_string(rasterization_state.front_face)},
	    {"depth_bias_enable", to_string_vk_bool(rasterization_state.depth_bias_enable)},
	};

	return graph.create_node("Rasterization State", "Core", data);
}

size_t viewport_state_node(Graph &graph, const ViewportState &viewport_state)
{
	nlohmann::json data = {
	    {"viewport_count", viewport_state.viewport_count},
	    {"scissor_count", viewport_state.scissor_count}};

	return graph.create_node("Viewport State", "Core", data);
}

size_t multisample_state_node(Graph &graph, const MultisampleState &multisample_state)
{
	nlohmann::json data = {
	    {"rasterization_samples", multisample_state.rasterization_samples},
	    {"sample_shading_enable", to_string_vk_bool(multisample_state.sample_shading_enable)},
	    {"min_sample_shading", multisample_state.min_sample_shading},
	    {"sample_mask", multisample_state.sample_mask},
	    {"alpha_to_coverage_enable", to_string_vk_bool(multisample_state.alpha_to_coverage_enable)},
	    {"alpha_to_one_enable", to_string_vk_bool(multisample_state.alpha_to_one_enable)}};

	return graph.create_node("Multisample State", "Core", data);
}

size_t depth_stencil_state_node(Graph &graph, const DepthStencilState &depth_stencil_state)
{
	nlohmann::json data = {
	    {"depth_test_enable", to_string_vk_bool(depth_stencil_state.depth_test_enable)},
	    {"depth_write_enable", to_string_vk_bool(depth_stencil_state.depth_write_enable)},
	    {"depth_compare_op", to_string(depth_stencil_state.depth_compare_op)},
	    {"depth_bounds_test_enable", to_string_vk_bool(depth_stencil_state.depth_bounds_test_enable)},
	    {"stencil_test_enable", to_string_vk_bool(depth_stencil_state.stencil_test_enable)},
	    {"front", {{"fail_op", to_string(depth_stencil_state.front.fail_op)}, {"pass_op", to_string(depth_stencil_state.front.pass_op)}, {"depth_fail_op", to_string(depth_stencil_state.front.depth_fail_op)}, {"compare_op", to_string(depth_stencil_state.front.compare_op)}}},
	    {"back", {{"fail_op", to_string(depth_stencil_state.back.fail_op)}, {"pass_op", to_string(depth_stencil_state.back.pass_op)}, {"depth_fail_op", to_string(depth_stencil_state.back.depth_fail_op)}, {"compare_op", to_string(depth_stencil_state.back.compare_op)}}}};

	return graph.create_node("Depth Stencil State", "Core", data);
}

size_t color_blend_state_node(Graph &graph, const ColorBlendState &color_blend_state)
{
	nlohmann::json data = {
	    {"logic_op_enable", to_string_vk_bool(color_blend_state.logic_op_enable)},
	    {"logic_op", to_string(color_blend_state.logic_op)}};

	return graph.create_node("Color Blend State", "Core", data);
}

size_t color_blend_attachment_state_node(Graph &graph, const ColorBlendAttachmentState &state)
{
	nlohmann::json data = {
	    {"blend_enable", to_string_vk_bool(state.blend_enable)},
	    {"src_color_blend_factor", to_string(state.src_color_blend_factor)},
	    {"dst_color_blend_factor", to_string(state.dst_color_blend_factor)},
	    {"color_blend_op", to_string(state.color_blend_op)},
	    {"src_alpha_blend_factor", to_string(state.src_alpha_blend_factor)},
	    {"dst_alpha_blend_factor", to_string(state.dst_alpha_blend_factor)},
	    {"alpha_blend_op", to_string(state.alpha_blend_op)},
	    {"color_write_mask", to_string_vk_color_component_flags(state.color_write_mask)}};

	return graph.create_node("Color Blend Attachment State", "Core", data);
}

}        // namespace framework_graph
}        // namespace graphing
}        // namespace vkb
