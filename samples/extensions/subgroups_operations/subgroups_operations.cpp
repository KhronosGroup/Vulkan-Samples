/* Copyright (c) 2023, Mobica Limited
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

#include "subgroups_operations.h"
#include <glsl_compiler.h>

#include <random>

#define GRAVITY 9.81f

void SubgroupsOperations::Pipeline::destroy(VkDevice device)
{
	if (pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, pipeline, nullptr);
	}

	if (pipeline_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
	}
}

SubgroupsOperations::SubgroupsOperations()
{
	// SPIRV 1.4 requires Vulkan 1.1
	set_api_version(VK_API_VERSION_1_1);

	// Subgroup size control extensions required by this sample
	add_device_extension(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);

	// Required for VK_EXT_subgroup_size_control
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// Required by VK_KHR_spirv_1_4
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	// TODO: remove when not needed; for #extension GL_EXT_debug_printf : enable
	add_device_extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);

	// Targeting SPIR-V version
	vkb::GLSLCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);

	title       = "Subgroups operations";
	camera.type = vkb::CameraType::FirstPerson;

	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 256.0f);
	camera.set_position({0.0f, 5.0f, 0.0f});
}

SubgroupsOperations::~SubgroupsOperations()
{
	if (device)
	{
		fft_buffers.fft_tilde_h_kt_dx->destroy(get_device().get_handle());
		fft_buffers.fft_tilde_h_kt_dy->destroy(get_device().get_handle());
		fft_buffers.fft_tilde_h_kt_dz->destroy(get_device().get_handle());
		fft_buffers.fft_displacement->destroy(get_device().get_handle());
		fft_buffers.fft_input_htilde0->destroy(get_device().get_handle());
		fft_buffers.fft_input_htilde0_conj->destroy(get_device().get_handle());
		butterfly_precomp.destroy(get_device().get_handle());

		precompute.pipeline.destroy(get_device().get_handle());
		vkDestroyDescriptorSetLayout(get_device().get_handle(), precompute.descriptor_set_layout, nullptr);

		tildas.pipeline.destroy(get_device().get_handle());
		vkDestroyDescriptorSetLayout(get_device().get_handle(), tildas.descriptor_set_layout, nullptr);

		initial_tildas.pipeline.destroy(get_device().get_handle());
		vkDestroyDescriptorSetLayout(get_device().get_handle(), initial_tildas.descriptor_set_layout, nullptr);

		fft_inversion.pipeline.destroy(get_device().get_handle());
		vkDestroyDescriptorSetLayout(get_device().get_handle(), fft_inversion.descriptor_set_layout, nullptr);

		fft.tilde_axis_x->destroy(get_device().get_handle());
		fft.tilde_axis_y->destroy(get_device().get_handle());
		fft.tilde_axis_z->destroy(get_device().get_handle());
		fft.pipelines.horizontal.destroy(get_device().get_handle());
		fft.pipelines.vertical.destroy(get_device().get_handle());
		vkDestroyDescriptorSetLayout(get_device().get_handle(), fft.descriptor_set_layout, nullptr);

		vkDestroySemaphore(get_device().get_handle(), compute.semaphore, nullptr);
		vkDestroyCommandPool(get_device().get_handle(), compute.command_pool, nullptr);

		ocean.pipelines._default.destroy(get_device().get_handle());
		ocean.pipelines.wireframe.destroy(get_device().get_handle());
		vkDestroyDescriptorSetLayout(get_device().get_handle(), ocean.descriptor_set_layout, nullptr);
		vkDestroySemaphore(get_device().get_handle(), ocean.semaphore, nullptr);
	}
}

bool SubgroupsOperations::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	ocean.graphics_queue_family_index = get_device().get_queue_family_index(VK_QUEUE_GRAPHICS_BIT);

	load_assets();
	setup_descriptor_pool();
	prepare_uniform_buffers();
	prepare_compute();

	// prepare grpahics pipeline
	create_semaphore();
	create_descriptor_set_layout();

	create_initial_tildas();
	create_tildas();
	create_butterfly_texture();
	create_fft();
	create_fft_inversion();

	create_descriptor_set();
	create_pipelines();

	build_compute_command_buffer();

	build_command_buffers();

	// signal semaphore
	VkSubmitInfo submit_info         = vkb::initializers::submit_info();
	submit_info.signalSemaphoreCount = 1u;
	submit_info.pSignalSemaphores    = &ocean.semaphore;
	VK_CHECK(vkQueueSubmit(queue, 1u, &submit_info, VK_NULL_HANDLE));
	get_device().wait_idle();

	prepared = true;
	return true;
}

void SubgroupsOperations::prepare_compute()
{
	create_compute_queue();
	create_compute_command_pool();
	create_compute_command_buffer();
}

void SubgroupsOperations::create_compute_queue()
{
	// create compute queue and get family index
	compute.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_COMPUTE_BIT);

	vkGetDeviceQueue(get_device().get_handle(), compute.queue_family_index, 0u, &compute.queue);
}

void SubgroupsOperations::create_compute_command_pool()
{
	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex        = compute.queue_family_index;
	command_pool_create_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK(vkCreateCommandPool(get_device().get_handle(), &command_pool_create_info, nullptr, &compute.command_pool));
}

void SubgroupsOperations::create_compute_command_buffer()
{
	// Create a command buffer for compute operations
	VkCommandBufferAllocateInfo command_buffer_allocate_info =
	    vkb::initializers::command_buffer_allocate_info(compute.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1u);

	VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &command_buffer_allocate_info, &compute.command_buffer));

	// Semaphore for compute & graphics sync
	VkSemaphoreCreateInfo semaphore_create_info = vkb::initializers::semaphore_create_info();
	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &semaphore_create_info, nullptr, &compute.semaphore));
}

void SubgroupsOperations::build_compute_command_buffer()
{
	// record compute command
	VkCommandBufferBeginInfo begin_info = vkb::initializers::command_buffer_begin_info();
	VK_CHECK(vkBeginCommandBuffer(compute.command_buffer, &begin_info));

	// buttle fly texture
	{
		vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, precompute.pipeline.pipeline);
		vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, precompute.pipeline.pipeline_layout, 0u, 1u, &precompute.descriptor_set, 0u, nullptr);

		vkCmdDispatch(compute.command_buffer, 1u, DISPLACEMENT_MAP_DIM, 1u);
	}

	// initial tildas textures
	{
		vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, initial_tildas.pipeline.pipeline);
		vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, initial_tildas.pipeline.pipeline_layout, 0u, 1u, &initial_tildas.descriptor_set, 0u, nullptr);

		vkCmdDispatch(compute.command_buffer, DISPLACEMENT_MAP_DIM / 32u, DISPLACEMENT_MAP_DIM, 1u);
	}

	// tildas textures
	{
		vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, tildas.pipeline.pipeline);
		vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, tildas.pipeline.pipeline_layout, 0u, 1u, &tildas.descriptor_set, 0u, nullptr);

		vkCmdDispatch(compute.command_buffer, DISPLACEMENT_MAP_DIM / 8u, DISPLACEMENT_MAP_DIM, 1u);
	}

	// fft horizontal; for Y axis
	{
		vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.horizontal.pipeline);
		vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.horizontal.pipeline_layout, 0u, 1u, &fft.descriptor_set_axis_y, 0u, nullptr);
		vkCmdDispatch(compute.command_buffer, DISPLACEMENT_MAP_DIM / 32u, DISPLACEMENT_MAP_DIM, 1u);
	}

	/*
	    // fft horizontal; for X axis
	    {
	        vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.horizontal.pipeline);
	        vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.horizontal.pipeline_layout, 0u, 1u, &fft.descriptor_set_axis_x, 0u, nullptr);
	        vkCmdDispatch(compute.command_buffer, DISPLACEMENT_MAP_DIM / 32u, DISPLACEMENT_MAP_DIM, 1u);
	    }

	    // fft horizontal; for Z axis
	    {
	        vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.horizontal.pipeline);
	        vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.horizontal.pipeline_layout, 0u, 1u, &fft.descriptor_set_axis_z, 0u, nullptr);
	        vkCmdDispatch(compute.command_buffer, DISPLACEMENT_MAP_DIM / 32u, DISPLACEMENT_MAP_DIM, 1u);
	    }

	    // fft vertical; for Y axis
	    {
	        vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.vertical.pipeline);
	        vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.vertical.pipeline_layout, 0u, 1u, &fft.descriptor_set_axis_y, 0u, nullptr);
	        vkCmdDispatch(compute.command_buffer, DISPLACEMENT_MAP_DIM / 32u, DISPLACEMENT_MAP_DIM, 1u);
	    }

	    // fft vertical; for X axis
	    {
	        vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.vertical.pipeline);
	        vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.vertical.pipeline_layout, 0u, 1u, &fft.descriptor_set_axis_x, 0u, nullptr);
	        vkCmdDispatch(compute.command_buffer, DISPLACEMENT_MAP_DIM / 32u, DISPLACEMENT_MAP_DIM, 1u);
	    }

	    // fft vertical; for Z axis
	    {
	        vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.vertical.pipeline);
	        vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft.pipelines.vertical.pipeline_layout, 0u, 1u, &fft.descriptor_set_axis_z, 0u, nullptr);
	        vkCmdDispatch(compute.command_buffer, DISPLACEMENT_MAP_DIM / 32u, DISPLACEMENT_MAP_DIM, 1u);
	    }

	// fft inverse
	{
		vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft_inversion.pipeline.pipeline);
		vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft_inversion.pipeline.pipeline_layout, 0u, 1u, &fft_inversion.descriptor_set, 0u, nullptr);
		vkCmdDispatch(compute.command_buffer, DISPLACEMENT_MAP_DIM / 32u, DISPLACEMENT_MAP_DIM, 1u);
	}

	VK_CHECK(vkEndCommandBuffer(compute.command_buffer));
}

void SubgroupsOperations::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().fillModeNonSolid)
	{
		gpu.get_mutable_requested_features().fillModeNonSolid = VK_TRUE;
	}

	if (gpu.get_features().vertexPipelineStoresAndAtomics)
	{
		gpu.get_mutable_requested_features().vertexPipelineStoresAndAtomics = VK_TRUE;
	}

	if (gpu.get_features().tessellationShader)
	{
		gpu.get_mutable_requested_features().tessellationShader = VK_TRUE;
	}
	else
	{
		throw vkb::VulkanException(VK_ERROR_FEATURE_NOT_PRESENT, "Selected GPU does not support tessellation shaders!");
	}

	subgroups_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
	subgroups_properties.pNext = VK_NULL_HANDLE;

	VkPhysicalDeviceProperties2 device_properties2 = {};
	device_properties2.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_properties2.pNext                       = &subgroups_properties;
	vkGetPhysicalDeviceProperties2(gpu.get_handle(), &device_properties2);
}

void SubgroupsOperations::create_initial_tildas()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindngs = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 3u)};

	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindngs);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &initial_tildas.descriptor_set_layout));

	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &initial_tildas.descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &initial_tildas.descriptor_set));

	VkPipelineLayoutCreateInfo compute_pipeline_layout_info = {};
	compute_pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compute_pipeline_layout_info.setLayoutCount             = 1u;
	compute_pipeline_layout_info.pSetLayouts                = &initial_tildas.descriptor_set_layout;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &compute_pipeline_layout_info, nullptr, &initial_tildas.pipeline.pipeline_layout));

	VkComputePipelineCreateInfo computeInfo = vkb::initializers::compute_pipeline_create_info(initial_tildas.pipeline.pipeline_layout);
	computeInfo.stage                       = load_shader("subgroups_operations/fft_tilde_h0.comp", VK_SHADER_STAGE_COMPUTE_BIT);

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1u, &computeInfo, nullptr, &initial_tildas.pipeline.pipeline));

	fft_buffers.fft_input_htilde0      = std::make_unique<FBAttachment>();
	fft_buffers.fft_input_htilde0_conj = std::make_unique<FBAttachment>();

	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, grid_size, grid_size, *fft_buffers.fft_input_htilde0);
	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, grid_size, grid_size, *fft_buffers.fft_input_htilde0_conj);

	VkDescriptorImageInfo  htilde_0_descriptor      = create_fb_descriptor(*fft_buffers.fft_input_htilde0);
	VkDescriptorImageInfo  htilde_conj_0_descriptor = create_fb_descriptor(*fft_buffers.fft_input_htilde0_conj);
	VkDescriptorBufferInfo input_random_descriptor  = create_descriptor(*fft_buffers.fft_input_random);
	VkDescriptorBufferInfo fft_params_ubo_buffer    = create_descriptor(*fft_params_ubo);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(initial_tildas.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0u, &htilde_0_descriptor),
	    vkb::initializers::write_descriptor_set(initial_tildas.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, &htilde_conj_0_descriptor),
	    vkb::initializers::write_descriptor_set(initial_tildas.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, &input_random_descriptor),
	    vkb::initializers::write_descriptor_set(initial_tildas.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3u, &fft_params_ubo_buffer)};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0u, nullptr);
}

void SubgroupsOperations::create_tildas()
{
	fft_buffers.fft_tilde_h_kt_dx = std::make_unique<FBAttachment>();
	fft_buffers.fft_tilde_h_kt_dy = std::make_unique<FBAttachment>();
	fft_buffers.fft_tilde_h_kt_dz = std::make_unique<FBAttachment>();

	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, grid_size, grid_size, *fft_buffers.fft_tilde_h_kt_dx);
	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, grid_size, grid_size, *fft_buffers.fft_tilde_h_kt_dy);
	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, grid_size, grid_size, *fft_buffers.fft_tilde_h_kt_dz);

	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindngs = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 3u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 4u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 5u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 6u)};
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 6u)};

	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindngs);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &tildas.descriptor_set_layout));

	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &tildas.descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &tildas.descriptor_set));

	VkPipelineLayoutCreateInfo compute_pipeline_layout_info = {};
	compute_pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compute_pipeline_layout_info.setLayoutCount             = 1u;
	compute_pipeline_layout_info.pSetLayouts                = &tildas.descriptor_set_layout;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &compute_pipeline_layout_info, nullptr, &tildas.pipeline.pipeline_layout));

	VkComputePipelineCreateInfo computeInfo = vkb::initializers::compute_pipeline_create_info(tildas.pipeline.pipeline_layout);
	computeInfo.stage                       = load_shader("subgroups_operations/fft_tilde_h.comp", VK_SHADER_STAGE_COMPUTE_BIT);

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1u, &computeInfo, nullptr, &tildas.pipeline.pipeline));

	VkDescriptorImageInfo htilde_0_descriptor      = create_fb_descriptor(*fft_buffers.fft_input_htilde0);
	VkDescriptorImageInfo htilde_conj_0_descriptor = create_fb_descriptor(*fft_buffers.fft_input_htilde0_conj);

	VkDescriptorImageInfo image_dx_descriptor = create_fb_descriptor(*fft_buffers.fft_tilde_h_kt_dx);
	VkDescriptorImageInfo image_dy_descriptor = create_fb_descriptor(*fft_buffers.fft_tilde_h_kt_dy);
	VkDescriptorImageInfo image_dz_descriptor = create_fb_descriptor(*fft_buffers.fft_tilde_h_kt_dz);

	VkDescriptorBufferInfo fft_params_ubo_buffer = create_descriptor(*fft_params_ubo);
	VkDescriptorBufferInfo fft_time_ubo_buffer   = create_descriptor(*fft_time_ubo);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(tildas.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0u, &htilde_0_descriptor),
	    vkb::initializers::write_descriptor_set(tildas.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, &htilde_conj_0_descriptor),
	    vkb::initializers::write_descriptor_set(tildas.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2u, &image_dx_descriptor),
	    vkb::initializers::write_descriptor_set(tildas.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3u, &image_dy_descriptor),
	    vkb::initializers::write_descriptor_set(tildas.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4u, &image_dz_descriptor),
	    vkb::initializers::write_descriptor_set(tildas.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5u, &fft_params_ubo_buffer),
	    vkb::initializers::write_descriptor_set(tildas.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6u, &fft_time_ubo_buffer)};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0u, nullptr);
}

void SubgroupsOperations::load_assets()
{
	generate_plane();
	log_2_N = glm::log2(static_cast<float>(grid_size));

	/*

	// generate fft inputs
	h_tilde_0.clear();
	h_tilde_0_conj.clear();
	for (uint32_t m = 0; m < grid_size; ++m)
	{
	    for (uint32_t n = 0; n < grid_size; ++n)
	    {
	        h_tilde_0.push_back(hTilde_0(n, m, 1));
	        h_tilde_0_conj.push_back(std::conj(hTilde_0(-n, -m, 1)));
	    }
	}

	// calculate weights
	uint32_t pow2 = 1U;
	for (uint32_t i = 0U; i < log_2_N; ++i)
	{
	    for (uint32_t j = 0U; j < pow2; ++j)
	    {
	        weights.push_back(calculate_weight(j, pow2 * 2));
	    }
	    pow2 *= 2;
	}

	auto fft_input_htilde0_size      = static_cast<VkDeviceSize>(h_tilde_0.size() * sizeof(std::complex<float>));
	fft_buffers.fft_input_htilde0    = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                    fft_input_htilde0_size,
	                                                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
	auto fft_input_htilde0_conj_size = static_cast<VkDeviceSize>(h_tilde_0_conj.size() * sizeof(std::complex<float>));

	fft_buffers.fft_input_htilde0_conj = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                                         fft_input_htilde0_conj_size,
	                                                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);

	auto fft_input_weights_size  = static_cast<VkDeviceSize>(weights.size() * sizeof(std::complex<float>));
	fft_buffers.fft_input_weight = std::make_unique<vkb::core::Buffer>(get_device(), fft_input_weights_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	fft_buffers.fft_input_htilde0->update(h_tilde_0.data(), fft_input_htilde0_size);
	fft_buffers.fft_input_htilde0_conj->update(h_tilde_0_conj.data(), fft_input_htilde0_conj_size);
	fft_buffers.fft_input_weight->update(weights.data(), fft_input_weights_size);

	*/
}

