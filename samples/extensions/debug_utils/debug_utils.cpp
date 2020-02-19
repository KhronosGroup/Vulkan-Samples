/* Copyright (c) 2020, Sascha Willems
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
 * Debug Utils labeling
 * Note that you need to run this example inside a debugging tool like RenderDoc to see those labels
 */

#include "debug_utils.h"

#include "scene_graph/components/sub_mesh.h"

DebugUtils::DebugUtils()
{
	title = "Debug Utils";
}

DebugUtils::~DebugUtils()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), pipelines.skysphere, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.sphere, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.composition, nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.bloom[0], nullptr);
		vkDestroyPipeline(get_device().get_handle(), pipelines.bloom[1], nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.models, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.composition, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.bloom_filter, nullptr);

		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.models, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.composition, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layouts.bloom_filter, nullptr);

		vkDestroyRenderPass(get_device().get_handle(), offscreen.render_pass, nullptr);
		vkDestroyRenderPass(get_device().get_handle(), filter_pass.render_pass, nullptr);

		vkDestroyFramebuffer(get_device().get_handle(), offscreen.framebuffer, nullptr);
		vkDestroyFramebuffer(get_device().get_handle(), filter_pass.framebuffer, nullptr);

		vkDestroySampler(get_device().get_handle(), offscreen.sampler, nullptr);
		vkDestroySampler(get_device().get_handle(), filter_pass.sampler, nullptr);

		offscreen.depth.destroy(get_device().get_handle());
		offscreen.color[0].destroy(get_device().get_handle());
		offscreen.color[1].destroy(get_device().get_handle());

		filter_pass.color[0].destroy(get_device().get_handle());

		vkDestroySampler(get_device().get_handle(), textures.skysphere.sampler, nullptr);
	}
}

/* 
 * Checks if the required extension is supported at instance level
 */
void DebugUtils::debug_check_extension()
{
	std::vector<const char *> enabled_instance_extensions = instance->get_extensions();
	for (auto &enabled_extension : enabled_instance_extensions)
	{
		if (strcmp(enabled_extension, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
		{
			debug_utils_supported = true;
			break;
		}
	}
	if (!debug_utils_supported)
	{
		LOGE("Required extension {} not supported or available, no debugging possible with this sample", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		LOGE("Make sure to compile the sample in debug mode and/or enable the validation layers");
	}
}

/*
 * Command buffer debug labeling functions
 */

void DebugUtils::cmd_begin_label(VkCommandBuffer command_buffer, const char *label_name, std::vector<float> color)
{
	if (!debug_utils_supported)
	{
		return;
	}
	VkDebugUtilsLabelEXT label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName           = label_name;
	label.color[0]             = color[0];
	label.color[1]             = color[1];
	label.color[2]             = color[2];
	label.color[3]             = color[3];
	vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label);
}

void DebugUtils::cmd_insert_label(VkCommandBuffer command_buffer, const char *label_name, std::vector<float> color)
{
	if (!debug_utils_supported)
	{
		return;
	}
	VkDebugUtilsLabelEXT label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName           = label_name;
	label.color[0]             = color[0];
	label.color[1]             = color[1];
	label.color[2]             = color[2];
	label.color[3]             = color[3];
	vkCmdInsertDebugUtilsLabelEXT(command_buffer, &label);
}

void DebugUtils::cmd_end_label(VkCommandBuffer command_buffer)
{
	if (!debug_utils_supported)
	{
		return;
	}
	vkCmdEndDebugUtilsLabelEXT(command_buffer);
}

/*
 * Queue debug labeling functions
 */

void DebugUtils::queue_begin_label(VkQueue queue, const char *label_name, std::vector<float> color)
{
	if (!debug_utils_supported)
	{
		return;
	}
	VkDebugUtilsLabelEXT label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName           = label_name;
	label.color[0]             = color[0];
	label.color[1]             = color[1];
	label.color[2]             = color[2];
	label.color[3]             = color[3];
	vkQueueBeginDebugUtilsLabelEXT(queue, &label);
}

void DebugUtils::queue_insert_label(VkQueue queue, const char *label_name, std::vector<float> color)
{
	if (!debug_utils_supported)
	{
		return;
	}
	VkDebugUtilsLabelEXT label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
	label.pLabelName           = label_name;
	label.color[0]             = color[0];
	label.color[1]             = color[1];
	label.color[2]             = color[2];
	label.color[3]             = color[3];
	vkQueueInsertDebugUtilsLabelEXT(queue, &label);
}

void DebugUtils::queue_end_label(VkQueue queue)
{
	if (!debug_utils_supported)
	{
		return;
	}
	vkQueueEndDebugUtilsLabelEXT(queue);
}

/*
 * Object naming and tagging functions
 */

void DebugUtils::set_object_name(VkObjectType object_type, uint64_t object_handle, const char *object_name)
{
	if (!debug_utils_supported)
	{
		return;
	}
	VkDebugUtilsObjectNameInfoEXT name_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
	name_info.objectType                    = object_type;
	name_info.objectHandle                  = object_handle;
	name_info.pObjectName                   = object_name;
	vkSetDebugUtilsObjectNameEXT(device->get_handle(), &name_info);
}

/*
 * This sample uses a modified version of the shader loading function that adds a tag to the shader
 * The tag includes the source GLSL that can then be displayed by a debugging application
 */
VkPipelineShaderStageCreateInfo DebugUtils::debug_load_shader(const std::string &file, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shader_stage = {};
	shader_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage                           = stage;
	shader_stage.module                          = vkb::load_shader(file.c_str(), device->get_handle(), stage);
	shader_stage.pName                           = "main";
	assert(shader_stage.module != VK_NULL_HANDLE);
	shader_modules.push_back(shader_stage.module);

	if (debug_utils_supported)
	{
		// Name the sader (by file name)
		set_object_name(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t) shader_stage.module, std::string("Shader " + file).c_str());

		std::vector<uint8_t> buffer = vkb::fs::read_shader(file);
		// Pass the source GLSL shader code via an object tag
		VkDebugUtilsObjectTagInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT};
		info.objectType                   = VK_OBJECT_TYPE_SHADER_MODULE;
		info.objectHandle                 = (uint64_t) shader_stage.module;
		info.tagName                      = 1;
		info.tagSize                      = buffer.size() * sizeof(uint8_t);
		info.pTag                         = buffer.data();
		vkSetDebugUtilsObjectTagEXT(device->get_handle(), &info);
	}

	return shader_stage;
}

