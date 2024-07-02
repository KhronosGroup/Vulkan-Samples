/* Copyright (c) 2024, Sascha Willems
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

#include "dynamic_rendering_local_read.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/sub_mesh.h"

DynamicRenderingLocalRead::DynamicRenderingLocalRead()
{
	title = "Dynamic Rendering local read";

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_position({0.0f, 0.5f, 0.0f});
	camera.set_position({-3.2f, 1.0f, 5.9f});
	camera.set_rotation({0.5f, 210.05f, 0.0f});
	camera.set_perspective(60.f, static_cast<float>(width) / static_cast<float>(height), 256.f, 0.1f);

#if defined(USE_DYNAMIC_RENDERING)
	set_api_version(VK_API_VERSION_1_2);
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	add_device_extension(VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME);
	// To simplify barrier setup used for dynamic rendering, we use sync2
	add_device_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

	// Dynamic rendering doesn't use render passes
	// To make sure that framework related classes like the user interface are aware of this, we explicitly st the base class' renderpass to a null handle
	render_pass = VK_NULL_HANDLE;

	LOGI("Using dynamic rendering with local read");
#endif
}

DynamicRenderingLocalRead::~DynamicRenderingLocalRead()
{
	if (has_device())
	{
		for (Pass pass : {scene_opaque_pass, scene_transparent_pass, composition_pass})
		{
			vkDestroyPipeline(get_device().get_handle(), pass.pipeline, nullptr);
			vkDestroyPipelineLayout(get_device().get_handle(), pass.pipeline_layout, nullptr);
			vkDestroyDescriptorSetLayout(get_device().get_handle(), pass.descriptor_set_layout, nullptr);
		}
		for (FrameBufferAttachment attachment : {attachments.albedo, attachments.normal, attachments.positionDepth})
		{
			destroy_attachment(attachment);
		}
		vkDestroySampler(get_device().get_handle(), textures.transparent_glass.sampler, nullptr);
	}
}

void DynamicRenderingLocalRead::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
#if defined(USE_DYNAMIC_RENDERING)
	auto &requested_dynamic_rendering_features            = gpu.request_extension_features<VkPhysicalDeviceDynamicRenderingFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR);
	requested_dynamic_rendering_features.dynamicRendering = VK_TRUE;

	auto &requested_dynamic_rendering_local_read_features                     = gpu.request_extension_features<VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES_KHR);
	requested_dynamic_rendering_local_read_features.dynamicRenderingLocalRead = VK_TRUE;

	// To simplify barrier setup used for dynamic rendering, we use sync2
	auto &requested_synchronisation2_features            = gpu.request_extension_features<VkPhysicalDeviceSynchronization2FeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR);
	requested_synchronisation2_features.synchronization2 = VK_TRUE;
#endif
}

void DynamicRenderingLocalRead::setup_framebuffer()
{
	if (attachment_width != width || attachment_height != height)
	{
		attachment_width  = width;
		attachment_height = height;
		create_attachments();

#if defined(USE_DYNAMIC_RENDERING)
		// Dynamic rendering uses a new layout to make writes to attachments visible for reads via input attachments
		const VkImageLayout image_layout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR;
#else
		const VkImageLayout image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#endif

		// Update descriptors (e.g. on resize)
		// The attachments will be used as input attachments for some of the passes in this sample
		std::vector<VkDescriptorImageInfo> descriptor_image_infos = {
		    vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, attachments.positionDepth.view, image_layout),
		    vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, attachments.normal.view, image_layout),
		    vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, attachments.albedo.view, image_layout),
		};
		std::vector<VkWriteDescriptorSet> write_descriptor_sets;

		for (size_t i = 0; i < descriptor_image_infos.size(); i++)
		{
			write_descriptor_sets.push_back(vkb::initializers::write_descriptor_set(composition_pass.descriptor_set, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, i, &descriptor_image_infos[i]));
		}
		write_descriptor_sets.push_back(vkb::initializers::write_descriptor_set(scene_transparent_pass.descriptor_set, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, &descriptor_image_infos[0]));
		vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
	}

#if !defined(USE_DYNAMIC_RENDERING)
	VkImageView attachment_views[5]{};

	VkFramebufferCreateInfo framebuffer_ci{};
	framebuffer_ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_ci.renderPass      = render_pass;
	framebuffer_ci.attachmentCount = 5;
	framebuffer_ci.pAttachments    = attachment_views;
	framebuffer_ci.width           = width;
	framebuffer_ci.height          = height;
	framebuffer_ci.layers          = 1;

	// Create frame buffers for every swap chain image
	framebuffers.resize(get_render_context().get_render_frames().size());
	for (uint32_t i = 0; i < framebuffers.size(); i++)
	{
		attachment_views[0] = swapchain_buffers[i].view;
		attachment_views[1] = attachments.positionDepth.view;
		attachment_views[2] = attachments.normal.view;
		attachment_views[3] = attachments.albedo.view;
		attachment_views[4] = depth_stencil.view;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_ci, nullptr, &framebuffers[i]));
	}
#endif
}

void DynamicRenderingLocalRead::setup_render_pass()
{
	attachment_width  = width;
	attachment_height = height;

	create_attachments();

#if !defined(USE_DYNAMIC_RENDERING)

	// We only need a render pass if we don't use dynamic rendering

	std::array<VkAttachmentDescription, 5> attachments{};
	// Color attachment
	attachments[0].format         = get_render_context().get_swapchain().get_format();
	attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Deferred attachments
	// Position
	attachments[1].format         = this->attachments.positionDepth.format;
	attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Normals
	attachments[2].format         = this->attachments.normal.format;
	attachments[2].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[2].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[2].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Albedo
	attachments[3].format         = this->attachments.albedo.format;
	attachments[3].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[3].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[3].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[3].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[3].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[3].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Depth attachment
	attachments[4].format         = depth_format;
	attachments[4].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[4].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[4].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[4].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[4].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[4].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Three subpasses
	std::array<VkSubpassDescription, 3> subpass_descriptions{};

	// First subpass: Fill G-Buffer components
	// ----------------------------------------------------------------------------------------

	VkAttachmentReference color_references[4] = {
	    {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
	    {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
	    {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
	    {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

	VkAttachmentReference depth_reference =
	    {4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

	subpass_descriptions[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_descriptions[0].colorAttachmentCount    = 4;
	subpass_descriptions[0].pColorAttachments       = color_references;
	subpass_descriptions[0].pDepthStencilAttachment = &depth_reference;

	// Second subpass: Final composition (using G-Buffer components)
	// ----------------------------------------------------------------------------------------

	VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	VkAttachmentReference input_references[3];
	input_references[0] = {1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	input_references[1] = {2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	input_references[2] = {3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

	subpass_descriptions[1].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_descriptions[1].colorAttachmentCount    = 1;
	subpass_descriptions[1].pColorAttachments       = &colorReference;
	subpass_descriptions[1].pDepthStencilAttachment = &depth_reference;
	// Use the color attachments filled in the first pass as input attachments
	subpass_descriptions[1].inputAttachmentCount = 3;
	subpass_descriptions[1].pInputAttachments    = input_references;

	// Third subpass: Forward transparency
	// ----------------------------------------------------------------------------------------
	colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	input_references[0] = {1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

	subpass_descriptions[2].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_descriptions[2].colorAttachmentCount    = 1;
	subpass_descriptions[2].pColorAttachments       = &colorReference;
	subpass_descriptions[2].pDepthStencilAttachment = &depth_reference;
	// Use the color/depth attachments filled in the first pass as input attachments
	subpass_descriptions[2].inputAttachmentCount = 1;
	subpass_descriptions[2].pInputAttachments    = input_references;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 5> dependencies;

	// This makes sure that writes to the depth image are done before we try to write to it again
	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask   = 0;
	dependencies[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = 0;

	dependencies[1].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].dstSubpass      = 0;
	dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask   = 0;
	dependencies[1].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dependencyFlags = 0;

	// This dependency transitions the input attachment from color attachment to input attachment read
	dependencies[2].srcSubpass      = 0;
	dependencies[2].dstSubpass      = 1;
	dependencies[2].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[2].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[2].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[2].dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[3].srcSubpass      = 1;
	dependencies[3].dstSubpass      = 2;
	dependencies[3].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[3].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[3].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[3].dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[4].srcSubpass      = 2;
	dependencies[4].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[4].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[4].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[4].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[4].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo render_pass_ci = {};
	render_pass_ci.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_ci.attachmentCount        = static_cast<uint32_t>(attachments.size());
	render_pass_ci.pAttachments           = attachments.data();
	render_pass_ci.subpassCount           = static_cast<uint32_t>(subpass_descriptions.size());
	render_pass_ci.pSubpasses             = subpass_descriptions.data();
	render_pass_ci.dependencyCount        = static_cast<uint32_t>(dependencies.size());
	render_pass_ci.pDependencies          = dependencies.data();

	VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_ci, nullptr, &render_pass));
#endif
}

void DynamicRenderingLocalRead::prepare_gui()
{
#if !defined(USE_DYNAMIC_RENDERING)
	create_gui(*window, nullptr, 15.0f, true);
	get_gui().set_subpass(2);
	get_gui().prepare(pipeline_cache, render_pass,
	                  {load_shader("uioverlay/uioverlay.vert", VK_SHADER_STAGE_VERTEX_BIT),
	                   load_shader("uioverlay/uioverlay.frag", VK_SHADER_STAGE_FRAGMENT_BIT)});
#endif
}

void DynamicRenderingLocalRead::load_assets()
{
	vkb::GLTFLoader loader{get_device()};
	scenes.opaque      = loader.read_scene_from_file("scenes/subpass_scene_opaque.gltf");
	scenes.transparent = loader.read_scene_from_file("scenes/subpass_scene_transparent.gltf");

	textures.transparent_glass = load_texture("textures/transparent_glass_rgba.ktx", vkb::sg::Image::Color);
}

void DynamicRenderingLocalRead::create_attachment(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment &attachment)
{
	if (attachment.image != VK_NULL_HANDLE)
	{
		destroy_attachment(attachment);
	}

	VkImageAspectFlags aspect_mask{0};

	attachment.format = format;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	assert(aspect_mask > 0);

	VkImageCreateInfo image_ci = vkb::initializers::image_create_info();
	image_ci.imageType         = VK_IMAGE_TYPE_2D;
	image_ci.format            = format;
	image_ci.extent.width      = attachment_width;
	image_ci.extent.height     = attachment_height;
	image_ci.extent.depth      = 1;
	image_ci.mipLevels         = 1;
	image_ci.arrayLayers       = 1;
	image_ci.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_ci.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_ci.usage             = usage | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	image_ci.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

	VkMemoryAllocateInfo memory_ai = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements{};

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image_ci, nullptr, &attachment.image));
	vkGetImageMemoryRequirements(get_device().get_handle(), attachment.image, &memory_requirements);
	memory_ai.allocationSize  = memory_requirements.size;
	memory_ai.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_ai, nullptr, &attachment.memory));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), attachment.image, attachment.memory, 0));

	VkImageViewCreateInfo image_view_ci           = vkb::initializers::image_view_create_info();
	image_view_ci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view_ci.format                          = format;
	image_view_ci.subresourceRange                = {};
	image_view_ci.subresourceRange.aspectMask     = aspect_mask;
	image_view_ci.subresourceRange.baseMipLevel   = 0;
	image_view_ci.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
	image_view_ci.subresourceRange.baseArrayLayer = 0;
	image_view_ci.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
	image_view_ci.image                           = attachment.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view_ci, nullptr, &attachment.view));

#if defined(USE_DYNAMIC_RENDERING)
	// Without render passes and their implicit layout transitions, we need to explicitly transition the attachments
	// We use a new layout introduced by this extension that makes writes to images visible via input attachments
	VkCommandBuffer command_buffer = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkImageMemoryBarrier2KHR imageMemoryBarrier{};
	imageMemoryBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
	imageMemoryBarrier.srcStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
	imageMemoryBarrier.dstStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
	imageMemoryBarrier.dstAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
	imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR;
	imageMemoryBarrier.subresourceRange = image_view_ci.subresourceRange;
	imageMemoryBarrier.image            = attachment.image;

	VkDependencyInfoKHR dependencyInfo{};
	dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers    = &imageMemoryBarrier;
	vkCmdPipelineBarrier2KHR(command_buffer, &dependencyInfo);

	get_device().flush_command_buffer(command_buffer, queue);
#endif
}

void DynamicRenderingLocalRead::destroy_attachment(FrameBufferAttachment &attachment)
{
	vkDestroyImageView(get_device().get_handle(), attachment.view, nullptr);
	vkDestroyImage(get_device().get_handle(), attachment.image, nullptr);
	vkFreeMemory(get_device().get_handle(), attachment.memory, nullptr);
	attachment = {};
}

void DynamicRenderingLocalRead::create_attachments()
{
	// The deferred setup used in this sample stores positions, normals and albedo into separate attachments
	// In a real-world application one would try to pack as much information as possible into as small targets as possible to e.g. save bandwidth
	create_attachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, attachments.positionDepth);
	create_attachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, attachments.normal);
	create_attachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, attachments.albedo);
}

void DynamicRenderingLocalRead::prepare_buffers()
{
	buffers.ubo_vs      = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(shader_data_vs), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	buffers.ssbo_lights = std::make_unique<vkb::core::Buffer>(get_device(), lights.size() * sizeof(Light), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffer();
	update_lights_buffer();
}

void DynamicRenderingLocalRead::update_lights_buffer()
{
	std::random_device                    rnd_device;
	std::default_random_engine            rnd_gen(rnd_device());
	std::uniform_real_distribution<float> rnd_dist(-1.0f, 1.0f);
	std::uniform_real_distribution<float> rnd_col(0.0f, 0.5f);

	glm::vec3 light_range{8.0f, 0.6f, 8.0f};

	for (auto &light : lights)
	{
		light.position = glm::vec4(rnd_dist(rnd_gen) * light_range.x, 1.0f + std::abs(rnd_dist(rnd_gen)) * light_range.y, rnd_dist(rnd_gen) * light_range.z, 1.0f);
		light.radius   = 1.0f + std::abs(rnd_dist(rnd_gen)) * 3.0f;
		light.color    = glm::vec3(rnd_col(rnd_gen), rnd_col(rnd_gen), rnd_col(rnd_gen)) * 2.0f;
	}

	buffers.ssbo_lights->convert_and_update(lights);
}

void DynamicRenderingLocalRead::update_uniform_buffer()
{
	shader_data_vs.projection = camera.matrices.perspective;
	shader_data_vs.view       = camera.matrices.view;
	shader_data_vs.model      = glm::mat4(1.f);
	buffers.ubo_vs->convert_and_update(shader_data_vs);
}

void DynamicRenderingLocalRead::prepare_layouts_and_descriptors()
{
	// Set layouts
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings{};
	VkDescriptorSetLayoutCreateInfo           descriptor_layout_create_info{};

	// Offscreen opaque scene rendering
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	};

	descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &scene_opaque_pass.descriptor_set_layout));

	// Transparent scene rendering (forward pass)
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
	};

	descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &scene_transparent_pass.descriptor_set_layout));

	// Composition pass
	set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
	};

	descriptor_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &composition_pass.descriptor_set_layout));

	// Pool

	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 4),
	};
	uint32_t                   num_descriptor_sets = 4;
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), num_descriptor_sets);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));

	// Descriptors

	std::vector<VkWriteDescriptorSet> write_descriptor_sets{};
	VkDescriptorSetAllocateInfo       allocInfo{};

#if defined(USE_DYNAMIC_RENDERING)
	// Dynamic rendering uses a new layout to make writes to attachments visible for reads via input attachments
	const VkImageLayout image_layout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR;
#else
	const VkImageLayout image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#endif

	VkDescriptorImageInfo tex_descriptor_position = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, attachments.positionDepth.view, image_layout);
	VkDescriptorImageInfo tex_descriptor_normal   = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, attachments.normal.view, image_layout);
	VkDescriptorImageInfo tex_descriptor_albedo   = vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, attachments.albedo.view, image_layout);

	VkDescriptorBufferInfo ubo_vs_descriptor      = create_descriptor(*buffers.ubo_vs);
	VkDescriptorBufferInfo ssbo_lights_descriptor = create_descriptor(*buffers.ssbo_lights);

	VkDescriptorImageInfo glass_image_descriptor = create_descriptor(textures.transparent_glass);

	// Opaque scene parts
	allocInfo = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &scene_opaque_pass.descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &allocInfo, &scene_opaque_pass.descriptor_set));
	write_descriptor_sets = {
	    // Binding 0: Vertex shader uniform buffer
	    vkb::initializers::write_descriptor_set(scene_opaque_pass.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &ubo_vs_descriptor)};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	// Transparent scene parts
	allocInfo = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &scene_transparent_pass.descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &allocInfo, &scene_transparent_pass.descriptor_set));
	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(scene_transparent_pass.descriptor_set, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, &tex_descriptor_position),
	    vkb::initializers::write_descriptor_set(scene_transparent_pass.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &ubo_vs_descriptor),
	    vkb::initializers::write_descriptor_set(scene_transparent_pass.descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &glass_image_descriptor),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	// Composition pass
	allocInfo = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &composition_pass.descriptor_set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &allocInfo, &composition_pass.descriptor_set));
	write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(composition_pass.descriptor_set, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, &tex_descriptor_position),
	    vkb::initializers::write_descriptor_set(composition_pass.descriptor_set, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &tex_descriptor_normal),
	    vkb::initializers::write_descriptor_set(composition_pass.descriptor_set, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, &tex_descriptor_albedo),
	    vkb::initializers::write_descriptor_set(composition_pass.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &ssbo_lights_descriptor),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void DynamicRenderingLocalRead::prepare_pipelines()
{
	// Layouts
	VkPipelineLayoutCreateInfo pipeline_layout_create_info{};

	// We use push constants to pass per-scene node information (material, local matrix)
	VkPushConstantRange push_constant_range{};
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_range.size       = sizeof(PushConstantSceneNode);

	// Opaque scene rendering
	pipeline_layout_create_info                        = vkb::initializers::pipeline_layout_create_info(&scene_opaque_pass.descriptor_set_layout, 1);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &scene_opaque_pass.pipeline_layout));

	// Transparent scene rendering
	pipeline_layout_create_info                        = vkb::initializers::pipeline_layout_create_info(&scene_transparent_pass.descriptor_set_layout, 1);
	pipeline_layout_create_info.pushConstantRangeCount = 1;
	pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &scene_transparent_pass.pipeline_layout));

	// Composition pass
	pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&composition_pass.descriptor_set_layout, 1);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &composition_pass.pipeline_layout));

	// Pipelines
	VkPipelineInputAssemblyStateCreateInfo         input_assembly_state   = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo         raster_state           = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	VkPipelineColorBlendAttachmentState            blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo            blend_state            = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);
	VkPipelineViewportStateCreateInfo              viewport_state         = vkb::initializers::pipeline_viewport_state_create_info(1, 1);
	VkPipelineDepthStencilStateCreateInfo          depth_stencil_state    = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);
	VkPipelineMultisampleStateCreateInfo           multisample_state      = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);
	std::array<VkDynamicState, 2>                  dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo               dynamic_state = vkb::initializers::pipeline_dynamic_state_create_info(dynamic_states.data(), vkb::to_u32(dynamic_states.size()));
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	// Vertex bindings an attributes for model rendering
	// This sample uses separate vertex buffers as stored in a glTF scene
	// Position, Normal, UV

	// Binding description
	std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(2, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX),
	};

	// Attribute descriptions
	std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
	    vkb::initializers::vertex_input_attribute_description(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),
	    vkb::initializers::vertex_input_attribute_description(2, 2, VK_FORMAT_R32G32_SFLOAT, 0),
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size()) - 1;
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size()) - 1;
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	// We need to specify the pipeline layout and the render pass description up front as well.
	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info();
	pipeline_create_info.stageCount                   = vkb::to_u32(shader_stages.size());
	pipeline_create_info.pStages                      = shader_stages.data();
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pRasterizationState          = &raster_state;
	pipeline_create_info.pColorBlendState             = &blend_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;

#if defined(USE_DYNAMIC_RENDERING)
	// Dynamic rendering does not use render passes
	pipeline_create_info.renderPass = VK_NULL_HANDLE;

	// Chain in dynamic rendering info structure used to supply dynamic rendering related information for pipeline creation
	VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
	pipeline_create_info.pNext = &pipeline_rendering_create_info;
#else
	pipeline_create_info.renderPass = render_pass;
#endif

	/*
	    Pipeline for the opaque parts of the scene
	*/

	std::array<VkPipelineColorBlendAttachmentState, 4> blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE)};

	blend_state.attachmentCount = 4;
	blend_state.pAttachments    = blend_attachment_states.data();

	pipeline_create_info.layout = scene_opaque_pass.pipeline_layout;