glm::vec2 SubgroupsOperations::rndGaussian()
{
	float x1, x2, w;
	auto  rndVal = []() -> float {
        std::random_device                    rndDevice;
        std::mt19937                          mt(rndDevice());
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);
        return dis(mt);
	};
	do
	{
		x1 = 2.0f * rndVal() - 1.0f;
		x2 = 2.0f * rndVal() - 1.0f;
		w  = x1 * x1 + x2 * x2;
	} while (w >= 1.0f);
	w = glm::sqrt((-2.0f * glm::log(w)) / w);
	return glm::vec2{x1 * w, x2 * w};
}

void SubgroupsOperations::prepare_uniform_buffers()
{
	camera_ubo              = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(CameraUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	camera_postion_ubo      = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(CameraPosition), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	fft_params_ubo          = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(FFTParametersUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	fft_time_ubo            = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(TimeUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	invert_fft_ubo          = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(FFTInvert), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	tessellation_params_ubo = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(TessellationParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void SubgroupsOperations::generate_plane()
{
	uint32_t              vertex_count = grid_size + 1u;
	std::vector<Vertex>   plane_vertices;
	const float           tex_coord_scale = 64.0f;
	std::vector<uint32_t> indices;
	int32_t               half_grid_size = static_cast<int32_t>(grid_size / 2);

	for (int32_t z = -half_grid_size; z <= half_grid_size; ++z)
	{
		for (int32_t x = -half_grid_size; x <= half_grid_size; ++x)
		{
			float  u = static_cast<float>(x) / static_cast<float>(grid_size) + 0.5f;
			float  v = static_cast<float>(z) / static_cast<float>(grid_size) + 0.5f;
			Vertex vert;
			vert.pos = glm::vec3(float(x), 0.0f, float(z));
			vert.uv  = glm::vec2(u, v) * tex_coord_scale;

			plane_vertices.push_back(vert);
		}
	}

	for (uint32_t y = 0u; y < grid_size; ++y)
	{
		for (uint32_t x = 0u; x < grid_size; ++x)
		{
			// tris 1
			indices.push_back((vertex_count * y) + x);
			indices.push_back((vertex_count * (y + 1u)) + x);
			indices.push_back((vertex_count * y) + x + 1u);

			// tris 2
			indices.push_back((vertex_count * y) + x + 1u);
			indices.push_back((vertex_count * (y + 1u)) + x);
			indices.push_back((vertex_count * (y + 1u)) + x + 1u);
		}
	}

	auto vertex_buffer_size = vkb::to_u32(plane_vertices.size() * sizeof(Vertex));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));
	ocean.grid.index_count  = vkb::to_u32(indices.size());

	ocean.grid.vertex = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        vertex_buffer_size,
	                                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);

	ocean.grid.index = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                       index_buffer_size,
	                                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                       VMA_MEMORY_USAGE_CPU_TO_GPU);

	ocean.grid.vertex->update(plane_vertices.data(), vertex_buffer_size);
	ocean.grid.index->update(indices.data(), index_buffer_size);
}

void SubgroupsOperations::create_semaphore()
{
	// Semaphore for graphics queue
	VkSemaphoreCreateInfo semaphore_create_info = vkb::initializers::semaphore_create_info();
	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &semaphore_create_info, nullptr, &ocean.semaphore));
}