/*
 * Name and tag some Vulkan objects
 * All objects named in this function will appear with the those names in a debugging tool
 */
void DebugUtils::debug_name_objects()
{
	if (!debug_utils_supported)
	{
		return;
	}
	set_object_name(VK_OBJECT_TYPE_BUFFER, (uint64_t) uniform_buffers.matrices->get_handle(), "Matrices uniform buffer");

	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.skysphere, "Skysphere pipeline");
	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.composition, "Skysphere pipeline");
	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.sphere, "Sphere rendering pipeline");
	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.bloom[0], "Horizontal bloom filter pipeline");
	set_object_name(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipelines.bloom[1], "Vertical bloom filter pipeline");

	set_object_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t) pipeline_layouts.models, "Model rendering pipeline layout");
	set_object_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t) pipeline_layouts.composition, "Composition pass pipeline layout");
	set_object_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t) pipeline_layouts.bloom_filter, "Bloom filter pipeline layout");

	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_sets.sphere, "Sphere model descriptor set");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_sets.skysphere, "Sky sphere model descriptor set");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_sets.composition, "Composition pass descriptor set");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_sets.bloom_filter, "Bloom filter descriptor set");

	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) descriptor_set_layouts.models, "Model rendering descriptor set layout");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) descriptor_set_layouts.composition, "Composition pass set layout");
	set_object_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) descriptor_set_layouts.bloom_filter, "Bloom filter descriptor set layout");

	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) textures.skysphere.image->get_vk_image().get_handle(), "Sky sphere texture");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) offscreen.depth.image, "Offscreen pass depth image");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) offscreen.depth.image, "Offscreen pass depth image");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) offscreen.color[0].image, "Offscreen pass color image 0");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) offscreen.color[1].image, "Offscreen pass color image 1");
	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) filter_pass.color[0].image, "Bloom filter pass color image");

	set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) depth_stencil.image, "Base depth/stencil image");
	for (size_t i = 0; i < swapchain_buffers.size(); i++)
	{
		std::string name = "Swapchain image" + std::to_string(i);
		set_object_name(VK_OBJECT_TYPE_IMAGE, (uint64_t) swapchain_buffers[i].image, name.c_str());
	}

	set_object_name(VK_OBJECT_TYPE_SAMPLER, (uint64_t) offscreen.sampler, "Offscreen pass sampler");
	set_object_name(VK_OBJECT_TYPE_SAMPLER, (uint64_t) filter_pass.sampler, "Bloom filter pass sampler");

	set_object_name(VK_OBJECT_TYPE_RENDER_PASS, (uint64_t) offscreen.render_pass, "Offscreen pass render pass");
	set_object_name(VK_OBJECT_TYPE_RENDER_PASS, (uint64_t) filter_pass.render_pass, "Bloom filter pass render pass");
}

