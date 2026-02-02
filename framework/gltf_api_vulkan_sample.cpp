/* Copyright (c) 2026, Arm Limited and Contributors
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

#include "gltf_api_vulkan_sample.h"

#include <glm/detail/func_common.hpp>
#include <string>

void GLTFApiVulkanSample::destroy_image(ApiVulkanSample::ImageData &image_data)
{
	VkDevice device_handle = get_device().get_handle();
	vkDestroyImageView(device_handle, image_data.view, nullptr);
	vkDestroyImage(device_handle, image_data.image, nullptr);
	vkFreeMemory(device_handle, image_data.mem, nullptr);
	image_data.view  = VK_NULL_HANDLE;
	image_data.image = VK_NULL_HANDLE;
	image_data.mem   = VK_NULL_HANDLE;
}

void GLTFApiVulkanSample::destroy_pipeline(GLTFApiVulkanSample::PipelineData &pipeline_data)
{
	VkDevice device_handle = get_device().get_handle();
	vkDestroyPipeline(device_handle, pipeline_data.pipeline, nullptr);
	vkDestroyPipelineLayout(device_handle, pipeline_data.pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(device_handle, pipeline_data.set_layout, nullptr);
	pipeline_data.pipeline        = VK_NULL_HANDLE;
	pipeline_data.pipeline_layout = VK_NULL_HANDLE;
	pipeline_data.set_layout      = VK_NULL_HANDLE;
}

GLTFApiVulkanSample::GLTFApiVulkanSample()
{
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
}

GLTFApiVulkanSample::~GLTFApiVulkanSample()
{
	if (has_device())
	{
		VkDevice device_handle = get_device().get_handle();
		vkDestroyDescriptorPool(device_handle, main_pass.descriptor_pool, nullptr);

		vkDestroyPipelineLayout(device_handle, present.pipeline.pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(device_handle, present.pipeline.set_layout, nullptr);
		present.pipeline.pipeline_layout = VK_NULL_HANDLE;
		present.pipeline.set_layout      = VK_NULL_HANDLE;

		vkDestroyPipelineLayout(device_handle, main_pass.sky_pipeline.pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(device_handle, main_pass.sky_pipeline.set_layout, nullptr);
		main_pass.sky_pipeline.pipeline_layout = VK_NULL_HANDLE;
		main_pass.sky_pipeline.set_layout      = VK_NULL_HANDLE;

		vkDestroyPipelineLayout(device_handle, main_pass.meshes.pipeline.pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(device_handle, main_pass.meshes.pipeline.set_layout, nullptr);
		main_pass.meshes.pipeline.pipeline_layout = VK_NULL_HANDLE;
		main_pass.meshes.pipeline.set_layout      = VK_NULL_HANDLE;

		destroy_image(main_pass.image);

		vkDestroySampler(device_handle, samplers.nearest, nullptr);
	}
}

void GLTFApiVulkanSample::setup_samplers()
{        // Samplers are not affected by settings.
	// Created once and re-used for all configurations.
	assert(samplers.nearest == VK_NULL_HANDLE);

	VkSamplerCreateInfo sampler_create_info{
	    .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	    .pNext                   = nullptr,
	    .flags                   = 0u,
	    .magFilter               = VK_FILTER_NEAREST,
	    .minFilter               = VK_FILTER_NEAREST,
	    .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST,
	    .addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .mipLodBias              = 0.0f,
	    .anisotropyEnable        = VK_FALSE,
	    .compareEnable           = VK_FALSE,
	    .minLod                  = 0.0f,
	    .maxLod                  = 0.0f,
	    .borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
	    .unnormalizedCoordinates = VK_FALSE,
	};

	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_create_info, nullptr, &samplers.nearest));
}

void GLTFApiVulkanSample::prepare_pipelines()
{}

bool GLTFApiVulkanSample::resize(const uint32_t _width, const uint32_t _height)
{
	if (!prepared)
	{
		return false;
	}

	get_render_context().handle_surface_changes();

	// Don't recreate the swapchain if the dimensions haven't changed.
	if (width == get_render_context().get_surface_extent().width && height == get_render_context().get_surface_extent().height)
	{
		return false;
	}

	width  = get_render_context().get_surface_extent().width;
	height = get_render_context().get_surface_extent().height;

	prepared = false;

	// Ensure all operations on the device have been finished before destroying resources.
	get_device().wait_idle();

	create_swapchain_buffers();

	reset_gpu_data();

	if ((width > 0u) && (height > 0u))
	{
		if (has_gui())
		{
			get_gui().resize(width, height);
		}
	}

	rebuild_command_buffers();

	get_device().wait_idle();

	// Notify derived class.
	view_changed();

	prepared = true;
	return true;
}

void GLTFApiVulkanSample::reset_gpu_data()
{
	setup_additional_descriptor_pool();
	prepare_pipelines();
}

void GLTFApiVulkanSample::load_assets(const std::string &scene_file)
{
	vkb::GLTFLoader loader{get_device()};
	sg_scene = loader.read_scene_from_file(scene_file);
	assert(sg_scene);

	camera.type              = vkb::CameraType::FirstPerson;
	const float aspect_ratio = 1.0f;        // dummy value. Reset by update_extents.
	camera.set_perspective(50.0f, aspect_ratio, 4000.0f, 1.0f);
	camera.set_rotation(glm::vec3(230.0f, 101.0f, -5.0f));
	camera.set_translation(glm::vec3(115.0f, -390.0f, 18.0f));
	camera.translation_speed = 100.0f;

	// Store all data from glTF scene nodes in a vector.
	for (auto &mesh : sg_scene->get_components<vkb::sg::Mesh>())
	{
		for (auto &node : mesh->get_nodes())
		{
			for (auto &submesh : mesh->get_submeshes())
			{
				const vkb::sg::Material *mesh_material = submesh->get_material();
				if (mesh_material)
				{
					bool        negative_scale   = glm::any(glm::lessThanEqual(node->get_transform().get_scale(), glm::vec3(0.0f)));
					const auto &color_texture_it = mesh_material->textures.find("base_color_texture");
					// Cull double-sided/transparent/negatively-scaled/non-textured meshes.
					if (!negative_scale &&
					    !mesh_material->double_sided &&
					    mesh_material->alpha_mode == vkb::sg::AlphaMode::Opaque &&
					    color_texture_it != mesh_material->textures.end())
					{
						SubmeshData &mesh_data = scene_data.emplace_back(std::move(SubmeshData{
						    .submesh            = submesh,
						    .world_matrix       = node->get_transform().get_world_matrix(),
						    .base_color_texture = color_texture_it->second}));
					}
					else
					{
						LOGI("Ignoring glTF mesh <{}>", submesh->get_name());
					}
				}
			}
		}
	}
	assert(!scene_data.empty());
}

void GLTFApiVulkanSample::setup_descriptor_pool_main_pass()
{
	assert(main_pass.descriptor_pool == VK_NULL_HANDLE);
	const uint32_t max_sets = vkb::to_u32(scene_data.size());

	std::array<VkDescriptorPoolSize, 2> pool_sizes =
	    {
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vkb::to_u32(scene_data.size())),
	        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, vkb::to_u32(scene_data.size())),
	    };

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        vkb::to_u32(pool_sizes.size()),
	        pool_sizes.data(), max_sets);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &main_pass.descriptor_pool));
	debug_utils.set_debug_name(get_device().get_handle(), VK_OBJECT_TYPE_DESCRIPTOR_POOL, get_object_handle(main_pass.descriptor_pool), "Main pass descriptor pool");
}

void GLTFApiVulkanSample::setup_additional_descriptor_pool()
{}

void GLTFApiVulkanSample::setup_descriptor_set_layout_main_pass()
{
	VkDevice                        device_handle = get_device().get_handle();
	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info;
	VkPipelineLayoutCreateInfo      pipeline_layout_create_info;

	// Main pass glTF-submesh.
	{
		std::array<VkDescriptorSetLayoutBinding, 2> set_layout_bindings =
		    {
		        // Binding 0 : Vertex shader uniform buffer.
		        vkb::initializers::descriptor_set_layout_binding(
		            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		            VK_SHADER_STAGE_VERTEX_BIT,
		            0),
		        // Binding 1 : Fragment shader combined sampler.
		        vkb::initializers::descriptor_set_layout_binding(
		            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		            VK_SHADER_STAGE_FRAGMENT_BIT,
		            1),
		    };

		descriptor_layout_create_info =
		    vkb::initializers::descriptor_set_layout_create_info(
		        set_layout_bindings.data(),
		        vkb::to_u32(set_layout_bindings.size()));

		pipeline_layout_create_info =
		    vkb::initializers::pipeline_layout_create_info(
		        &main_pass.meshes.pipeline.set_layout, 1);

		assert(main_pass.meshes.pipeline.set_layout == VK_NULL_HANDLE);
		VK_CHECK(vkCreateDescriptorSetLayout(device_handle, &descriptor_layout_create_info, nullptr, &main_pass.meshes.pipeline.set_layout));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
		                           get_object_handle(main_pass.meshes.pipeline.set_layout), "Submeshes Descriptor Set Layout");

		assert(main_pass.meshes.pipeline.pipeline_layout == VK_NULL_HANDLE);
		VK_CHECK(vkCreatePipelineLayout(device_handle, &pipeline_layout_create_info, nullptr, &main_pass.meshes.pipeline.pipeline_layout));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE_LAYOUT,
		                           get_object_handle(main_pass.meshes.pipeline.pipeline_layout), "Submeshes Pipeline Layout");
	}

	// Sky
	{
		descriptor_layout_create_info =
		    vkb::initializers::descriptor_set_layout_create_info(nullptr, 0);

		pipeline_layout_create_info =
		    vkb::initializers::pipeline_layout_create_info(
		        &main_pass.sky_pipeline.set_layout, 1);

		assert(main_pass.sky_pipeline.set_layout == VK_NULL_HANDLE);
		VK_CHECK(vkCreateDescriptorSetLayout(device_handle, &descriptor_layout_create_info, nullptr, &main_pass.sky_pipeline.set_layout));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
		                           get_object_handle(main_pass.sky_pipeline.set_layout), "Sky Descriptor Set Layout");

		assert(main_pass.sky_pipeline.pipeline_layout == VK_NULL_HANDLE);
		VK_CHECK(vkCreatePipelineLayout(device_handle, &pipeline_layout_create_info, nullptr, &main_pass.sky_pipeline.pipeline_layout));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE_LAYOUT,
		                           get_object_handle(main_pass.sky_pipeline.pipeline_layout), "Sky Pipeline Layout");
	}
}

void GLTFApiVulkanSample::setup_descriptor_set_layout_present()
{
	VkDescriptorSetLayoutCreateInfo             descriptor_layout_create_info;
	VkPipelineLayoutCreateInfo                  pipeline_layout_create_info;
	VkDevice                                    device_handle = get_device().get_handle();
	std::array<VkDescriptorSetLayoutBinding, 1> set_layout_bindings =
	    {
	        // Binding 0 : Fragment shader combined sampler
	        vkb::initializers::descriptor_set_layout_binding(
	            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	            VK_SHADER_STAGE_FRAGMENT_BIT,
	            0),
	    };

	descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        vkb::to_u32(set_layout_bindings.size()));

	pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &present.pipeline.set_layout, 1);

	set_layout_bindings[0].pImmutableSamplers = &samplers.nearest;

	vkDestroyDescriptorSetLayout(device_handle, present.pipeline.set_layout, nullptr);
	VK_CHECK(vkCreateDescriptorSetLayout(device_handle, &descriptor_layout_create_info, nullptr, &present.pipeline.set_layout));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, get_object_handle(present.pipeline.set_layout), "Present Descriptor Set Layout");

	vkDestroyPipelineLayout(device_handle, present.pipeline.pipeline_layout, nullptr);
	VK_CHECK(vkCreatePipelineLayout(device_handle, &pipeline_layout_create_info, nullptr, &present.pipeline.pipeline_layout));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_PIPELINE_LAYOUT, get_object_handle(present.pipeline.pipeline_layout), "Present Pipeline Layout");
}

void GLTFApiVulkanSample::setup_descriptor_set_main_pass()
{
	VkDevice device_handle = get_device().get_handle();
	assert(main_pass.meshes.descriptor_sets.empty());
	main_pass.meshes.descriptor_sets.resize(scene_data.size(), VK_NULL_HANDLE);
	for (uint32_t i = 0; i < scene_data.size(); ++i)
	{
		auto &mesh_data       = scene_data[i];
		auto &mesh_descriptor = main_pass.meshes.descriptor_sets[i];

		VkDescriptorSetAllocateInfo descriptor_set_alloc_info = vkb::initializers::descriptor_set_allocate_info(main_pass.descriptor_pool, &main_pass.meshes.pipeline.set_layout, 1);
		VK_CHECK(vkAllocateDescriptorSets(device_handle, &descriptor_set_alloc_info, &mesh_descriptor));

		std::string debug_name = fmt::format("Descriptor Set glTF submesh-{} <{}>", i, mesh_data.submesh->get_name());
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET, get_object_handle(mesh_descriptor), debug_name.c_str());

		VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*mesh_data.vertex_ubo);
		VkDescriptorImageInfo  image_descriptor  = vkb::initializers::descriptor_image_info(
            mesh_data.base_color_texture->get_sampler()->get_core_sampler().get_handle(),
            mesh_data.base_color_texture->get_image()->get_vk_image_view().get_handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		std::array<VkWriteDescriptorSet, 2> write_descriptor_sets =
		    {
		        vkb::initializers::write_descriptor_set(mesh_descriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &buffer_descriptor),              // Binding 0 : Vertex shader uniform buffer.
		        vkb::initializers::write_descriptor_set(mesh_descriptor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &image_descriptor)        // Binding 1 : Color map.
		    };
		vkUpdateDescriptorSets(device_handle, vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, NULL);
	}
}

void GLTFApiVulkanSample::setup_descriptor_set_present()
{
	VkDescriptorSetAllocateInfo descriptor_set_alloc_info;
	VkDevice                    device_handle = get_device().get_handle();
	descriptor_set_alloc_info                 = vkb::initializers::descriptor_set_allocate_info(
        descriptor_pool, &present.pipeline.set_layout, 1);
	VK_CHECK(vkAllocateDescriptorSets(device_handle, &descriptor_set_alloc_info, &present.set));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET, get_object_handle(present.set), "Descriptor set Present");

	VkDescriptorImageInfo image_descriptor = vkb::initializers::descriptor_image_info(
	    samplers.nearest,
	    main_pass.image.view,
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	std::array<VkWriteDescriptorSet, 1> write_descriptor_sets = {
	    vkb::initializers::write_descriptor_set(
	        present.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &image_descriptor)};
	vkUpdateDescriptorSets(device_handle, vkb::to_u32(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void GLTFApiVulkanSample::prepare_uniform_buffers_main_pass()
{
	// Create uniform buffers for each glTF-submesh.
	for (auto &mesh_data : scene_data)
	{
		mesh_data.vertex_ubo = std::make_unique<vkb::core::BufferC>(
		    get_device(), sizeof(UBOVS), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}

void GLTFApiVulkanSample::update_uniform_buffer(float delta_time)
{
	// Main pass glTF-submeshes UBO.
	{
		UBOVS ubo_vs{
		    .projection = camera.matrices.perspective};

		for (auto &mesh_data : scene_data)
		{
			ubo_vs.modelview = camera.matrices.view * mesh_data.world_matrix;
			mesh_data.vertex_ubo->convert_and_update(ubo_vs);
		}
	}
}

void GLTFApiVulkanSample::request_gpu_features(vkb::core::PhysicalDeviceC &gpu)
{
	VulkanSample::request_gpu_features(gpu);
	auto &requested_features = gpu.get_mutable_requested_features();

	// Enable anisotropic filtering if supported.
	if (gpu.get_features().samplerAnisotropy)
	{
		requested_features.samplerAnisotropy = VK_TRUE;
	}

	// Enable texture compression.
	if (gpu.get_features().textureCompressionBC)
	{
		requested_features.textureCompressionBC = VK_TRUE;
	}
	else if (gpu.get_features().textureCompressionASTC_LDR)
	{
		requested_features.textureCompressionASTC_LDR = VK_TRUE;
	}
	else if (gpu.get_features().textureCompressionETC2)
	{
		requested_features.textureCompressionETC2 = VK_TRUE;
	}
}

void GLTFApiVulkanSample::setup_render_pass()
{}

void GLTFApiVulkanSample::setup_framebuffer()
{}

void GLTFApiVulkanSample::update_extents()
{
	main_pass.extend = get_render_context().get_surface_extent();

	camera.update_aspect_ratio(static_cast<float>(main_pass.extend.width) / static_cast<float>(main_pass.extend.height));
}

void GLTFApiVulkanSample::setup_depth_stencil()
{
	destroy_image(depth_stencil);

	update_extents();

	// Create depth Stencil Image
	{
		VkImageCreateInfo image_create_info{
		    .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		    .flags     = 0u,
		    .imageType = VK_IMAGE_TYPE_2D,
		    .format    = depth_format,
		    .extent{main_pass.extend.width, main_pass.extend.height, 1},
		    .mipLevels   = 1,
		    .arrayLayers = 1,
		    .samples     = VK_SAMPLE_COUNT_1_BIT,
		    .tiling      = VK_IMAGE_TILING_OPTIMAL,
		    .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT};

		VkDevice device_handle = get_device().get_handle();
		VK_CHECK(vkCreateImage(device_handle, &image_create_info, nullptr, &depth_stencil.image));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE, get_object_handle(depth_stencil.image), "GBuffer Depth Image");

		VkMemoryRequirements memReqs{};
		vkGetImageMemoryRequirements(device_handle, depth_stencil.image, &memReqs);

		VkMemoryAllocateInfo memory_allocation{
		    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		    .allocationSize  = memReqs.size,
		    .memoryTypeIndex = get_device().get_gpu().get_memory_type(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
		VK_CHECK(vkAllocateMemory(device_handle, &memory_allocation, nullptr, &depth_stencil.mem));
		VK_CHECK(vkBindImageMemory(device_handle, depth_stencil.image, depth_stencil.mem, 0));

		VkImageViewCreateInfo image_view_create_info{
		    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		    .image    = depth_stencil.image,
		    .viewType = VK_IMAGE_VIEW_TYPE_2D,
		    .format   = depth_format,
		    .subresourceRange{
		        .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
		        .baseMipLevel   = 0,
		        .levelCount     = 1,
		        .baseArrayLayer = 0,
		        .layerCount     = 1}};
		// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
		if (depth_format >= VK_FORMAT_D16_UNORM_S8_UINT)
		{
			image_view_create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		VK_CHECK(vkCreateImageView(device_handle, &image_view_create_info, nullptr, &depth_stencil.view));
		debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE_VIEW, get_object_handle(depth_stencil.view), "GBuffer Depth Image view");
	}
}

void GLTFApiVulkanSample::setup_color()
{
	destroy_image(main_pass.image);

	VkImageCreateInfo image_create_info{
	    .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .flags     = 0u,
	    .imageType = VK_IMAGE_TYPE_2D,
	    .format    = get_render_context().get_format(),
	    .extent{main_pass.extend.width, main_pass.extend.height, 1},
	    .mipLevels   = 1,
	    .arrayLayers = 1,
	    .samples     = VK_SAMPLE_COUNT_1_BIT,
	    .tiling      = VK_IMAGE_TILING_OPTIMAL,
	    .usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT};

	VkDevice device_handle = get_device().get_handle();
	VK_CHECK(vkCreateImage(device_handle, &image_create_info, nullptr, &main_pass.image.image));

	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE, get_object_handle(main_pass.image.image), "Main pass color image");

	VkMemoryRequirements mem_reqs{};
	vkGetImageMemoryRequirements(device_handle, main_pass.image.image, &mem_reqs);

	VkMemoryAllocateInfo mem_alloc{
	    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .allocationSize  = mem_reqs.size,
	    .memoryTypeIndex = get_device().get_gpu().get_memory_type(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
	VK_CHECK(vkAllocateMemory(device_handle, &mem_alloc, nullptr, &main_pass.image.mem));
	VK_CHECK(vkBindImageMemory(device_handle, main_pass.image.image, main_pass.image.mem, 0));

	VkImageViewCreateInfo image_view_create_info{
	    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext            = nullptr,
	    .image            = main_pass.image.image,
	    .viewType         = VK_IMAGE_VIEW_TYPE_2D,
	    .format           = get_render_context().get_format(),
	    .components       = vkb::initializers::component_mapping(),
	    .subresourceRange = {
	        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
	        .baseMipLevel   = 0,
	        .levelCount     = 1,
	        .baseArrayLayer = 0,
	        .layerCount     = 1,
	    }};
	VK_CHECK(vkCreateImageView(device_handle, &image_view_create_info, nullptr, &main_pass.image.view));
	debug_utils.set_debug_name(device_handle, VK_OBJECT_TYPE_IMAGE_VIEW, get_object_handle(main_pass.image.view), "Main pass color image view");
}

bool GLTFApiVulkanSample::is_show_stats()
{
	return has_gui() && last_options.show_stats;
}

void GLTFApiVulkanSample::prepare_gui()
{
	vkb::CounterSamplingConfig config{
	    .mode  = vkb::CounterSamplingMode::Continuous,
	    .speed = 0.1f};
	get_stats().request_stats({
	                              vkb::StatIndex::frame_times,
	                              vkb::StatIndex::gpu_cycles,
	                          },
	                          config);

	create_gui(*window, &get_stats(), 15.0f, true);
	if (present.render_pass != nullptr)
	{
		get_gui().prepare(pipeline_cache, present.render_pass,
		                  {load_shader("uioverlay/uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
		                   load_shader("uioverlay/uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)});
	}
}

void GLTFApiVulkanSample::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (has_gui())
	{
		drawer.checkbox("Show stats", &current_options.show_stats);
		if (is_show_stats())
		{
			get_gui().show_stats(get_stats());
		}
	}
}