void SubgroupsOperations::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 7u),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5u),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 7u)};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 7u);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void SubgroupsOperations::create_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	        0u),
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	        1u),
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	        2u),
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	        3u)};

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_set_layout_create_info, nullptr, &ocean.descriptor_set_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = vkb::initializers::pipeline_layout_create_info(&ocean.descriptor_set_layout);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &ocean.pipelines._default.pipeline_layout));
}

void SubgroupsOperations::create_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &ocean.descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &ocean.descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor              = create_descriptor(*camera_ubo);
	VkDescriptorImageInfo  desplacement_descriptor        = create_fb_descriptor(*fft_buffers.fft_displacement);
	VkDescriptorBufferInfo tessellation_params_descriptor = create_descriptor(*tessellation_params_ubo);
	VkDescriptorBufferInfo camera_pos_buffer_descriptor   = create_descriptor(*camera_postion_ubo);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(ocean.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, &buffer_descriptor),
	    vkb::initializers::write_descriptor_set(ocean.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, &desplacement_descriptor),
	    vkb::initializers::write_descriptor_set(ocean.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u, &tessellation_params_descriptor),
	    vkb::initializers::write_descriptor_set(ocean.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3u, &camera_pos_buffer_descriptor)};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0u, nullptr);
}

void SubgroupsOperations::create_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
	        0u,
	        VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_NONE,
	        VK_FRONT_FACE_COUNTER_CLOCKWISE,
	        0u);
	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);
	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1u,
	        &blend_attachment_state);
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);
	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1u, 1u, 0u);
	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0u);
	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0u);
	VkPipelineTessellationStateCreateInfo tessellation_state = vkb::initializers::pipeline_tessellation_state_create_info(3u);

	std::array<VkPipelineShaderStageCreateInfo, 4> shader_stages;
	shader_stages[0] = load_shader("subgroups_operations/ocean.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("subgroups_operations/ocean.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
	shader_stages[2] = load_shader("subgroups_operations/ocean.tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	shader_stages[3] = load_shader("subgroups_operations/ocean.tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0u, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0u, 0u, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
	    vkb::initializers::vertex_input_attribute_description(0u, 1u, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv))};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(ocean.pipelines._default.pipeline_layout, render_pass, 0u);
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pRasterizationState          = &rasterization_state;
	pipeline_create_info.pColorBlendState             = &color_blend_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;
	pipeline_create_info.pTessellationState           = &tessellation_state;
	pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages                      = shader_stages.data();
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1u, &pipeline_create_info, nullptr, &ocean.pipelines._default.pipeline));

	if (get_device().get_gpu().get_features().fillModeNonSolid)
	{
		rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
		VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1u, &pipeline_create_info, nullptr, &ocean.pipelines.wireframe.pipeline));
	}
}