#if defined(USE_DYNAMIC_RENDERING)
	// For dynamic rendering, additional information muss be set at pipeline creation
	VkFormat color_attachment_formats[4] = {
	    get_render_context().get_format(),
	    attachments.positionDepth.format,
	    attachments.normal.format,
	    attachments.albedo.format};

	pipeline_rendering_create_info.colorAttachmentCount    = 4;
	pipeline_rendering_create_info.pColorAttachmentFormats = color_attachment_formats;
	pipeline_rendering_create_info.depthAttachmentFormat   = depth_format;
	if (!vkb::is_depth_only_format(depth_format))
	{
		pipeline_rendering_create_info.stencilAttachmentFormat = depth_format;
	}
#else
	pipeline_create_info.subpass = 0;
#endif

	shader_stages[0] = load_shader("dynamic_rendering_local_read/scene_opaque.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_rendering_local_read/scene_opaque.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &scene_opaque_pass.pipeline));

	/*
	    Pipeline for the transparent parts of the scene
	*/
	for (auto &blend_attachment_state : blend_attachment_states)
	{
		blend_attachment_state.blendEnable         = VK_TRUE;
		blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
		blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
		blend_attachment_state.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	pipeline_create_info.layout = scene_transparent_pass.pipeline_layout;

	vertex_input_state.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());

	raster_state.cullMode = VK_CULL_MODE_NONE;

#if defined(USE_DYNAMIC_RENDERING)
	// For dynamic rendering, additional information must be set at pipeline creation
	pipeline_rendering_create_info.pColorAttachmentFormats = color_attachment_formats;
	pipeline_rendering_create_info.depthAttachmentFormat   = depth_format;
	if (!vkb::is_depth_only_format(depth_format))
	{
		pipeline_rendering_create_info.stencilAttachmentFormat = depth_format;
	}
#else
	blend_state.attachmentCount = 1;
	pipeline_create_info.subpass = 2;
#endif

	shader_stages[0] = load_shader("dynamic_rendering_local_read/scene_transparent.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_rendering_local_read/scene_transparent.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &scene_transparent_pass.pipeline));

	/*
	    Pipeline for the final scene composition
	*/

#if defined(USE_DYNAMIC_RENDERING)
	// For dynamic rendering, additional information muss be set at pipeline creation
	pipeline_rendering_create_info.colorAttachmentCount    = 4;
	pipeline_rendering_create_info.pColorAttachmentFormats = color_attachment_formats;
	pipeline_rendering_create_info.depthAttachmentFormat   = depth_format;
	if (!vkb::is_depth_only_format(depth_format))
	{
		pipeline_rendering_create_info.stencilAttachmentFormat = depth_format;
	}
#else
	blend_state.attachmentCount = 1;
	pipeline_create_info.subpass = 1;
#endif

	blend_attachment_states = {
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE),
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE)};

	pipeline_create_info.layout = composition_pass.pipeline_layout;

	depth_stencil_state.depthWriteEnable = VK_FALSE;
	depth_stencil_state.depthTestEnable  = VK_FALSE;

	raster_state.cullMode = VK_CULL_MODE_NONE;

	// This pass does render a full-screen triangle with vertices generated in the vertex shader, so no vertex input state is required
	VkPipelineVertexInputStateCreateInfo empty_vertex_input_state{};
	empty_vertex_input_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipeline_create_info.pVertexInputState = &empty_vertex_input_state;

	shader_stages[0] = load_shader("dynamic_rendering_local_read/composition.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("dynamic_rendering_local_read/composition.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &composition_pass.pipeline));
}