void DebugUtils::get_device_features()
{
	// Enable anisotropic filtering if supported
	if (supported_device_features.samplerAnisotropy)
	{
		requested_device_features.samplerAnisotropy = VK_TRUE;
	}
}

void DebugUtils::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass            = render_pass;
	render_pass_begin_info.renderArea.offset.x   = 0;
	render_pass_begin_info.renderArea.offset.y   = 0;
	render_pass_begin_info.clearValueCount       = 2;
	render_pass_begin_info.pClearValues          = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		{
			/*
				First pass: Render scene to offscreen framebuffer
			*/

			cmd_begin_label(draw_cmd_buffers[i], "Offscreen pass", {1.0f, 0.78f, 0.05f, 1.0f});

			std::array<VkClearValue, 3> clear_values;
			clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[1].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[2].depthStencil = {0.0f, 0};

			VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
			render_pass_begin_info.renderPass               = offscreen.render_pass;
			render_pass_begin_info.framebuffer              = offscreen.framebuffer;
			render_pass_begin_info.renderArea.extent.width  = offscreen.width;
			render_pass_begin_info.renderArea.extent.height = offscreen.height;
			render_pass_begin_info.clearValueCount          = 3;
			render_pass_begin_info.pClearValues             = clear_values.data();

			vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkb::initializers::viewport((float) offscreen.width, (float) offscreen.height, 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(offscreen.width, offscreen.height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = {0};

			if (display_skysphere)
			{
				cmd_insert_label(draw_cmd_buffers[i], "Draw sky sphere", {0.0f, 0.5f, 1.0f, 1.0f});

				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skysphere);
				push_const_block.object_type = 0;
				vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layouts.models, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block), &push_const_block);
				vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.skysphere, 0, NULL);

				draw_model(models.skysphere, draw_cmd_buffers[i]);
			}

			// Spheres
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.sphere);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.models, 0, 1, &descriptor_sets.sphere, 0, NULL);
			std::vector<glm::vec3> mesh_colors = {
			    glm::vec3(1.0f, 0.0f, 0.0f),
			    glm::vec3(0.0f, 1.0f, 0.0f),
			    glm::vec3(0.0f, 0.0f, 1.0f),
			};
			std::vector<glm::vec3> mesh_offsets = {
			    glm::vec3(-2.5f, 0.0f, 0.0f),
			    glm::vec3(0.0f, 0.0f, 0.0f),
			    glm::vec3(2.5f, 0.0f, 0.0f),
			};
			for (uint32_t j = 0; j < 3; j++)
			{
				push_const_block.object_type = 1;
				push_const_block.offset      = glm::vec4(mesh_offsets[j], 0.0f);
				push_const_block.color       = glm::vec4(mesh_colors[j], 0.0f);
				vkCmdPushConstants(draw_cmd_buffers[i], pipeline_layouts.models, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_const_block), &push_const_block);
				cmd_insert_label(draw_cmd_buffers[i], "Draw sphere", {mesh_colors[j].r, mesh_colors[j].g, mesh_colors[j].b, 1.0f});
				draw_model(models.scene, draw_cmd_buffers[i]);
			}

			vkCmdEndRenderPass(draw_cmd_buffers[i]);

			cmd_end_label(draw_cmd_buffers[i]);
		}

		/*
			Second render pass: First bloom pass
		*/
		if (bloom)
		{
			cmd_begin_label(draw_cmd_buffers[i], "Separable bloom filter", {0.5f, 0.76f, 0.34f, 1.0f});

			cmd_begin_label(draw_cmd_buffers[i], "Vertical bloom pass", {0.4f, 0.61f, 0.27f, 1.0f});

			VkClearValue clear_values[2];
			clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[1].depthStencil = {0.0f, 0};

			// Bloom filter
			VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
			render_pass_begin_info.framebuffer              = filter_pass.framebuffer;
			render_pass_begin_info.renderPass               = filter_pass.render_pass;
			render_pass_begin_info.clearValueCount          = 1;
			render_pass_begin_info.renderArea.extent.width  = filter_pass.width;
			render_pass_begin_info.renderArea.extent.height = filter_pass.height;
			render_pass_begin_info.pClearValues             = clear_values;

			vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkb::initializers::viewport((float) filter_pass.width, (float) filter_pass.height, 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(filter_pass.width, filter_pass.height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.bloom_filter, 0, 1, &descriptor_sets.bloom_filter, 0, NULL);

			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bloom[1]);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(draw_cmd_buffers[i]);

			cmd_end_label(draw_cmd_buffers[i]);
		}

		/*
			Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
		*/

		/*
			Third render pass: Scene rendering with applied second bloom pass (when enabled)
		*/
		{
			cmd_begin_label(draw_cmd_buffers[i], "Horizontal bloom pass and composition", {0.4f, 0.61f, 0.27f, 1.0f});

			VkClearValue clear_values[2];
			clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
			clear_values[1].depthStencil = {0.0f, 0};

			// Final composition
			VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
			render_pass_begin_info.framebuffer              = framebuffers[i];
			render_pass_begin_info.renderPass               = render_pass;
			render_pass_begin_info.clearValueCount          = 2;
			render_pass_begin_info.renderArea.extent.width  = width;
			render_pass_begin_info.renderArea.extent.height = height;
			render_pass_begin_info.pClearValues             = clear_values;

			vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
			vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.composition, 0, 1, &descriptor_sets.composition, 0, NULL);

			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

			// Bloom
			if (bloom)
			{
				cmd_insert_label(draw_cmd_buffers[i], "Bloom full screen quad", {1.0f, 1.0f, 1.0f, 1.0f});
				vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bloom[0]);
				vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);
			}

			draw_ui(draw_cmd_buffers[i]);

			vkCmdEndRenderPass(draw_cmd_buffers[i]);

			cmd_end_label(draw_cmd_buffers[i]);
			cmd_end_label(draw_cmd_buffers[i]);
		}

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void DebugUtils::create_attachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment)
{
	VkImageAspectFlags aspect_mask = 0;
	VkImageLayout      image_layout;

	attachment->format = format;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspect_mask  = VK_IMAGE_ASPECT_COLOR_BIT;
		image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
		// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
		if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
		{
			aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	assert(aspect_mask > 0);

	VkImageCreateInfo image = vkb::initializers::image_create_info();
	image.imageType         = VK_IMAGE_TYPE_2D;
	image.format            = format;
	image.extent.width      = offscreen.width;
	image.extent.height     = offscreen.height;
	image.extent.depth      = 1;
	image.mipLevels         = 1;
	image.arrayLayers       = 1;
	image.samples           = VK_SAMPLE_COUNT_1_BIT;
	image.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image.usage             = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image, nullptr, &attachment->image));
	vkGetImageMemoryRequirements(get_device().get_handle(), attachment->image, &memory_requirements);
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &attachment->mem));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), attachment->image, attachment->mem, 0));

	VkImageViewCreateInfo image_view_create_info           = vkb::initializers::image_view_create_info();
	image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format                          = format;
	image_view_create_info.subresourceRange                = {};
	image_view_create_info.subresourceRange.aspectMask     = aspect_mask;
	image_view_create_info.subresourceRange.baseMipLevel   = 0;
	image_view_create_info.subresourceRange.levelCount     = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount     = 1;
	image_view_create_info.image                           = attachment->image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view_create_info, nullptr, &attachment->view));
}

// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
void DebugUtils::prepare_offscreen_buffer()
{
	{
		offscreen.width  = width;
		offscreen.height = height;

		// Color attachments

		create_attachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[0]);
		create_attachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[1]);
		// Depth attachment
		create_attachment(depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &offscreen.depth);

		// Set up separate renderpass with references to the colorand depth attachments
		std::array<VkAttachmentDescription, 3> attachment_descriptions = {};

		// Init attachment properties
		for (uint32_t i = 0; i < 3; ++i)
		{
			attachment_descriptions[i].samples        = VK_SAMPLE_COUNT_1_BIT;
			attachment_descriptions[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment_descriptions[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
			attachment_descriptions[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment_descriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			if (i == 2)
			{
				attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachment_descriptions[i].finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else
			{
				attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachment_descriptions[i].finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
		}

		// Formats
		attachment_descriptions[0].format = offscreen.color[0].format;
		attachment_descriptions[1].format = offscreen.color[1].format;
		attachment_descriptions[2].format = offscreen.depth.format;

		std::vector<VkAttachmentReference> color_references;
		color_references.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
		color_references.push_back({1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

		VkAttachmentReference depth_reference = {};
		depth_reference.attachment            = 2;
		depth_reference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass    = {};
		subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments       = color_references.data();
		subpass.colorAttachmentCount    = 2;
		subpass.pDepthStencilAttachment = &depth_reference;

		// Use subpass dependencies for attachment layput transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass      = 0;
		dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass      = 0;
		dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo render_pass_create_info = {};
		render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.pAttachments           = attachment_descriptions.data();
		render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachment_descriptions.size());
		render_pass_create_info.subpassCount           = 1;
		render_pass_create_info.pSubpasses             = &subpass;
		render_pass_create_info.dependencyCount        = 2;
		render_pass_create_info.pDependencies          = dependencies.data();

		VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &offscreen.render_pass));

		std::array<VkImageView, 3> attachments;
		attachments[0] = offscreen.color[0].view;
		attachments[1] = offscreen.color[1].view;
		attachments[2] = offscreen.depth.view;

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.pNext                   = NULL;
		framebuffer_create_info.renderPass              = offscreen.render_pass;
		framebuffer_create_info.pAttachments            = attachments.data();
		framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
		framebuffer_create_info.width                   = offscreen.width;
		framebuffer_create_info.height                  = offscreen.height;
		framebuffer_create_info.layers                  = 1;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &offscreen.framebuffer));

		// Create sampler to sample from the color attachments
		VkSamplerCreateInfo sampler = vkb::initializers::sampler_create_info();
		sampler.magFilter           = VK_FILTER_NEAREST;
		sampler.minFilter           = VK_FILTER_NEAREST;
		sampler.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV        = sampler.addressModeU;
		sampler.addressModeW        = sampler.addressModeU;
		sampler.mipLodBias          = 0.0f;
		sampler.maxAnisotropy       = 1.0f;
		sampler.minLod              = 0.0f;
		sampler.maxLod              = 1.0f;
		sampler.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &offscreen.sampler));
	}

	// Bloom separable filter pass
	{
		filter_pass.width  = width;
		filter_pass.height = height;

		// Color attachments

		// Two color buffers
		create_attachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &filter_pass.color[0]);

		// Set up separate renderpass with references to the colorand depth attachments
		std::array<VkAttachmentDescription, 1> attachment_descriptions = {};

		// Init attachment properties
		attachment_descriptions[0].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment_descriptions[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment_descriptions[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachment_descriptions[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment_descriptions[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment_descriptions[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachment_descriptions[0].format         = filter_pass.color[0].format;

		std::vector<VkAttachmentReference> color_references;
		color_references.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments    = color_references.data();
		subpass.colorAttachmentCount = 1;

		// Use subpass dependencies for attachment layput transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass      = 0;
		dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass      = 0;
		dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo render_pass_create_info = {};
		render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.pAttachments           = attachment_descriptions.data();
		render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachment_descriptions.size());
		render_pass_create_info.subpassCount           = 1;
		render_pass_create_info.pSubpasses             = &subpass;
		render_pass_create_info.dependencyCount        = 2;
		render_pass_create_info.pDependencies          = dependencies.data();

		VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &filter_pass.render_pass));

		std::array<VkImageView, 1> attachments;
		attachments[0] = filter_pass.color[0].view;

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.pNext                   = NULL;
		framebuffer_create_info.renderPass              = filter_pass.render_pass;
		framebuffer_create_info.pAttachments            = attachments.data();
		framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
		framebuffer_create_info.width                   = filter_pass.width;
		framebuffer_create_info.height                  = filter_pass.height;
		framebuffer_create_info.layers                  = 1;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &filter_pass.framebuffer));

		// Create sampler to sample from the color attachments
		VkSamplerCreateInfo sampler = vkb::initializers::sampler_create_info();
		sampler.magFilter           = VK_FILTER_NEAREST;
		sampler.minFilter           = VK_FILTER_NEAREST;
		sampler.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV        = sampler.addressModeU;
		sampler.addressModeW        = sampler.addressModeU;
		sampler.mipLodBias          = 0.0f;
		sampler.maxAnisotropy       = 1.0f;
		sampler.minLod              = 0.0f;
		sampler.maxLod              = 1.0f;
		sampler.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &filter_pass.sampler));
	}
}

