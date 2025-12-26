/* Copyright (c) 2021-2025, Arm Limited and Contributors
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

#include "async_compute.h"

#include "api_vulkan_sample.h"
#include "common/vk_common.h"
#include "filesystem/legacy.h"
#include "gltf_loader.h"
#include "gui.h"

#include "scene_graph/components/orthographic_camera.h"
#include "stats/stats.h"

AsyncComputeSample::AsyncComputeSample()
{
	auto &config = get_configuration();

	config.insert<vkb::BoolSetting>(0, async_enabled, false);
	config.insert<vkb::BoolSetting>(1, async_enabled, true);
	config.insert<vkb::BoolSetting>(0, rotate_shadows, false);
	config.insert<vkb::BoolSetting>(1, rotate_shadows, true);
	config.insert<vkb::BoolSetting>(0, double_buffer_hdr_frames, false);
	config.insert<vkb::BoolSetting>(1, double_buffer_hdr_frames, true);
}

void AsyncComputeSample::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
#ifdef VKB_ENABLE_PORTABILITY
	// Since sampler_info.compareEnable = VK_TRUE, must enable the mutableComparisonSamplers feature of VK_KHR_portability_subset
	REQUEST_REQUIRED_FEATURE(gpu, VkPhysicalDevicePortabilitySubsetFeaturesKHR, mutableComparisonSamplers);
#endif
}

void AsyncComputeSample::draw_gui()
{
	get_gui().show_options_window(
	    /* body = */ [this]() {
		    ImGui::Checkbox("Enable async queues", &async_enabled);
		    ImGui::Checkbox("Double buffer HDR", &double_buffer_hdr_frames);
		    ImGui::Checkbox("Rotate shadows", &rotate_shadows);
	    },
	    /* lines = */ 3);
}

static VkExtent3D downsample_extent(const VkExtent3D &extent, uint32_t level)
{
	return {
	    std::max(1u, extent.width >> level),
	    std::max(1u, extent.height >> level),
	    std::max(1u, extent.depth >> level)};
}

void AsyncComputeSample::prepare_render_targets()
{
	// To make this sample demanding enough to saturate the tested mobile devices, use 4K.
	// Could base this off the swapchain extent, but comparing cross-device performance
	// could get awkward.
	VkExtent3D size = {3840, 2160, 1};

	// Support double-buffered HDR.
	vkb::core::Image color_targets[2]{
	    {get_device(), size, VK_FORMAT_R16G16B16A16_SFLOAT,
	     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	     VMA_MEMORY_USAGE_GPU_ONLY},
	    {get_device(), size, VK_FORMAT_R16G16B16A16_SFLOAT,
	     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	     VMA_MEMORY_USAGE_GPU_ONLY},
	};
	color_targets[0].set_debug_name("color_targets[0]");
	color_targets[1].set_debug_name("color_targets[1]");

	// Should only really need one depth target, but vkb::RenderTarget needs to own the resource.
	vkb::core::Image depth_targets[2]{
	    {get_device(), size, VK_FORMAT_D32_SFLOAT,
	     VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	     VMA_MEMORY_USAGE_GPU_ONLY},
	    {get_device(), size, VK_FORMAT_D32_SFLOAT,
	     VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	     VMA_MEMORY_USAGE_GPU_ONLY},
	};
	depth_targets[0].set_debug_name("depth_targets[0]");
	depth_targets[1].set_debug_name("depth_targets[1]");

	// 8K shadow-map overkill to stress devices.
	// Min-spec is 4K however, so clamp to that if required.
	VkExtent3D              shadow_resolution{8 * 1024, 8 * 1024, 1};
	VkImageFormatProperties depth_properties{};
	vkGetPhysicalDeviceImageFormatProperties(get_device().get_gpu().get_handle(), VK_FORMAT_D16_UNORM, VK_IMAGE_TYPE_2D,
	                                         VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	                                         0, &depth_properties);
	shadow_resolution.width  = std::min(depth_properties.maxExtent.width, shadow_resolution.width);
	shadow_resolution.height = std::min(depth_properties.maxExtent.height, shadow_resolution.height);
	shadow_resolution.width  = std::min(get_device().get_gpu().get_properties().limits.maxFramebufferWidth, shadow_resolution.width);
	shadow_resolution.height = std::min(get_device().get_gpu().get_properties().limits.maxFramebufferHeight, shadow_resolution.height);

	vkb::core::Image shadow_target{get_device(), shadow_resolution, VK_FORMAT_D16_UNORM,
	                               VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	                               VMA_MEMORY_USAGE_GPU_ONLY};
	shadow_target.set_debug_name("shadow_target");

	// Create a simple mip-chain used for bloom blur.
	// Could technically mip-map the HDR target,
	// but there's no real reason to do it like that.
	for (uint32_t level = 1; level < 7; level++)
	{
		blur_chain.push_back(std::make_unique<vkb::core::Image>(
		    get_device(), downsample_extent(size, level),
		    VK_FORMAT_R16G16B16A16_SFLOAT,
		    VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		    VMA_MEMORY_USAGE_GPU_ONLY));
		blur_chain_views.push_back(std::make_unique<vkb::core::ImageView>(
		    *blur_chain.back(), VK_IMAGE_VIEW_TYPE_2D));
	}

	// Calculate valid filter
	VkFilter filter = VK_FILTER_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), depth_targets[0].get_format(), &filter);

	auto sampler_info         = vkb::initializers::sampler_create_info();
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.minFilter    = filter;
	sampler_info.magFilter    = filter;
	sampler_info.maxLod       = VK_LOD_CLAMP_NONE;

	linear_sampler = std::make_unique<vkb::core::Sampler>(get_device(), sampler_info);

	// Inverse Z, so use GEQ test.
	sampler_info.compareOp     = VK_COMPARE_OP_GREATER_OR_EQUAL;
	sampler_info.compareEnable = VK_TRUE;
	comparison_sampler         = std::make_unique<vkb::core::Sampler>(get_device(), sampler_info);

	for (unsigned i = 0; i < 2; i++)
	{
		std::vector<vkb::core::Image> color_attachments;
		color_attachments.push_back(std::move(color_targets[i]));
		color_attachments.push_back(std::move(depth_targets[i]));
		forward_render_targets[i] = std::make_unique<vkb::RenderTarget>(std::move(color_attachments));
	}

	std::vector<vkb::core::Image> shadow_attachments;
	shadow_attachments.push_back(std::move(shadow_target));
	shadow_render_target = std::make_unique<vkb::RenderTarget>(std::move(shadow_attachments));
}