void DynamicRenderingLocalRead::draw_scene(std::unique_ptr<vkb::sg::Scene> &scene, VkCommandBuffer cmd, VkPipelineLayout pipeline_layout)
{
	for (auto &mesh : scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &node : mesh->get_nodes())
		{
			for (auto &sub_mesh : mesh->get_submeshes())
			{
				const auto &vertex_buffer_position = sub_mesh->vertex_buffers.at("position");
				const auto &vertex_buffer_normal   = sub_mesh->vertex_buffers.at("normal");
				auto       &index_buffer           = sub_mesh->index_buffer;
				auto        mesh_material          = dynamic_cast<const vkb::sg::PBRMaterial *>(sub_mesh->get_material());

				PushConstantSceneNode push_constant_scene_node{};
				push_constant_scene_node.matrix = node->get_transform().get_world_matrix();
				push_constant_scene_node.color  = mesh_material->base_color_factor;
				vkCmdPushConstants(cmd, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantSceneNode), &push_constant_scene_node);

				VkDeviceSize offsets[1] = {0};
				vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffer_position.get(), offsets);
				vkCmdBindVertexBuffers(cmd, 1, 1, vertex_buffer_normal.get(), offsets);

				bool has_uv = sub_mesh->vertex_buffers.find("texcoord_0") != sub_mesh->vertex_buffers.end();
				if (has_uv)
				{
					const auto &vertex_buffer_uv = sub_mesh->vertex_buffers.at("texcoord_0");
					vkCmdBindVertexBuffers(cmd, 2, 1, vertex_buffer_uv.get(), offsets);
				}
				vkCmdBindIndexBuffer(cmd, index_buffer->get_handle(), 0, sub_mesh->index_type);

				vkCmdDrawIndexed(cmd, sub_mesh->vertex_indices, 1, 0, 0, 0);
			}
		}
	}
}

