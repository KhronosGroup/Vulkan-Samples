/* Copyright (c) 2020-2024, Arm Limited and Contributors
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

#include "16bit_arithmetic.h"

#include "gui.h"
#include "stats/stats.h"
#include <random>
#include <scene_graph/components/camera.h>

static constexpr unsigned Width    = 1024;
static constexpr unsigned Height   = 1024;
static constexpr unsigned NumBlobs = 16;

KHR16BitArithmeticSample::KHR16BitArithmeticSample()
{
	// Enables required extensions to use 16-bit storage.
	// For this sample, this is not optional.
	// This sample also serves as a tutorial on how to use 16-bit storage
	// for SSBOs and push constants.
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, false);
	add_device_extension(VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME, false);
	add_device_extension(VK_KHR_16BIT_STORAGE_EXTENSION_NAME, false);

	// Enables the extension which allows shaders to use 16-bit float and 8-bit integer arithmetic.
	// This sample will only make use of 16-bit floats.
	add_device_extension(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME, true);

	auto &config = get_configuration();

	config.insert<vkb::BoolSetting>(0, khr_16bit_arith_enabled, false);
	config.insert<vkb::BoolSetting>(1, khr_16bit_arith_enabled, true);
}

bool KHR16BitArithmeticSample::prepare(const vkb::ApplicationOptions &options)
{
	if (!VulkanSample::prepare(options))
	{
		return false;
	}

	// Normally, we should see the immediate effect on frame times,
	// but if we're somehow hitting 60 FPS, GPU cycles / s should go down while hitting vsync.
	get_stats().request_stats({vkb::StatIndex::gpu_cycles, vkb::StatIndex::frame_times});
	create_gui(*window, &get_stats());

	// Set up some structs for the (color, depth) attachments in the default render pass.
	load_store_infos.resize(2);
	load_store_infos[0].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store_infos[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;
	load_store_infos[1].load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	load_store_infos[1].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkClearValue clear_value;
	clear_value.color.float32[0] = 0.0f;
	clear_value.color.float32[1] = 0.0f;
	clear_value.color.float32[2] = 0.0f;
	clear_value.color.float32[3] = 1.0f;
	clear_values.push_back(clear_value);
	clear_value.depthStencil.depth   = 1.0f;
	clear_value.depthStencil.stencil = 0;
	clear_values.push_back(clear_value);

	// Generate some random blobs to render and place them in a 4xfp16 data structure.
	std::default_random_engine            rng(42);
	std::normal_distribution<float>       position_dist(0.0f, 0.1f);
	std::uniform_real_distribution<float> intensity_dist(0.4f, 0.8f);
	std::uniform_real_distribution<float> falloff_dist(50.0f, 100.0f);

	glm::vec4 initial_data_fp32[NumBlobs];
	for (unsigned i = 0; i < NumBlobs; i++)
	{
		initial_data_fp32[i].x = position_dist(rng);
		initial_data_fp32[i].y = position_dist(rng);
		initial_data_fp32[i].z = intensity_dist(rng);
		initial_data_fp32[i].w = falloff_dist(rng);
	}

	// Convert FP32 to FP16.
	glm::uvec2 initial_data_fp16[NumBlobs];
	for (unsigned i = 0; i < NumBlobs; i++)
	{
		initial_data_fp16[i].x = glm::packHalf2x16(initial_data_fp32[i].xy);
		initial_data_fp16[i].y = glm::packHalf2x16(initial_data_fp32[i].zw);
	}

	// Upload the blob buffer.
	auto &device = get_render_context().get_device();

	blob_buffer         = std::make_unique<vkb::core::BufferC>(device, sizeof(initial_data_fp16),
                                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                       VMA_MEMORY_USAGE_GPU_ONLY);
	auto staging_buffer = vkb::core::BufferC::create_staging_buffer(device, initial_data_fp16);

	auto &cmd = device.request_command_buffer();
	cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
	cmd.copy_buffer(staging_buffer, *blob_buffer, sizeof(initial_data_fp16));

	vkb::BufferMemoryBarrier barrier;
	barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
	barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	cmd.buffer_memory_barrier(*blob_buffer, 0, VK_WHOLE_SIZE, barrier);
	cmd.end();

	auto &queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
	queue.submit(cmd, device.request_fence());
	device.get_fence_pool().wait();

	// Create the target image we render into in the main compute shader.
	image = std::make_unique<vkb::core::Image>(device, VkExtent3D{Width, Height, 1},
	                                           VK_FORMAT_R16G16B16A16_SFLOAT,
	                                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                                           VMA_MEMORY_USAGE_GPU_ONLY);

	image_view = std::make_unique<vkb::core::ImageView>(*image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT,
	                                                    0, 0, 1, 1);

	// Calculate valid filter
	VkFilter filter = VK_FILTER_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), image->get_format(), &filter);

	VkSamplerCreateInfo sampler_create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	sampler_create_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_create_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_create_info.magFilter           = filter;
	sampler_create_info.minFilter           = filter;
	sampler_create_info.maxLod              = VK_LOD_CLAMP_NONE;
	sampler                                 = std::make_unique<vkb::core::Sampler>(device, sampler_create_info);

	// Load shader modules.
	auto &module =
	    device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_COMPUTE_BIT,
	                                                      vkb::ShaderSource{"16bit_arithmetic/compute_buffer.comp"});
	compute_layout = &device.get_resource_cache().request_pipeline_layout({&module});

	if (supported_extensions)
	{
		vkb::ShaderVariant variant;
		if (supports_push_constant16)
		{
			variant.add_define("PUSH_CONSTANT_16");
		}

		const char *shader = "16bit_arithmetic/compute_buffer_fp16.comp";
		auto       &module_fp16 =
		    device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_COMPUTE_BIT,
		                                                      vkb::ShaderSource{shader}, variant);
		compute_layout_fp16 = &device.get_resource_cache().request_pipeline_layout({&module_fp16});
	}
	else
	{
		compute_layout_fp16 = compute_layout;
	}

	// Setup the visualization subpass which is there to blit the final result to screen.
	vkb::ShaderSource vertex_source{"16bit_arithmetic/visualize.vert"};
	vkb::ShaderSource fragment_source{"16bit_arithmetic/visualize.frag"};
	auto              subpass = std::make_unique<VisualizationSubpass>(get_render_context(),
                                                          std::move(vertex_source),
                                                          std::move(fragment_source));

	subpass->view    = image_view.get();
	subpass->sampler = sampler.get();
	subpasses.emplace_back(std::move(subpass));
	for (auto &subpass : subpasses)
	{
		subpass->prepare();
	}

	return true;
}

KHR16BitArithmeticSample::VisualizationSubpass::VisualizationSubpass(vkb::RenderContext &context,
                                                                     vkb::ShaderSource &&vertex_source,
                                                                     vkb::ShaderSource &&fragment_source) :
    vkb::rendering::SubpassC(context, std::move(vertex_source), std::move(fragment_source))
{
	set_output_attachments({0});
}

void KHR16BitArithmeticSample::VisualizationSubpass::draw(vkb::CommandBuffer &command_buffer)
{
	command_buffer.bind_pipeline_layout(*layout);

	// A depth-stencil attachment exists in the default render pass, make sure we ignore it.
	vkb::DepthStencilState ds_state = {};
	ds_state.depth_test_enable      = VK_FALSE;
	ds_state.stencil_test_enable    = VK_FALSE;
	ds_state.depth_write_enable     = VK_FALSE;
	ds_state.depth_compare_op       = VK_COMPARE_OP_ALWAYS;
	command_buffer.set_depth_stencil_state(ds_state);

	command_buffer.bind_image(*view, *sampler, 0, 0, 0);
	command_buffer.draw(3, 1, 0, 0);
}

void KHR16BitArithmeticSample::VisualizationSubpass::prepare()
{
	auto                            &device             = get_render_context().get_device();
	auto                            &vert_shader_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader());
	auto                            &frag_shader_module = device.get_resource_cache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader());
	std::vector<vkb::ShaderModule *> shader_modules{&vert_shader_module, &frag_shader_module};
	layout = &device.get_resource_cache().request_pipeline_layout(shader_modules);
}

void KHR16BitArithmeticSample::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	// Required features.
	REQUEST_REQUIRED_FEATURE(gpu,
	                         VkPhysicalDevice16BitStorageFeatures,
	                         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES,
	                         storageBuffer16BitAccess);

	// Optional features.
	supported_extensions = REQUEST_OPTIONAL_FEATURE(gpu,
	                                                VkPhysicalDeviceFloat16Int8FeaturesKHR,
	                                                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR,
	                                                shaderFloat16);

	supports_push_constant16 = gpu.get_extension_features<VkPhysicalDevice16BitStorageFeatures>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES).storagePushConstant16;
}

void KHR16BitArithmeticSample::draw_renderpass(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
	if (khr_16bit_arith_enabled)
	{
		command_buffer.bind_pipeline_layout(*compute_layout_fp16);
	}
	else
	{
		command_buffer.bind_pipeline_layout(*compute_layout);
	}

	command_buffer.bind_buffer(*blob_buffer, 0, NumBlobs * sizeof(glm::uvec2), 0, 0, 0);
	command_buffer.bind_image(*image_view, 0, 1, 0);

	// Wait for fragment shader is done reading before we can write in compute.
	vkb::ImageMemoryBarrier write_after_read_hazard;
	write_after_read_hazard.src_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	write_after_read_hazard.dst_stage_mask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	write_after_read_hazard.src_access_mask = 0;
	write_after_read_hazard.dst_access_mask = VK_ACCESS_SHADER_WRITE_BIT;
	write_after_read_hazard.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
	write_after_read_hazard.new_layout      = VK_IMAGE_LAYOUT_GENERAL;
	command_buffer.image_memory_barrier(*image_view, write_after_read_hazard);

	// 16-bit push constants are supported by VK_KHR_16bit_storage, which is handy for conserving space without
	// using many "unpack" instructions in the shader.
	struct Push16
	{
		uint16_t num_blobs;
		uint16_t fp16_seed;
		int16_t  range_x, range_y;
	} push16 = {};

	struct Push32
	{
		uint32_t num_blobs;
		float    fp32_seed;
		int32_t  range_x, range_y;
	} push32 = {};

	frame_count      = (frame_count + 1u) & 511u;
	float seed_value = 0.5f * glm::sin(glm::two_pi<float>() * (static_cast<float>(frame_count) / 512.0f));

	push32.num_blobs = NumBlobs;
	push32.fp32_seed = seed_value;
	push32.range_x   = 2;
	push32.range_y   = 1;

	if (khr_16bit_arith_enabled && supports_push_constant16)
	{
		push16.num_blobs = push32.num_blobs;
		push16.fp16_seed = static_cast<uint16_t>(glm::packHalf2x16(glm::vec2(push32.fp32_seed)));
		push16.range_x   = push32.range_x;
		push16.range_y   = push32.range_y;
		command_buffer.push_constants(push16);
	}
	else
	{
		command_buffer.push_constants(push32);
	}

	command_buffer.set_specialization_constant(0, Width);
	command_buffer.set_specialization_constant(1, Height);

	// Workgroup size is (8, 8)
	command_buffer.dispatch(Width / 8, Height / 8, 1);

	vkb::ImageMemoryBarrier to_fragment_barrier;
	to_fragment_barrier.old_layout      = VK_IMAGE_LAYOUT_GENERAL;
	to_fragment_barrier.new_layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	to_fragment_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	to_fragment_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	to_fragment_barrier.src_access_mask = VK_ACCESS_SHADER_WRITE_BIT;
	to_fragment_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
	command_buffer.image_memory_barrier(*image_view, to_fragment_barrier);

	// Blit result to screen and render UI.
	command_buffer.begin_render_pass(render_target, load_store_infos, clear_values, subpasses);
	command_buffer.set_viewport(0, {{0.0f, 0.0f, static_cast<float>(render_target.get_extent().width), static_cast<float>(render_target.get_extent().height), 0.0f, 1.0f}});
	command_buffer.set_scissor(0, {{{0, 0}, render_target.get_extent()}});
	subpasses.front()->draw(command_buffer);

	get_gui().draw(command_buffer);
	command_buffer.end_render_pass();
}

void KHR16BitArithmeticSample::draw_gui()
{
	const char *label;
	if (supported_extensions)
	{
		label = "Enable 16-bit arithmetic";
	}
	else
	{
		label = "16-bit arithmetic (unsupported features)";
	}

	get_gui().show_options_window(
	    /* body = */ [this, label]() {
		    if (!supported_extensions)
		    {
			    ImGui::Text("%s", label);
		    }
		    else
		    {
			    ImGui::Checkbox(label, &khr_16bit_arith_enabled);
		    }
	    },
	    /* lines = */ 1);
}

std::unique_ptr<vkb::VulkanSampleC> create_16bit_arithmetic()
{
	return std::make_unique<KHR16BitArithmeticSample>();
}