void AsyncComputeSample::setup_queues()
{
	present_graphics_queue = &get_device().get_queue_by_present(0);
	last_async_enabled     = async_enabled;

	// Need to be careful about sync if we're going to suddenly switch to async compute.
	get_device().wait_idle();

	// The way we set things up here somewhat heavily favors devices where we have 2 or more graphics queues.
	// The pipeline we ideally want is:
	// - Low priority graphics queue renders the HDR frames
	// - Async compute queue does post
	// - High priority queue does (HDR + Bloom) tonemap + UI in one graphics pass and presents.
	//
	// We want to present in the high priority graphics queue since on at least Arm devices,
	// we can get pre-emption behavior
	// where we can start rendering the next frame in parallel with async compute post,
	// but the next frame will not block presentation. This keeps latency low, and
	// is important to achieve full utilization of the fragment queue.
	// Getting the async queue idle as fast as possible unblocks vertex shading work for the next frame.

	// On desktop, in particular on architectures with just one graphics queue, this setup isn't very appealing
	// since we cannot have a low and high priority graphics queue.
	// We would ideally change the entire pipeline to be geared towards presenting in the async compute queue where
	// tonemap + UI happens in compute instead.
	// This complicates things since we would have to render UI in a fragment pass, which compute just composites.
	// The hardcore alternative is to render the UI entirely in compute, but all of these consideration
	// are outside the scope of this sample.

	if (async_enabled)
	{
		const auto &queue_family_properties = get_device().get_gpu().get_queue_family_properties();
		uint32_t    graphics_family_index   = vkb::get_queue_family_index(queue_family_properties, VK_QUEUE_GRAPHICS_BIT);
		uint32_t    compute_family_index    = vkb::get_queue_family_index(queue_family_properties, VK_QUEUE_COMPUTE_BIT);

		if (queue_family_properties[graphics_family_index].queueCount >= 2)
		{
			LOGI("Device has 2 or more graphics queues.");
			early_graphics_queue = &get_device().get_queue(graphics_family_index, 1);
		}
		else
		{
			LOGI("Device has just 1 graphics queue.");
			early_graphics_queue = present_graphics_queue;
		}

		if (graphics_family_index == compute_family_index)
		{
			LOGI("Device does not have a dedicated compute queue family.");
			post_compute_queue = early_graphics_queue;
		}
		else
		{
			LOGI("Device has async compute queue.");
			post_compute_queue = &get_device().get_queue(compute_family_index, 0);
		}
	}
	else
	{
		// Force everything through the same queue.
		early_graphics_queue = present_graphics_queue;
		post_compute_queue   = present_graphics_queue;
	}
}