void SubgroupsOperations::update_uniform_buffers()
{
	CameraUbo ubo;
	ubo.model      = glm::mat4(1.0f);
	ubo.model      = glm::translate(ubo.model, glm::vec3(0.0f));
	ubo.view       = camera.matrices.view;
	ubo.projection = camera.matrices.perspective;

	CameraPosition cam_pos;
	cam_pos.position = glm::vec4(camera.position, 0.0f);

	FFTParametersUbo fft_ubo;
	fft_ubo.amplitude = ui.amplitude;
	fft_ubo.grid_size = grid_size;
	fft_ubo.length    = ui.length;
	fft_ubo.wind      = ui.wind;

	FFTInvert invertFft;
	invertFft.grid_size = grid_size;
	invertFft.page_idx  = log_2_N % 2;

	TessellationParams tess_params;
	tess_params.displacement_scale = 0.5f;
	tess_params.choppines          = 0.75f;

	TimeUbo t;
	t.time = float(timer.elapsed<vkb::Timer::Seconds>());

	fft_time_ubo->convert_and_update(t);
	camera_ubo->convert_and_update(ubo);
	fft_params_ubo->convert_and_update(fft_ubo);
	fft_time_ubo->convert_and_update(fftTime);
	invert_fft_ubo->convert_and_update(invertFft);
	tessellation_params_ubo->convert_and_update(tess_params);
	camera_postion_ubo->convert_and_update(cam_pos);
}