void DebugUtils::load_assets()
{
	models.skysphere   = load_model("scenes/geosphere.gltf");
	textures.skysphere = load_texture("textures/skysphere_rgba.ktx");
	models.scene       = load_model("scenes/geosphere.gltf");
}

void DebugUtils::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6)};
	uint32_t                   num_descriptor_sets = 4;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void DebugUtils::setup_descriptor_set_layout()
{
	// Object rendering (into offscreen buffer)
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
	};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.models));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layouts.models,
	        1);

	// Pass object offset and color via push constant
	VkPushConstantRange push_constant_range            = vkb::initializers::push_constant_range(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(push_const_block), 0);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.models));

	// Bloom filter
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};

	descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.bloom_filter));

	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.bloom_filter, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.bloom_filter));

	// G-Buffer composition
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};

	descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.composition));

	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.composition, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layouts.composition));
}

void DebugUtils::setup_descriptor_sets()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &descriptor_set_layouts.models,
	        1);

	// Sphere model object descriptor set
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.sphere));

	VkDescriptorBufferInfo            matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	VkDescriptorImageInfo             environment_image_descriptor = create_descriptor(textures.skysphere);
	std::vector<VkWriteDescriptorSet> write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.sphere, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.sphere, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	// Sky sphere descriptor set
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.skysphere));

	matrix_buffer_descriptor     = create_descriptor(*uniform_buffers.matrices);
	environment_image_descriptor = create_descriptor(textures.skysphere);
	write_descriptor_sets        = {
        vkb::initializers::write_descriptor_set(descriptor_sets.skysphere, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matrix_buffer_descriptor),
        vkb::initializers::write_descriptor_set(descriptor_sets.skysphere, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &environment_image_descriptor),
    };
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	// Bloom filter
	alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.bloom_filter, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.bloom_filter));

	std::vector<VkDescriptorImageInfo> color_descriptors = {
	    vkb::initializers::descriptor_image_info(offscreen.sampler, offscreen.color[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	    vkb::initializers::descriptor_image_info(offscreen.sampler, offscreen.color[1].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	};

	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.bloom_filter, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &color_descriptors[0]),
	    vkb::initializers::write_descriptor_set(descriptor_sets.bloom_filter, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &color_descriptors[1]),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);

	// Composition descriptor set
	alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.composition, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.composition));

	color_descriptors = {
	    vkb::initializers::descriptor_image_info(offscreen.sampler, offscreen.color[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	    vkb::initializers::descriptor_image_info(offscreen.sampler, filter_pass.color[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	};

	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(descriptor_sets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &color_descriptors[0]),
	    vkb::initializers::write_descriptor_set(descriptor_sets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &color_descriptors[1]),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
}

void DebugUtils::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_BACK_BIT,
	        VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_FALSE,
	        VK_FALSE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(
	        pipeline_layouts.models,
	        render_pass,
	        0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	};

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;

	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	// Full screen pipelines

	// Empty vertex input state, full screen triangles are generated by the vertex shader
	VkPipelineVertexInputStateCreateInfo empty_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	pipeline_create_info.pVertexInputState                 = &empty_input_state;

	// Final fullscreen composition pass pipeline
	shader_stages[0]                  = debug_load_shader("debug_utils/composition.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                  = debug_load_shader("debug_utils/composition.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	pipeline_create_info.layout       = pipeline_layouts.composition;
	pipeline_create_info.renderPass   = render_pass;
	rasterization_state.cullMode      = VK_CULL_MODE_FRONT_BIT;
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments    = blend_attachment_states.data();
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.composition));

	// Bloom pass
	shader_stages[0]                           = debug_load_shader("debug_utils/bloom.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1]                           = debug_load_shader("debug_utils/bloom.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	color_blend_state.pAttachments             = &blend_attachment_state;
	blend_attachment_state.colorWriteMask      = 0xF;
	blend_attachment_state.blendEnable         = VK_TRUE;
	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

	// Set constant parameters via specialization constants
	VkSpecializationInfo                    specialization_info;
	std::array<VkSpecializationMapEntry, 1> specialization_map_entries;
	specialization_map_entries[0]        = vkb::initializers::specialization_map_entry(0, 0, sizeof(uint32_t));
	uint32_t dir                         = 1;
	specialization_info                  = vkb::initializers::specialization_info(1, specialization_map_entries.data(), sizeof(dir), &dir);
	shader_stages[1].pSpecializationInfo = &specialization_info;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.bloom[0]));

	// Second blur pass (into separate framebuffer)
	pipeline_create_info.renderPass = filter_pass.render_pass;
	dir                             = 0;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.bloom[1]));
	shader_stages[1].pSpecializationInfo = nullptr;

	// Object rendering pipelines
	rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;

	// Vertex bindings an attributes for model rendering
	// Binding description
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                        // Position
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),        // Normal
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6),           // UV
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	pipeline_create_info.pVertexInputState = &vertex_input_state;

	// skysphere pipeline (background cube)
	blend_attachment_state.blendEnable = VK_FALSE;
	pipeline_create_info.layout        = pipeline_layouts.models;
	pipeline_create_info.renderPass    = offscreen.render_pass;
	color_blend_state.attachmentCount  = 2;
	color_blend_state.pAttachments     = blend_attachment_states.data();

	shader_stages[0] = debug_load_shader("debug_utils/gbuffer.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = debug_load_shader("debug_utils/gbuffer.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.skysphere));

	// Enable depth test and write
	depth_stencil_state.depthWriteEnable = VK_TRUE;
	depth_stencil_state.depthTestEnable  = VK_TRUE;
	// Flip cull mode
	rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipelines.sphere));
}