bool AsyncComputeSample::prepare(const vkb::ApplicationOptions &options)
{
	// Set setup_queues() for details.
	set_high_priority_graphics_queue_enable(true);

	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	load_scene("scenes/bonza/Bonza.gltf");

	auto &camera_node = vkb::add_free_camera(get_scene(), "main_camera", get_render_context().get_surface_extent());
	camera            = &camera_node.get_component<vkb::sg::Camera>();

	// Attach a shadow camera to the directional light.
	auto lights = get_scene().get_components<vkb::sg::Light>();
	for (auto &light : lights)
	{
		if (light->get_light_type() == vkb::sg::LightType::Directional)
		{
			vkb::sg::LightProperties props{};
			props.color = glm::vec3(50.0f, 40.0f, 30.0f);
			light->set_properties(props);
			auto *node = light->get_node();

			// Hardcoded to fit to the scene.
			auto ortho_camera = std::make_unique<vkb::sg::OrthographicCamera>("shadow_camera",
			                                                                  -2000.0f, 3000.0f,
			                                                                  -2500.0f, 1500.0f,
			                                                                  -2000.0f, 2000.0f);

			ortho_camera->set_node(*node);
			get_scene().add_component(std::move(ortho_camera), *node);
			shadow_camera = &node->get_component<vkb::sg::Camera>();
			break;
		}
	}

	prepare_render_targets();

	vkb::ShaderSource vert_shader("async_compute/forward.vert.spv");
	vkb::ShaderSource frag_shader("async_compute/forward.frag.spv");
	auto              scene_subpass =
	    std::make_unique<ShadowMapForwardSubpass>(get_render_context(), std::move(vert_shader), std::move(frag_shader), get_scene(), *camera, *shadow_camera);

	vkb::ShaderSource shadow_vert_shader("async_compute/shadow.vert.spv");
	vkb::ShaderSource shadow_frag_shader("async_compute/shadow.frag.spv");
	auto              shadow_scene_subpass =
	    std::make_unique<DepthMapSubpass>(get_render_context(), std::move(shadow_vert_shader), std::move(shadow_frag_shader), get_scene(), *shadow_camera);
	shadow_render_pipeline.add_subpass(std::move(shadow_scene_subpass));
	shadow_render_pipeline.set_load_store({{VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE}});

	vkb::ShaderSource composite_vert_shader("async_compute/composite.vert.spv");
	vkb::ShaderSource composite_frag_shader("async_compute/composite.frag.spv");
	auto              composite_scene_subpass =
	    std::make_unique<CompositeSubpass>(get_render_context(), std::move(composite_vert_shader), std::move(composite_frag_shader));

	forward_render_pipeline.add_subpass(std::move(scene_subpass));
	forward_render_pipeline.set_load_store({{VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE},
	                                        {VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE}});

	auto blit_render_pipeline = std::make_unique<vkb::RenderPipeline>();
	blit_render_pipeline->add_subpass(std::move(composite_scene_subpass));
	blit_render_pipeline->set_load_store({{VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE},
	                                      {VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE}});

	set_render_pipeline(std::move(blit_render_pipeline));

	vkb::CounterSamplingConfig config;
	config.mode = vkb::CounterSamplingMode::Continuous;
	get_stats().request_stats({
	                              vkb::StatIndex::frame_times,
	                              vkb::StatIndex::gpu_cycles,
	                              vkb::StatIndex::gpu_vertex_cycles,
	                              vkb::StatIndex::gpu_fragment_cycles,
	                          },
	                          config);

	create_gui(*window, &get_stats());

	// Store the start time to calculate rotation
	start_time = std::chrono::system_clock::now();

	auto &threshold_module = get_device().get_resource_cache().request_shader_module(VK_SHADER_STAGE_COMPUTE_BIT,
	                                                                                 vkb::ShaderSource("async_compute/threshold.comp.spv"));
	auto &blur_up_module   = get_device().get_resource_cache().request_shader_module(VK_SHADER_STAGE_COMPUTE_BIT,
	                                                                                 vkb::ShaderSource("async_compute/blur_up.comp.spv"));
	auto &blur_down_module = get_device().get_resource_cache().request_shader_module(VK_SHADER_STAGE_COMPUTE_BIT,
	                                                                                 vkb::ShaderSource("async_compute/blur_down.comp.spv"));
	threshold_pipeline     = &get_device().get_resource_cache().request_pipeline_layout({&threshold_module});
	blur_up_pipeline       = &get_device().get_resource_cache().request_pipeline_layout({&blur_up_module});
	blur_down_pipeline     = &get_device().get_resource_cache().request_pipeline_layout({&blur_down_module});

	setup_queues();

	return true;
}

void AsyncComputeSample::render_shadow_pass()
{
	auto &queue          = *early_graphics_queue;
	auto  command_buffer = get_render_context().get_active_frame().get_command_pool(queue).request_command_buffer();
	command_buffer->set_debug_name("shadow_pass");
	command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	auto &views = shadow_render_target->get_views();
	assert(!views.empty());

	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		command_buffer->image_memory_barrier(views[0], memory_barrier);
	}

	set_viewport_and_scissor(*command_buffer, shadow_render_target->get_extent());
	shadow_render_pipeline.draw(*command_buffer, *shadow_render_target, VK_SUBPASS_CONTENTS_INLINE);
	command_buffer->end_render_pass();

	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		memory_barrier.src_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		command_buffer->image_memory_barrier(views[0], memory_barrier);
	}

	command_buffer->end();

	get_render_context().submit(queue, {command_buffer});
}

vkb::RenderTarget &AsyncComputeSample::get_current_forward_render_target()
{
	return *forward_render_targets[forward_render_target_index];
}