void SubgroupsOperations::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	std::array<VkClearValue, 2> clear_values;
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0u};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = static_cast<uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues             = clear_values.data();

	for (uint32_t i = 0u; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];
		auto &cmd_buff                     = draw_cmd_buffers[i];

		VK_CHECK(vkBeginCommandBuffer(cmd_buff, &command_buffer_begin_info));

		vkCmdBeginRenderPass(cmd_buff, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd_buff, 0u, 1u, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(cmd_buff, 0u, 1u, &scissor);

		// draw ocean
		{
			vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, ocean.pipelines._default.pipeline_layout, 0u, 1u, &ocean.descriptor_set, 0u, nullptr);
			vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, ui.wireframe ? ocean.pipelines.wireframe.pipeline : ocean.pipelines._default.pipeline);

			VkDeviceSize offset[] = {0u};
			vkCmdBindVertexBuffers(cmd_buff, 0u, 1u, ocean.grid.vertex->get(), offset);
			vkCmdBindIndexBuffer(cmd_buff, ocean.grid.index->get_handle(), VkDeviceSize(0), VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(cmd_buff, ocean.grid.index_count, 1u, 0u, 0u, 0u);
		}

		draw_ui(cmd_buff);

		vkCmdEndRenderPass(cmd_buff);

		VK_CHECK(vkEndCommandBuffer(cmd_buff));
	}
}