void DynamicRenderingLocalRead::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[5]{};
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[2].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[3].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[4].depthStencil = {0.0f, 0};

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		auto cmd = draw_cmd_buffers[i];

		vkBeginCommandBuffer(cmd, &command_buffer_begin_info);

#if defined(USE_DYNAMIC_RENDERING)
		// With dynamic rendering and local read there are no render passes

		const std::vector<FrameBufferAttachment> attachment_list = {attachments.positionDepth, attachments.normal, attachments.albedo};

		VkImageSubresourceRange subresource_range_color{};
		subresource_range_color.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource_range_color.levelCount = VK_REMAINING_MIP_LEVELS;
		subresource_range_color.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkImageSubresourceRange subresource_range_depth{};
		subresource_range_depth.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		subresource_range_depth.levelCount = VK_REMAINING_MIP_LEVELS;
		subresource_range_depth.layerCount = VK_REMAINING_ARRAY_LAYERS;

		vkb::image_layout_transition(cmd, swapchain_buffers[i].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, subresource_range_color);
		vkb::image_layout_transition(cmd, depth_stencil.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, subresource_range_depth);

		VkRenderingAttachmentInfoKHR color_attachment_info[4]{};
		for (auto j = 0; j < 4; j++)
		{
			color_attachment_info[j]             = vkb::initializers::rendering_attachment_info();
			color_attachment_info[j].imageLayout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR;
			color_attachment_info[j].resolveMode = VK_RESOLVE_MODE_NONE;
			color_attachment_info[j].loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment_info[j].storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment_info[j].clearValue  = clear_values[j];
		}

		color_attachment_info[0].imageView = swapchain_buffers[i].view;
		for (auto i = 0; i < 3; i++)
		{
			color_attachment_info[i + 1].imageView = attachment_list[i].view;
		}

		VkRenderingAttachmentInfoKHR depth_attachment_info = vkb::initializers::rendering_attachment_info();
		depth_attachment_info.imageView                    = depth_stencil.view;
		depth_attachment_info.imageLayout                  = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depth_attachment_info.resolveMode                  = VK_RESOLVE_MODE_NONE;
		depth_attachment_info.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment_info.storeOp                      = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment_info.clearValue                   = clear_values[1];

		VkRenderingInfoKHR render_info   = vkb::initializers::rendering_info();
		render_info.renderArea           = {0, 0, width, height};
		render_info.layerCount           = 1;
		render_info.colorAttachmentCount = 4;
		render_info.pColorAttachments    = &color_attachment_info[0];
		render_info.renderArea           = {0, 0, width, height};

		render_info.pDepthAttachment = &depth_attachment_info;
		if (!vkb::is_depth_only_format(depth_format))
		{
			render_info.pStencilAttachment = &depth_attachment_info;
		}

		/*
		    Dynamic rendering start
		*/
		vkCmdBeginRenderingKHR(cmd, &render_info);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		/*
		    First draw
		    Fills the G-Buffer attachments containing image data for the deferred composition (color+depth, normals, albedo)
		*/

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_opaque_pass.pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_opaque_pass.pipeline_layout, 0, 1, &scene_opaque_pass.descriptor_set, 0, nullptr);
		draw_scene(scenes.opaque, cmd, scene_opaque_pass.pipeline_layout);

		// We want to read the input attachments in the next pass, with dynamic rendering local read this requires use of a barrier with the "by region" flag set

		// A new feature of the dynamic rendering local read extension is the ability to use pipeline barriers in the dynamic render pass
		// to allow framebuffer-local dependencies (i.e. read-after-write) between draw calls using the "by region" flag
		// So with this barrier we can use the output attachments from the draw call above as input attachments in the next call
		VkMemoryBarrier2KHR memoryBarrier{};
		memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR;
		memoryBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		memoryBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		memoryBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT;

		VkDependencyInfoKHR dependencyInfo{};
		dependencyInfo.sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
		dependencyInfo.dependencyFlags    = VK_DEPENDENCY_BY_REGION_BIT;
		dependencyInfo.memoryBarrierCount = 1;
		dependencyInfo.pMemoryBarriers    = &memoryBarrier;

		vkCmdPipelineBarrier2KHR(cmd, &dependencyInfo);

		/*
		    Second draw
		    This will use the G-Buffer attachments that have been filled in the first draw as input attachment for the deferred scene composition
		*/
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, composition_pass.pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, composition_pass.pipeline_layout, 0, 1, &composition_pass.descriptor_set, 0, nullptr);
		vkCmdDraw(cmd, 3, 1, 0, 0);

		// Third draw
		// Render transparent geometry using a forward pass that compares against depth generated during the first draw
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_transparent_pass.pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_transparent_pass.pipeline_layout, 0, 1, &scene_transparent_pass.descriptor_set, 0, nullptr);
		draw_scene(scenes.transparent, cmd, scene_transparent_pass.pipeline_layout);

		// @todo: UI is disabled for now, required some fixup in the framework to make it work properly with dynamic rendering local reads
		// draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderingKHR(cmd);

		/*
		    Dynamic rendering end
		*/

		vkb::image_layout_transition(cmd, swapchain_buffers[i].image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresource_range_color);