VkSemaphore AsyncComputeSample::render_forward_offscreen_pass(VkSemaphore hdr_wait_semaphore)
{
	auto &queue          = *early_graphics_queue;
	auto  command_buffer = get_render_context().get_active_frame().get_command_pool(queue).request_command_buffer();
	command_buffer->set_debug_name("forward_offscreen_pass");

	command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	auto &views = get_current_forward_render_target().get_views();
	assert(1 < views.size());

	{
		// If maintenance9 is not enabled, resources with VK_SHARING_MODE_EXCLUSIVE must only be accessed by queues in the queue family that has ownership of the resource.
		// Upon creation resources with VK_SHARING_MODE_EXCLUSIVE are not owned by any queue, ownership is implicitly acquired upon first use.
		// The application must perform a queue family ownership transfer if it wishes to make the memory contents of the resource accessible to a different queue family.
		// A queue family can take ownership of a resource without an ownership transfer, in the same way as for a resource that was just created, but the content will be undefined.
		// We do not need to acquire color_targets[0] from present_graphics to early_graphics
		// A queue transfer barrier is not necessary for the resource first access.
		// Moreover, in our sample we do not care about the content at this point so we can skip the queue transfer barrier.
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		command_buffer->image_memory_barrier(views[0], memory_barrier);
	}

	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		command_buffer->image_memory_barrier(views[1], memory_barrier);
	}

	set_viewport_and_scissor(*command_buffer, get_current_forward_render_target().get_extent());
	forward_render_pipeline.draw(*command_buffer, get_current_forward_render_target(), VK_SUBPASS_CONTENTS_INLINE);
	command_buffer->end_render_pass();

	const bool queue_family_transfer = early_graphics_queue->get_family_index() != post_compute_queue->get_family_index();
	{
		// When doing async compute this barrier is used to do a queue family ownership transfer

		// release_barrier_0: Releasing color_targets[0] from early_graphics to post_compute
		//     This release barrier is replicated by the corresponding acquire_barrier_0 in the post_compute queue
		//     The application must ensure the release operation happens before the acquire operation. This sample uses semaphores for that.
		//     The transfer ownership barriers are submitted twice (release and acquire) but they are only executed once.
		vkb::ImageMemoryBarrier memory_barrier{
		    .src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    .dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,        // Ignored for the release barrier.
		                                                                   // Release barriers ignore dst_access_mask unless using VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR
		    .src_access_mask  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		    .dst_access_mask  = 0,                                               // dst_access_mask is ignored for release barriers, without affecting its validity
		    .old_layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,        // We want a layout transition, so the old_layout and new_layout values need to be replicated in the acquire barrier
		    .new_layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    .src_queue_family = queue_family_transfer ?
		                            early_graphics_queue->get_family_index() :
		                            VK_QUEUE_FAMILY_IGNORED,        // Release barriers are executed from a queue of the source queue family
		    .dst_queue_family = queue_family_transfer ? post_compute_queue->get_family_index() : VK_QUEUE_FAMILY_IGNORED,
		};

		command_buffer->image_memory_barrier(views[0], memory_barrier);
	}

	command_buffer->end();

	// Conditionally waits on hdr_wait_semaphore.
	// This resolves the write-after-read hazard where previous frame tonemap read from HDR buffer.

	// We are not using VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR
	// so VK_PIPELINE_STAGE_ALL_COMMANDS_BIT is the only valid stage to wait for queue transfer operations.
	const VkPipelineStageFlags wait_stage = queue_family_transfer ? VK_PIPELINE_STAGE_ALL_COMMANDS_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	auto signal_semaphore = get_render_context().submit(queue, {command_buffer}, hdr_wait_semaphore, wait_stage);

	if (hdr_wait_semaphore)
	{
		get_render_context().release_owned_semaphore(hdr_wait_semaphore);
	}

	return signal_semaphore;
}