void SubgroupsOperations::draw()
{
	VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	// Submit compute commands
	VkSubmitInfo compute_submit_info         = vkb::initializers::submit_info();
	compute_submit_info.commandBufferCount   = 1u;
	compute_submit_info.pCommandBuffers      = &compute.command_buffer;
	compute_submit_info.waitSemaphoreCount   = 1u;
	compute_submit_info.pWaitSemaphores      = &ocean.semaphore;
	compute_submit_info.pWaitDstStageMask    = &wait_stage_mask;
	compute_submit_info.signalSemaphoreCount = 1u;
	compute_submit_info.pSignalSemaphores    = &compute.semaphore;

	VK_CHECK(vkQueueSubmit(compute.queue, 1u, &compute_submit_info, VK_NULL_HANDLE));

	VkPipelineStageFlags graphics_wait_stage_masks[]  = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore          graphics_wait_semaphores[]   = {compute.semaphore, semaphores.acquired_image_ready};
	VkSemaphore          graphics_signal_semaphores[] = {ocean.semaphore, semaphores.render_complete};

	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount   = 1u;
	submit_info.pCommandBuffers      = &draw_cmd_buffers[current_buffer];
	submit_info.waitSemaphoreCount   = 2u;
	submit_info.pWaitSemaphores      = graphics_wait_semaphores;
	submit_info.pWaitDstStageMask    = graphics_wait_stage_masks;
	submit_info.signalSemaphoreCount = 2u;
	submit_info.pSignalSemaphores    = graphics_signal_semaphores;

	VK_CHECK(vkQueueSubmit(queue, 1u, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
}

void SubgroupsOperations::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (get_device().get_gpu().get_features().fillModeNonSolid)
		{
			if (drawer.checkbox("Wireframe", &ui.wireframe))
			{
				build_command_buffers();
			}
		}
	}

	if (drawer.header("Ocean settings"))

	{
		if (drawer.input_float("Amplitude", &ui.amplitude, 0.1f, 3u))
		{
			// update input for fft
		}
		if (drawer.input_float("Length", &ui.length, 10.f, 1u))
		{
			// update input for fft
		}
		if (drawer.header("Wind"))
		{
			if (drawer.input_float("X", &ui.wind.x, 10.f, 2u))
			{
				// update input for fft
			}
			if (drawer.input_float("Y", &ui.wind.y, 10.f, 2u))
			{
				// update input for fft
			}
		}
	}
}

bool SubgroupsOperations::resize(const uint32_t width, const uint32_t height)
{
	if (!ApiVulkanSample::resize(width, height))
		return false;
	update_uniform_buffers();
	build_compute_command_buffer();
	build_command_buffers();
	return true;
}

void SubgroupsOperations::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	if (!timer.is_running())
		timer.start();

	update_uniform_buffers();
	draw();

	auto elapsed_time = static_cast<float>(timer.elapsed<vkb::Timer::Seconds>());
	if (elapsed_time >= 1.0f)
		timer.lap();
}

std::unique_ptr<vkb::VulkanSample> create_subgroups_operations()
{
	return std::make_unique<SubgroupsOperations>();
}

// TODO: move out usage to the function arguments
void SubgroupsOperations::createFBAttachement(VkFormat format, uint32_t width, uint32_t height, FBAttachment &attachment)
{
	attachment.format = format;

	VkImageCreateInfo image = vkb::initializers::image_create_info();
	image.imageType         = VK_IMAGE_TYPE_2D;
	image.format            = format;
	image.extent.width      = width;
	image.extent.height     = height;
	image.extent.depth      = 1u;
	image.mipLevels         = 1u;
	image.arrayLayers       = 1u;
	image.samples           = VK_SAMPLE_COUNT_1_BIT;
	image.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	image.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

	VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
	VkMemoryRequirements memory_requirements;

	VK_CHECK(vkCreateImage(get_device().get_handle(), &image, nullptr, &attachment.image));
	vkGetImageMemoryRequirements(get_device().get_handle(), attachment.image, &memory_requirements);
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &attachment.memory));
	VK_CHECK(vkBindImageMemory(get_device().get_handle(), attachment.image, attachment.memory, 0));

	VkImageViewCreateInfo image_view_create_info           = vkb::initializers::image_view_create_info();
	image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format                          = format;
	image_view_create_info.subresourceRange                = {};
	image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_create_info.subresourceRange.baseMipLevel   = 0u;
	image_view_create_info.subresourceRange.levelCount     = 1u;
	image_view_create_info.subresourceRange.baseArrayLayer = 0u;
	image_view_create_info.subresourceRange.layerCount     = 1u;
	image_view_create_info.image                           = attachment.image;
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view_create_info, nullptr, &attachment.view));

	VkImageMemoryBarrier imgMemBarrier            = vkb::initializers::image_memory_barrier();
	imgMemBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
	imgMemBarrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
	imgMemBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	imgMemBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	imgMemBarrier.image                           = attachment.image;
	imgMemBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	imgMemBarrier.subresourceRange.baseArrayLayer = 0u;
	imgMemBarrier.subresourceRange.levelCount     = 1u;
	imgMemBarrier.subresourceRange.baseArrayLayer = 0u;
	imgMemBarrier.subresourceRange.layerCount     = 1u;
	imgMemBarrier.srcAccessMask                   = 0u;
	imgMemBarrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

	VkPipelineStageFlagBits srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlagBits dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

	VkCommandBuffer cmd = get_device().create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &imgMemBarrier);
	get_device().flush_command_buffer(cmd, queue, true);
}

uint32_t SubgroupsOperations::reverse(uint32_t i)
{
	uint32_t res = 0;
	for (int j = 0; j < log_2_N; j++)
	{
		res = (res << 1) + (i & 1);
		i >>= 1;
	}
	return res;
}