#else
		VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
		render_pass_begin_info.renderPass = render_pass;
		render_pass_begin_info.renderArea.offset.x = 0;
		render_pass_begin_info.renderArea.offset.y = 0;
		render_pass_begin_info.renderArea.extent.width = width;
		render_pass_begin_info.renderArea.extent.height = height;
		render_pass_begin_info.clearValueCount = 5;
		render_pass_begin_info.pClearValues = clear_values;
		render_pass_begin_info.framebuffer = framebuffers[i];

		// Start our render pass, which contains multiple sub passes
		vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// First sub pass
		// Renders the components of the scene to the G-Buffer attachments
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_opaque_pass.pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_opaque_pass.pipeline_layout, 0, 1, &scene_opaque_pass.descriptor_set, 0, nullptr);
		draw_scene(scenes.opaque, cmd, scene_opaque_pass.pipeline_layout);

		// Second sub pass
		// This subpass will use the G-Buffer components that have been filled in the first subpass as input attachment for the final compositing
		vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, composition_pass.pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, composition_pass.pipeline_layout, 0, 1, &composition_pass.descriptor_set, 0, nullptr);
		vkCmdDraw(cmd, 3, 1, 0, 0);

		// Third subpass
		// Render transparent geometry using a forward pass that compares against depth generated during G-Buffer fill
		vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_transparent_pass.pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scene_transparent_pass.pipeline_layout, 0, 1, &scene_transparent_pass.descriptor_set, 0, nullptr);
		draw_scene(scenes.transparent, cmd, scene_transparent_pass.pipeline_layout);

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(cmd);
#endif

		VK_CHECK(vkEndCommandBuffer(cmd));
	}
}

bool DynamicRenderingLocalRead::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	load_assets();
	prepare_buffers();
	prepare_layouts_and_descriptors();
	prepare_pipelines();
	build_command_buffers();
	prepared = true;
	return true;
}

void DynamicRenderingLocalRead::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
	if (camera.updated)
	{
		update_uniform_buffer();
	}
}

void DynamicRenderingLocalRead::on_update_ui_overlay(vkb::Drawer &drawer)
{
#if defined(USE_DYNAMIC_RENDERING)
	drawer.text("Using dynamic rendering with local read");
#else
	drawer.text("Using renderpass with subpasses");
#endif
	if (drawer.button("Randomize lights"))
	{
		get_device().wait_idle();
		update_lights_buffer();
	}
}

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_dynamic_rendering_local_read()
{
	return std::make_unique<DynamicRenderingLocalRead>();
}