VkSemaphore AsyncComputeSample::render_swapchain(VkSemaphore post_semaphore)
{
	auto &queue          = *present_graphics_queue;
	auto  command_buffer = get_render_context().get_active_frame().get_command_pool(queue).request_command_buffer();
	command_buffer->set_debug_name("swapchain");

	command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	if (post_compute_queue->get_family_index() != present_graphics_queue->get_family_index())
	{
		// acquire_barrier_1: Acquiring color_targets[0] from  post_compute to present_graphics
		//     This acquire barrier is replicated by the corresponding release_barrier_1 in the post_compute queue
		//     The application must ensure the acquire operation happens after the release operation. This sample uses semaphores for that.
		//     The transfer ownership barriers are submitted twice (release and acquire) but they are only executed once.
		vkb::ImageMemoryBarrier memory_barrier{
		    .src_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,        // Ignored for the acquire barrier.
		                                                                    // Acquire barriers ignore src_access_mask unless using VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR
		    .dst_stage_mask   = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		    .src_access_mask  = 0,        // src_access_mask is ignored for acquire barriers, without affecting its validity
		    .dst_access_mask  = VK_ACCESS_SHADER_READ_BIT,
		    .old_layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,        // Purely ownership transfer. We do not need a layout transition.
		    .new_layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    .src_queue_family = post_compute_queue->get_family_index(),
		    .dst_queue_family = present_graphics_queue->get_family_index(),        // Acquire barriers are executed from a queue of the destination queue family
		};

		command_buffer->image_memory_barrier(get_current_forward_render_target().get_views()[0], memory_barrier);

		// acquire_barrier_2: Acquiring blur_chain_views[1] from  post_compute to present_graphics
		//     This acquire barrier is replicated by the corresponding release_barrier_2 in the post_compute queue
		//     The application must ensure the acquire operation happens after the release operation. This sample uses semaphores for that.
		//     The transfer ownership barriers are submitted twice (release and acquire) but they are only executed once.
		vkb::ImageMemoryBarrier memory_barrier_2{
		    .src_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,        // Ignored for the acquire barrier.
		                                                                    // Acquire barriers ignore src_access_mask unless using VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR
		    .dst_stage_mask   = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		    .src_access_mask  = 0,        // src_access_mask is ignored for acquire barriers, without affecting its validity
		    .dst_access_mask  = VK_ACCESS_SHADER_READ_BIT,
		    .old_layout       = VK_IMAGE_LAYOUT_GENERAL,        // We want a layout transition, so the old_layout and new_layout values need to be replicated in the acquire barrier
		    .new_layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    .src_queue_family = post_compute_queue->get_family_index(),
		    .dst_queue_family = present_graphics_queue->get_family_index(),        // Acquire barriers are executed from a queue of the destination queue family

		};
		command_buffer->image_memory_barrier(*blur_chain_views[1], memory_barrier_2);
	}

	draw(*command_buffer, get_render_context().get_active_frame().get_render_target());

	// If maintenance9 is not enabled, resources with VK_SHARING_MODE_EXCLUSIVE must only be accessed by queues in the queue family that has ownership of the resource.
	// Upon creation resources with VK_SHARING_MODE_EXCLUSIVE are not owned by any queue, ownership is implicitly acquired upon first use.
	// The application must perform a queue family ownership transfer if it wishes to make the memory contents of the resource accessible to a different queue family.
	// A queue family can take ownership of a resource without an ownership transfer, in the same way as for a resource that was just created, but the content will be undefined.
	// We do not need to release blur_chain_views[1] and color_targets[0] from present_graphics
	// A queue transfer barrier is not necessary for the resource first access.
	// Moreover, in our sample we do not care about the content after presenting so we can skip the queue transfer barrier.

	command_buffer->end();

	// We're going to wait on this semaphore in different frame,
	// so we need to hold ownership of the semaphore until we complete the wait.
	hdr_wait_semaphores[forward_render_target_index] = get_render_context().request_semaphore_with_ownership();

	// We've read the post buffer outputs, so we need to consider write-after-read
	// next frame. This is only meaningful if we're doing double buffered HDR since it's
	// theoretically possible to complete HDR rendering for frame N + 1 while we're doing presentation.
	// In that case, the async compute post pipeline can start writing blur results *before* we're done reading.
	compute_post_semaphore = get_render_context().request_semaphore_with_ownership();

	const VkSemaphore signal_semaphores[] = {
	    get_render_context().request_semaphore(),
	    hdr_wait_semaphores[forward_render_target_index],
	    compute_post_semaphore,
	};

	const VkSemaphore wait_semaphores[] = {
	    post_semaphore,
	    get_render_context().consume_acquired_semaphore(),
	};

	const VkPipelineStageFlags wait_stages[] = {
	    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	};

	auto info                 = vkb::initializers::submit_info();
	info.pSignalSemaphores    = signal_semaphores;
	info.signalSemaphoreCount = 3;
	info.pWaitSemaphores      = wait_semaphores;
	info.waitSemaphoreCount   = 2;
	info.pWaitDstStageMask    = wait_stages;
	info.commandBufferCount   = 1;
	info.pCommandBuffers      = &command_buffer->get_handle();

	queue.submit({info}, get_render_context().get_active_frame().get_fence_pool().request_fence());
	get_render_context().release_owned_semaphore(wait_semaphores[1]);
	return signal_semaphores[0];
}