void SubgroupsOperations::create_butterfly_texture()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindngs = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2u)};
	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindngs);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &precompute.descriptor_set_layout));

	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &precompute.descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &precompute.descriptor_set));

	VkPipelineLayoutCreateInfo compute_pipeline_layout_info = {};
	compute_pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compute_pipeline_layout_info.setLayoutCount             = 1u;
	compute_pipeline_layout_info.pSetLayouts                = &precompute.descriptor_set_layout;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &compute_pipeline_layout_info, nullptr, &precompute.pipeline.pipeline_layout));

	VkComputePipelineCreateInfo computeInfo = {};
	computeInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computeInfo.layout                      = precompute.pipeline.pipeline_layout;
	computeInfo.stage                       = load_shader("subgroups_operations/butterfly_precomp.comp", VK_SHADER_STAGE_COMPUTE_BIT);

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1u, &computeInfo, nullptr, &precompute.pipeline.pipeline));

	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, log_2_N, DISPLACEMENT_MAP_DIM, butterfly_precomp);

	std::array<uint32_t, DISPLACEMENT_MAP_DIM> bit_reverse_arr;
	for (uint32_t i = 0; i < DISPLACEMENT_MAP_DIM; ++i)
		bit_reverse_arr[i] = reverse(i);

	bit_reverse_buffer = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(uint32_t) * bit_reverse_arr.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	bit_reverse_buffer->update(bit_reverse_arr.data(), sizeof(uint32_t) * bit_reverse_arr.size());

	VkDescriptorBufferInfo bit_reverse_descriptor = create_descriptor(*bit_reverse_buffer);
	bit_reverse_buffer->update(bit_reverse_arr.data(), sizeof(uint32_t) * bit_reverse_arr.size());

	VkDescriptorImageInfo image_descriptor = create_fb_descriptor(butterfly_precomp);
	VkDescriptorImageInfo  image_descriptor       = create_fb_descriptor(butterfly_precomp);
	VkDescriptorBufferInfo fft_params_ubo_buffer  = create_descriptor(*fft_params_ubo);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(precompute.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0u, &image_descriptor),
	    vkb::initializers::write_descriptor_set(precompute.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, &bit_reverse_descriptor),
	    vkb::initializers::write_descriptor_set(precompute.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u, &fft_params_ubo_buffer),
	};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0u, nullptr);
}

void SubgroupsOperations::create_fft()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindngs = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2u),
	};
	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindngs);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &fft.descriptor_set_layout));

	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &fft.descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &fft.descriptor_set_axis_y));
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &fft.descriptor_set_axis_x));
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &fft.descriptor_set_axis_z));

	VkPushConstantRange push_constant_range = vkb::initializers::push_constant_range(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int32_t), 0);

	VkPipelineLayoutCreateInfo compute_pipeline_layout_info = {};
	compute_pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compute_pipeline_layout_info.setLayoutCount             = 1u;
	compute_pipeline_layout_info.pSetLayouts                = &fft.descriptor_set_layout;
	compute_pipeline_layout_info.pushConstantRangeCount     = 1u;
	compute_pipeline_layout_info.pPushConstantRanges        = &push_constant_range;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &compute_pipeline_layout_info, nullptr, &fft.pipelines.horizontal.pipeline_layout));
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &compute_pipeline_layout_info, nullptr, &fft.pipelines.vertical.pipeline_layout));

	VkComputePipelineCreateInfo computeInfo = {};
	computeInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computeInfo.layout                      = fft.pipelines.horizontal.pipeline_layout;
	computeInfo.stage                       = load_shader("subgroups_operations/fft.comp", VK_SHADER_STAGE_COMPUTE_BIT);

	std::array<VkSpecializationMapEntry, 1> specialization_map_entries;
	VkSpecializationInfo                    spec_info;
	uint32_t                                direction = 0u;
	specialization_map_entries[0]                     = vkb::initializers::specialization_map_entry(0u, 0u, sizeof(uint32_t));
	spec_info                                         = vkb::initializers::specialization_info(static_cast<uint32_t>(specialization_map_entries.size()),
	                                                                                           specialization_map_entries.data(),
	                                                                                           sizeof(uint32_t),
	                                                                                           &direction);
	computeInfo.stage.pSpecializationInfo             = &spec_info;

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1u, &computeInfo, nullptr, &fft.pipelines.horizontal.pipeline));

	direction          = 1u;
	computeInfo.layout = fft.pipelines.vertical.pipeline_layout;

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1u, &computeInfo, nullptr, &fft.pipelines.vertical.pipeline));

	fft.tilde_axis_y = std::make_unique<FBAttachment>();
	fft.tilde_axis_x = std::make_unique<FBAttachment>();
	fft.tilde_axis_z = std::make_unique<FBAttachment>();
	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, grid_size, grid_size, *fft.tilde_axis_y);
	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, grid_size, grid_size, *fft.tilde_axis_x);
	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, grid_size, grid_size, *fft.tilde_axis_z);

	VkDescriptorImageInfo image_descriptor_battlefly    = create_fb_descriptor(butterfly_precomp);
	VkDescriptorImageInfo image_descriptor_tilda_y      = create_fb_descriptor(*fft_buffers.fft_tilde_h_kt_dy);
	VkDescriptorImageInfo image_descriptor_tilde_axis_y = create_fb_descriptor(*fft.tilde_axis_y);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets_asix_y = {
	    vkb::initializers::write_descriptor_set(fft.descriptor_set_axis_y, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0u, &image_descriptor_battlefly),
	    vkb::initializers::write_descriptor_set(fft.descriptor_set_axis_y, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, &image_descriptor_tilda_y),
	    vkb::initializers::write_descriptor_set(fft.descriptor_set_axis_y, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2u, &image_descriptor_tilde_axis_y),
	};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets_asix_y.size()), write_descriptor_sets_asix_y.data(), 0u, nullptr);

	VkDescriptorImageInfo image_descriptor_tilda_x      = create_fb_descriptor(*fft_buffers.fft_tilde_h_kt_dx);
	VkDescriptorImageInfo image_descriptor_tilde_axis_x = create_fb_descriptor(*fft.tilde_axis_x);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets_asix_x = {
	    vkb::initializers::write_descriptor_set(fft.descriptor_set_axis_x, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0u, &image_descriptor_battlefly),
	    vkb::initializers::write_descriptor_set(fft.descriptor_set_axis_x, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, &image_descriptor_tilda_x),
	    vkb::initializers::write_descriptor_set(fft.descriptor_set_axis_x, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2u, &image_descriptor_tilde_axis_x),
	};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets_asix_x.size()), write_descriptor_sets_asix_x.data(), 0u, nullptr);

	VkDescriptorImageInfo image_descriptor_tilda_z      = create_fb_descriptor(*fft_buffers.fft_tilde_h_kt_dz);
	VkDescriptorImageInfo image_descriptor_tilde_axis_z = create_fb_descriptor(*fft.tilde_axis_z);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets_asix_z = {
	    vkb::initializers::write_descriptor_set(fft.descriptor_set_axis_z, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0u, &image_descriptor_battlefly),
	    vkb::initializers::write_descriptor_set(fft.descriptor_set_axis_z, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, &image_descriptor_tilda_z),
	    vkb::initializers::write_descriptor_set(fft.descriptor_set_axis_z, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2u, &image_descriptor_tilde_axis_z),
	};

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets_asix_z.size()), write_descriptor_sets_asix_z.data(), 0u, nullptr);
}