// Prepare and initialize uniform buffer containing shader uniforms
void DebugUtils::prepare_uniform_buffers()
{
	// Matrices vertex shader uniform buffer
	uniform_buffers.matrices = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                               sizeof(ubo_vs),
	                                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                               VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void DebugUtils::update_uniform_buffers()
{
	ubo_vs.projection          = camera.matrices.perspective;
	ubo_vs.modelview           = camera.matrices.view * glm::mat4(1.0f);
	ubo_vs.skysphere_modelview = camera.matrices.view;
	uniform_buffers.matrices->convert_and_update(ubo_vs);
}

void DebugUtils::draw()
{
	queue_begin_label(queue, std::string("Graphics queue command buffer " + std::to_string(current_buffer) + " submission").c_str(), {1.0f, 1.0f, 1.0f, 1.0f});
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
	queue_end_label(queue);
}

bool DebugUtils::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -6.0f));
	camera.set_rotation(glm::vec3(0.0f, 180.0f, 0.0f));

	// Note: Using Revsered depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, (float) width / (float) height, 256.0f, 0.1f);

	debug_check_extension();
	load_assets();
	prepare_uniform_buffers();
	prepare_offscreen_buffer();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_sets();
	build_command_buffers();
	debug_name_objects();
	prepared = true;
	return true;
}

void DebugUtils::render(float delta_time)
{
	if (!prepared)
		return;
	draw();
	if (camera.updated)
		update_uniform_buffers();
}

void DebugUtils::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (debug_utils_supported)
	{
		drawer.text("Debug utilities enabled");
	}
	else
	{
		drawer.text("Warning: Debug utilities extension not available");
		drawer.text("Possible reasons:");
		drawer.text("- Driver does not support the extension");
		drawer.text("- Compiling in release mode with no validation layers enabled");
	}
	if (drawer.header("Settings"))
	{
		if (drawer.checkbox("Bloom", &bloom))
		{
			build_command_buffers();
		}
		if (drawer.checkbox("skysphere", &display_skysphere))
		{
			build_command_buffers();
		}
	}
}

void DebugUtils::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
}

std::unique_ptr<vkb::Application> create_debug_utils()
{
	return std::make_unique<DebugUtils>();
}