VkSemaphore AsyncComputeSample::render_compute_post(VkSemaphore wait_graphics_semaphore, VkSemaphore wait_present_semaphore)
{
	auto &queue          = *post_compute_queue;
	auto  command_buffer = get_render_context().get_active_frame().get_command_pool(queue).request_command_buffer();
	command_buffer->set_debug_name("compute_post");

	command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	if (early_graphics_queue->get_family_index() != post_compute_queue->get_family_index())
	{
		// acquire_barrier_0: Acquiring color_targets[0] from early_graphics to post_compute
		//     This acquire barrier is replicated by the corresponding release_barrier_0 in the early_graphics queue
		//     The application must ensure the acquire operation happens after the release operation. This sample uses semaphores for that.
		//     The transfer ownership barriers are submitted twice (release and acquire) but they are only executed once.
		vkb::ImageMemoryBarrier memory_barrier{
		    .src_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,        // Ignored for the acquire barrier.
		                                                                   // Acquire barriers ignore src_access_mask unless using VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR
		    .dst_stage_mask   = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		    .src_access_mask  = 0,        // src_access_mask is ignored for acquire barriers, without affecting its validity
		    .dst_access_mask  = VK_ACCESS_SHADER_READ_BIT,
		    .old_layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,        // We want a layout transition, so the old_layout and new_layout values need to be replicated in the release barrier
		    .new_layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    .src_queue_family = early_graphics_queue->get_family_index(),
		    .dst_queue_family = post_compute_queue->get_family_index(),        // Acquire barriers are executed from a queue of the destination queue family
		};
		command_buffer->image_memory_barrier(get_current_forward_render_target().get_views()[0], memory_barrier);
	}

	const auto discard_blur_view = [&](const vkb::core::ImageView &view) {
		// If maintenance9 is not enabled, resources with VK_SHARING_MODE_EXCLUSIVE must only be accessed by queues in the queue family that has ownership of the resource.
		// Upon creation resources with VK_SHARING_MODE_EXCLUSIVE are not owned by any queue, ownership is implicitly acquired upon first use.
		// The application must perform a queue family ownership transfer if it wishes to make the memory contents of the resource accessible to a different queue family.
		// A queue family can take ownership of a resource without an ownership transfer, in the same way as for a resource that was just created, but the content will be undefined.
		// We do not need to acquire blur_chain_views[1] from present_graphics to post_compute
		// A queue transfer barrier is not necessary for the resource first access.
		// Moreover, in our sample we do not care about the content at this point so we can skip the queue transfer barrier.
		vkb::ImageMemoryBarrier memory_barrier{};

		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_GENERAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_SHADER_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		command_buffer->image_memory_barrier(view, memory_barrier);
	};

	const auto read_only_blur_view = [&](const vkb::core::ImageView &view, bool is_final) {
		const bool queue_family_transfer = is_final && post_compute_queue->get_family_index() != present_graphics_queue->get_family_index();

		// release_barrier_2: Releasing blur_chain_views[1] from  post_compute to present_graphics
		//     This release barrier is replicated by the corresponding acquire_barrier_2 in the present_graphics queue
		//     The application must ensure the release operation happens before the acquire operation. This sample uses semaphores for that.
		//     The transfer ownership barriers are submitted twice (release and acquire) but they are only executed once.
		vkb::ImageMemoryBarrier memory_barrier{
		    .src_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		    .dst_stage_mask = is_final ? VkPipelineStageFlags(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT) : VkPipelineStageFlags(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),        // Ignored for the release barrier.
		                                                                                                                                                                 // Release barriers ignore dst_access_mask unless using VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR
		    .src_access_mask  = VK_ACCESS_SHADER_WRITE_BIT,
		    .dst_access_mask  = is_final ? VkAccessFlags(0) : VkAccessFlags(VK_ACCESS_SHADER_READ_BIT),        // dst_access_mask is ignored for release barriers, without affecting its validity
		    .old_layout       = VK_IMAGE_LAYOUT_GENERAL,                                                       // We want a layout transition, so the old_layout and new_layout values need to be replicated in the acquire barrier
		    .new_layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    .src_queue_family = queue_family_transfer ? post_compute_queue->get_family_index() : VK_QUEUE_FAMILY_IGNORED,            // Release barriers are executed from a queue of the source queue family
		    .dst_queue_family = queue_family_transfer ? present_graphics_queue->get_family_index() : VK_QUEUE_FAMILY_IGNORED,        // Release barriers are executed from a queue of the source queue family
		};

		command_buffer->image_memory_barrier(view, memory_barrier);
	};

	struct Push
	{
		uint32_t width, height;
		float    inv_width, inv_height;
		float    inv_input_width, inv_input_height;
	};

	const auto dispatch_pass = [&](const vkb::core::ImageView &dst, const vkb::core::ImageView &src, bool is_final = false) {
		discard_blur_view(dst);

		auto dst_extent = downsample_extent(dst.get_image().get_extent(), dst.get_subresource_range().baseMipLevel);
		auto src_extent = downsample_extent(src.get_image().get_extent(), src.get_subresource_range().baseMipLevel);

		Push push{};
		push.width            = dst_extent.width;
		push.height           = dst_extent.height;
		push.inv_width        = 1.0f / static_cast<float>(push.width);
		push.inv_height       = 1.0f / static_cast<float>(push.height);
		push.inv_input_width  = 1.0f / static_cast<float>(src_extent.width);
		push.inv_input_height = 1.0f / static_cast<float>(src_extent.height);

		command_buffer->push_constants(push);
		command_buffer->bind_image(src, *linear_sampler, 0, 0, 0);
		command_buffer->bind_image(dst, 0, 1, 0);
		command_buffer->dispatch((push.width + 7) / 8, (push.height + 7) / 8, 1);

		read_only_blur_view(dst, is_final);
	};

	// A very basic and dumb HDR Bloom pipeline. Don't consider this a particularly good or efficient implementation.
	// It's here to represent a plausible compute post workload.
	// - Threshold pass
	// - Blur down
	// - Blur up
	command_buffer->bind_pipeline_layout(*threshold_pipeline);
	dispatch_pass(*blur_chain_views[0], get_current_forward_render_target().get_views()[0]);

	command_buffer->bind_pipeline_layout(*blur_down_pipeline);
	for (uint32_t index = 1; index < blur_chain_views.size(); index++)
	{
		dispatch_pass(*blur_chain_views[index], *blur_chain_views[index - 1]);
	}

	command_buffer->bind_pipeline_layout(*blur_up_pipeline);
	for (uint32_t index = static_cast<uint32_t>(blur_chain_views.size() - 2); index >= 1; index--)
	{
		dispatch_pass(*blur_chain_views[index], *blur_chain_views[index + 1], index == 1);
	}

	if (post_compute_queue->get_family_index() != present_graphics_queue->get_family_index())
	{
		// release_barrier_1: Releasing color_targets[0] from post_compute to present_graphics
		//     This release barrier is replicated by the corresponding acquire_barrier_1 in the present_graphics queue
		//     The application must ensure the release operation happens before the acquire operation. This sample uses semaphores for that.
		//     The transfer ownership barriers are submitted twice (release and acquire) but they are only executed once.
		vkb::ImageMemoryBarrier memory_barrier{
		    .src_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		    .dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,        // Ignored for the release barrier.
		                                                                   // Release barriers ignore dst_access_mask unless using VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR
		    .src_access_mask  = VK_ACCESS_SHADER_READ_BIT,
		    .dst_access_mask  = 0,                                               // dst_access_mask is ignored for release barriers, without affecting its validity
		    .old_layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,        // Purely ownership transfer. We do not need a layout transition.
		    .new_layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    .src_queue_family = post_compute_queue->get_family_index(),        // Release barriers are executed from a queue of the source queue family
		    .dst_queue_family = present_graphics_queue->get_family_index(),
		};

		command_buffer->image_memory_barrier(get_current_forward_render_target().get_views()[0], memory_barrier);
	}

	command_buffer->end();

	VkPipelineStageFlags wait_stages[]     = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
	VkSemaphore          wait_semaphores[] = {wait_graphics_semaphore, wait_present_semaphore};
	VkSemaphore          signal_semaphore  = get_render_context().request_semaphore();

	auto info                 = vkb::initializers::submit_info();
	info.pSignalSemaphores    = &signal_semaphore;
	info.signalSemaphoreCount = 1;
	info.pWaitSemaphores      = wait_semaphores;
	info.waitSemaphoreCount   = wait_present_semaphore != VK_NULL_HANDLE ? 2 : 1;
	info.pWaitDstStageMask    = wait_stages;
	info.commandBufferCount   = 1;
	info.pCommandBuffers      = &command_buffer->get_handle();

	if (wait_present_semaphore != VK_NULL_HANDLE)
	{
		get_render_context().release_owned_semaphore(wait_present_semaphore);
	}

	queue.submit({info}, VK_NULL_HANDLE);
	return signal_semaphore;
}