void SubgroupsOperations::create_fft_inversion()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindngs = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 3u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 4u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 5u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 6u),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 7u)};

	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindngs);
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &fft_inversion.descriptor_set_layout));

	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &fft_inversion.descriptor_set_layout, 1u);
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &fft_inversion.descriptor_set));

	VkPipelineLayoutCreateInfo compute_pipeline_layout_info = {};
	compute_pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compute_pipeline_layout_info.setLayoutCount             = 1u;
	compute_pipeline_layout_info.pSetLayouts                = &fft_inversion.descriptor_set_layout;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &compute_pipeline_layout_info, nullptr, &fft_inversion.pipeline.pipeline_layout));

	VkComputePipelineCreateInfo computeInfo = {};
	computeInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computeInfo.layout                      = fft_inversion.pipeline.pipeline_layout;
	computeInfo.stage                       = load_shader("subgroups_operations/fft_invert.comp", VK_SHADER_STAGE_COMPUTE_BIT);

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1u, &computeInfo, nullptr, &fft_inversion.pipeline.pipeline));

	fft_buffers.fft_displacement = std::make_unique<FBAttachment>();

	createFBAttachement(VK_FORMAT_R32G32B32A32_SFLOAT, grid_size, grid_size, *fft_buffers.fft_displacement);

	VkDescriptorImageInfo image_descriptor_displacment_axis = create_fb_descriptor(*fft_buffers.fft_displacement);
	VkDescriptorImageInfo image_descriptor_pingpong0_axis_y = create_fb_descriptor(*fft_buffers.fft_tilde_h_kt_dy);
	VkDescriptorImageInfo image_descriptor_pingpong1_axis_y = create_fb_descriptor(*fft.tilde_axis_y);
	VkDescriptorImageInfo image_descriptor_pingpong0_axis_x = create_fb_descriptor(*fft_buffers.fft_tilde_h_kt_dx);
	VkDescriptorImageInfo image_descriptor_pingpong1_axis_x = create_fb_descriptor(*fft.tilde_axis_x);
	VkDescriptorImageInfo image_descriptor_pingpong0_axis_z = create_fb_descriptor(*fft_buffers.fft_tilde_h_kt_dz);
	VkDescriptorImageInfo image_descriptor_pingpong1_axis_z = create_fb_descriptor(*fft.tilde_axis_z);

	auto fft_page_descriptor = create_descriptor(*invert_fft_ubo);

	std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(fft_inversion.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0u, &image_descriptor_displacment_axis),
	    vkb::initializers::write_descriptor_set(fft_inversion.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, &image_descriptor_pingpong0_axis_y),
	    vkb::initializers::write_descriptor_set(fft_inversion.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2u, &image_descriptor_pingpong1_axis_y),
	    vkb::initializers::write_descriptor_set(fft_inversion.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3u, &image_descriptor_pingpong0_axis_x),
	    vkb::initializers::write_descriptor_set(fft_inversion.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4u, &image_descriptor_pingpong1_axis_x),
	    vkb::initializers::write_descriptor_set(fft_inversion.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 5u, &image_descriptor_pingpong0_axis_z),
	    vkb::initializers::write_descriptor_set(fft_inversion.descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6u, &image_descriptor_pingpong1_axis_z),
	    vkb::initializers::write_descriptor_set(fft_inversion.descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 7u, &fft_page_descriptor)};
	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0u, nullptr);
}

VkDescriptorImageInfo SubgroupsOperations::create_fb_descriptor(FBAttachment &attachment)
{
	VkDescriptorImageInfo image_descriptor{};
	image_descriptor.imageView   = attachment.view;
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	image_descriptor.sampler     = nullptr;
	return image_descriptor;
}