void AsyncComputeSample::update(float delta_time)
{
	// don't call the parent's update, because it's done differently here... but call the grandparent's update for fps logging
	vkb::Application::update(delta_time);

	if (last_async_enabled != async_enabled)
	{
		setup_queues();
	}

	// We can potentially get more overlap if we double buffer the HDR render target.
	// In this scenario, the next frame can run ahead a little further before it needs to block.
	if (double_buffer_hdr_frames)
	{
		forward_render_target_index = 1 - forward_render_target_index;
	}
	else
	{
		forward_render_target_index = 0;
	}

	auto *forward_subpass   = static_cast<ShadowMapForwardSubpass *>(forward_render_pipeline.get_subpasses()[0].get());
	auto *composite_subpass = static_cast<CompositeSubpass *>(get_render_pipeline().get_subpasses()[0].get());

	forward_subpass->set_shadow_map(&shadow_render_target->get_views()[0], comparison_sampler.get());

	composite_subpass->set_texture(&get_current_forward_render_target().get_views()[0], blur_chain_views[1].get(), linear_sampler.get());        // blur_chain[1] and color_targets[0] will be used by the present queue

	float rotation_factor = std::chrono::duration<float>(std::chrono::system_clock::now() - start_time).count();

	glm::quat orientation;

	// Lots of random jank to get a desired orientation quaternion for the directional light.
	if (rotate_shadows)
	{
		// Move shadows and directional light slightly.
		orientation = glm::normalize(
		    glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, -1.0f, 0.0f)) *
		    glm::angleAxis(-0.2f * glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)) *
		    glm::angleAxis(glm::two_pi<float>() * glm::fract(rotation_factor * 0.05f), glm::vec3(0.0f, 0.0f, -1.0f)) *
		    glm::angleAxis(-0.05f * glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));
	}
	else
	{
		orientation = glm::normalize(
		    glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, -1.0f, 0.0f)) *
		    glm::angleAxis(-0.2f * glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));
	}

	auto &shadow_camera_transform = shadow_camera->get_node()->get_component<vkb::sg::Transform>();
	shadow_camera_transform.set_rotation(orientation);

	// Explicit begin_frame and end_frame since we're doing async compute, many submissions and custom semaphores ...
	get_render_context().begin_frame();

	update_scene(delta_time);
	update_gui(delta_time);

	// Collect the performance data for the sample graphs
	update_stats(delta_time);

	// Setup render pipeline:
	// - Shadow pass
	// - HDR
	// - Async compute post
	// - Composite
	render_shadow_pass();
	VkSemaphore graphics_semaphore                   = render_forward_offscreen_pass(hdr_wait_semaphores[forward_render_target_index]);
	hdr_wait_semaphores[forward_render_target_index] = VK_NULL_HANDLE;
	VkSemaphore post_semaphore                       = render_compute_post(graphics_semaphore, compute_post_semaphore);
	compute_post_semaphore                           = VK_NULL_HANDLE;
	VkSemaphore present_semaphore                    = render_swapchain(post_semaphore);

	get_render_context().end_frame(present_semaphore);
}

void AsyncComputeSample::finish()
{
	if (has_device())
	{
		for (auto &sem : hdr_wait_semaphores)
		{
			// We're outside a frame context, so free the semaphore manually.
			get_device().wait_idle();
			vkDestroySemaphore(get_device().get_handle(), sem, nullptr);
		}

		if (compute_post_semaphore)
		{
			// We're outside a frame context, so free the semaphore manually.
			get_device().wait_idle();
			vkDestroySemaphore(get_device().get_handle(), compute_post_semaphore, nullptr);
		}
	}
}

std::unique_ptr<vkb::VulkanSampleC> create_async_compute()
{
	return std::make_unique<AsyncComputeSample>();
}

AsyncComputeSample::DepthMapSubpass::DepthMapSubpass(vkb::rendering::RenderContextC &render_context,
                                                     vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader,
                                                     vkb::sg::Scene &scene, vkb::sg::Camera &camera) :
    vkb::ForwardSubpass(render_context, std::move(vertex_shader), std::move(fragment_shader), scene, camera)
{
	// PCF, so need depth bias to avoid (most) shadow acne.
	base_rasterization_state.depth_bias_enable = VK_TRUE;
}

void AsyncComputeSample::DepthMapSubpass::draw(vkb::core::CommandBufferC &command_buffer)
{
	// Negative bias since we're using inverted Z.
	command_buffer.set_depth_bias(-1.0f, 0.0f, -2.0f);
	vkb::ForwardSubpass::draw(command_buffer);
}

AsyncComputeSample::ShadowMapForwardSubpass::ShadowMapForwardSubpass(vkb::rendering::RenderContextC &render_context,
                                                                     vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader,
                                                                     vkb::sg::Scene &scene, vkb::sg::Camera &camera, vkb::sg::Camera &shadow_camera_) :
    vkb::ForwardSubpass(render_context, std::move(vertex_shader), std::move(fragment_shader), scene, camera),
    shadow_camera(shadow_camera_)
{
}

void AsyncComputeSample::ShadowMapForwardSubpass::set_shadow_map(const vkb::core::ImageView *view, const vkb::core::Sampler *sampler)
{
	shadow_view    = view;
	shadow_sampler = sampler;
}

void AsyncComputeSample::ShadowMapForwardSubpass::draw(vkb::core::CommandBufferC &command_buffer)
{
	auto shadow_matrix = vkb::rendering::vulkan_style_projection(shadow_camera.get_projection()) * shadow_camera.get_view();

	shadow_matrix = glm::translate(glm::vec3(0.5f, 0.5f, 0.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 1.0f)) * shadow_matrix;

	auto &render_frame = get_render_context().get_active_frame();

	auto allocation = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(glm::mat4), thread_index);

	allocation.update(shadow_matrix);

	// Custom part, bind shadow map to the fragment shader.
	command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 5, 0);
	command_buffer.bind_image(*shadow_view, *shadow_sampler, 0, 6, 0);

	vkb::ForwardSubpass::draw(command_buffer);
}

AsyncComputeSample::CompositeSubpass::CompositeSubpass(vkb::rendering::RenderContextC &render_context,
                                                       vkb::ShaderSource             &&vertex_shader,
                                                       vkb::ShaderSource             &&fragment_shader) :
    vkb::rendering::SubpassC(render_context, std::move(vertex_shader), std::move(fragment_shader))
{
}

void AsyncComputeSample::CompositeSubpass::set_texture(const vkb::core::ImageView *hdr_view_, const vkb::core::ImageView *bloom_view_,
                                                       const vkb::core::Sampler *sampler_)
{
	hdr_view   = hdr_view_;
	bloom_view = bloom_view_;
	sampler    = sampler_;
}

void AsyncComputeSample::CompositeSubpass::prepare()
{
	auto &device   = get_render_context().get_device();
	auto &vertex   = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader());
	auto &fragment = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader());
	layout         = &device.get_resource_cache().request_pipeline_layout({&vertex, &fragment});
}

void AsyncComputeSample::CompositeSubpass::draw(vkb::core::CommandBufferC &command_buffer)
{
	command_buffer.bind_image(*hdr_view, *sampler, 0, 0, 0);
	command_buffer.bind_image(*bloom_view, *sampler, 0, 1, 0);
	command_buffer.bind_pipeline_layout(*layout);

	// A depth-stencil attachment exists in the default render pass, make sure we ignore it.
	vkb::DepthStencilState ds_state = {};
	ds_state.depth_test_enable      = VK_FALSE;
	ds_state.stencil_test_enable    = VK_FALSE;
	ds_state.depth_write_enable     = VK_FALSE;
	ds_state.depth_compare_op       = VK_COMPARE_OP_ALWAYS;
	command_buffer.set_depth_stencil_state(ds_state);

	command_buffer.draw(3, 1, 0, 0);
}
