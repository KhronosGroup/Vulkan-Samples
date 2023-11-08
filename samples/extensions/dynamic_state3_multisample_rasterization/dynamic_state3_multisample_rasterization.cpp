/* Copyright (c) 2023, Arm Limited and Contributors
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

#include "dynamic_state3_multisample_rasterization.h"

#include "common/vk_common.h"
#include "gltf_loader.h"
#include "gui.h"
#include "platform/filesystem.h"

#include <memory>

#include "scene_graph/components/mesh.h"
#include "scene_graph/components/material.h"

#include "rendering/postprocessing_renderpass.h"
#include "rendering/subpasses/forward_subpass.h"
#include "stats/stats.h"

namespace
{
template <typename T>
struct CopyBuffer
{
	std::vector<T> operator()(std::unordered_map<std::string, vkb::core::Buffer> &buffers, const char *bufferName)
	{
		auto iter = buffers.find(bufferName);
		if (iter == buffers.cend())
		{
			return {};
		}
		auto          &buffer = iter->second;
		std::vector<T> out;

		const size_t sz = buffer.get_size();
		out.resize(sz / sizeof(T));
		const bool alreadyMapped = buffer.get_data() != nullptr;
		if (!alreadyMapped)
		{
			buffer.map();
		}
		memcpy(&out[0], buffer.get_data(), sz);
		if (!alreadyMapped)
		{
			buffer.unmap();
		}
		return out;
	}
};

}        // namespace

namespace
{
const std::string to_string(VkSampleCountFlagBits count)
{
	switch (count)
	{
		case VK_SAMPLE_COUNT_1_BIT:
			return "No MSAA";
		case VK_SAMPLE_COUNT_2_BIT:
			return "2X MSAA";
		case VK_SAMPLE_COUNT_4_BIT:
			return "4X MSAA";
		case VK_SAMPLE_COUNT_8_BIT:
			return "8X MSAA";
		case VK_SAMPLE_COUNT_16_BIT:
			return "16X MSAA";
		case VK_SAMPLE_COUNT_32_BIT:
			return "32X MSAA";
		case VK_SAMPLE_COUNT_64_BIT:
			return "64X MSAA";
		default:
			return "Unknown";
	}
}

const std::string to_string(VkResolveModeFlagBits mode)
{
	switch (mode)
	{
		case VK_RESOLVE_MODE_NONE:
			return "None";
		case VK_RESOLVE_MODE_SAMPLE_ZERO_BIT:
			return "Sample 0";
		case VK_RESOLVE_MODE_AVERAGE_BIT:
			return "Average";
		case VK_RESOLVE_MODE_MIN_BIT:
			return "Min";
		case VK_RESOLVE_MODE_MAX_BIT:
			return "Max";
		default:
			return "Unknown";
	}
}
}        // namespace

DynamicState3MultisampleRasterization::DynamicState3MultisampleRasterization()
{
	// Extension of interest in this sample (optional)
	add_device_extension(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, true);

	// Extension dependency requirements (given that instance API version is 1.0.0)
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true);
	add_device_extension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, true);
	add_device_extension(VK_KHR_MAINTENANCE2_EXTENSION_NAME, true);
	add_device_extension(VK_KHR_MULTIVIEW_EXTENSION_NAME, true);

	auto &config = get_configuration();

	// MSAA will be enabled by default if supported
	// Batch mode will test the toggle between 1 or 2 renderpasses
	// with writeback resolve of color and depth
	config.insert<vkb::BoolSetting>(0, gui_run_postprocessing, false);
	config.insert<vkb::BoolSetting>(1, gui_run_postprocessing, true);
}

DynamicState3MultisampleRasterization::~DynamicState3MultisampleRasterization()
{
	if (device)
	{
		vkDestroyPipeline(get_device().get_handle(), sample_pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), sample_pipeline_layout, nullptr);
	}
}

void DynamicState3MultisampleRasterization::load_scene()
{
	assert(!!device);
	vkb::GLTFLoader   loader{*device};
	const std::string scene_path = "scenes/space_module/";
	//auto              scene      = loader.read_scene_from_file(scene_path + "vokselia.gltf");
	scene = loader.read_scene_from_file(scene_path + "SpaceModule.gltf");

	//const std::string scene_path = "scenes/";
	//scene = loader.read_scene_from_file(scene_path + "torusknot.gltf");

	assert(!!scene);

	//auto materials = scene->get_components<vkb::sg::Material>();
	//auto textures = scene->get_components<vkb::sg::Texture>();

	for (auto &&texture : scene->get_components<vkb::sg::Texture>())
	{
		//const auto name = texture->get_name();
		//std::cout << name << std::endl;
		const auto &short_name = texture->get_name();
		std::cout << short_name << std::endl;
		//auto        image_name = scene_path + short_name + ".ktx";
		//auto image_name = scene_path + short_name;
		//auto image      = vkb::sg::Image::load(image_name, image_name, vkb::sg::Image::Color);

		// hack:
		//image-> VK_FORMAT_R8G8B8A8_SRGB

		//image->create_vk_image(*device);
		//Texture texture;
		//texture.n_mip_maps = static_cast<uint32_t>(image->get_mipmaps().size());
		//texture.image      = const_cast<std::unique_ptr<vkb::core::Image>>(image->get_vk_image());
		//texture.image_view = const_cast<vkb::core::Image>(image->get_vk_image_view());
		//assert(texture.n_mip_maps == 1);
		/*
		texture.image = std::make_unique<vkb::core::Image>(*device,
		                                                   image->get_extent(),
		                                                   image->get_format(),
		                                                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		                                                   VMA_MEMORY_USAGE_GPU_ONLY,
		                                                   VK_SAMPLE_COUNT_1_BIT,
		                                                   1,
		                                                   1,
		                                                   VK_IMAGE_TILING_OPTIMAL,

		                                                   0);
		
		const auto &data        = texture->get_image()->get_data();
		auto        data_buffer = std::make_unique<vkb::core::Buffer>(*device, data.size() * sizeof(data[0]), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT, queue_families);
		data_buffer->update(data.data(), data.size() * sizeof(data[0]), 0);
		data_buffer->flush();

		auto &texture_cmd = device->get_command_pool().request_command_buffer();
		texture_cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);

		VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		subresource_range.baseMipLevel            = 0;
		subresource_range.levelCount              = texture->get_image()->get_mipmaps().size();

		vkb::image_layout_transition(
		    texture_cmd.get_handle(), texture->get_image()->get_vk_image().get_handle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

		auto              offsets              = texture->get_image()->get_offsets();
		VkBufferImageCopy region               = {};
		region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel       = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount     = 1;
		region.imageExtent                     = texture->get_image()->get_extent();
		region.bufferOffset                    = offsets[0][0];

		texture_cmd.copy_buffer_to_image(*data_buffer, texture->get_image()->get_vk_image(), {region});
		texture_cmd.end();

		auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
		queue.submit(texture_cmd, device->request_fence());
		device->get_fence_pool().wait();
		device->get_fence_pool().reset();

		//texture.image_view = std::make_unique<vkb::core::ImageView>(*texture.image, VK_IMAGE_VIEW_TYPE_2D);
		*/
		VkDescriptorImageInfo image_descriptor;
		image_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_descriptor.imageView   = texture->get_image()->get_vk_image_view().get_handle();
		image_descriptor.sampler     = sampler;
		image_descriptors.push_back(image_descriptor);
		//textures.emplace_back(std::move(texture));
	}

	/*
	for (auto && material : scene->get_components<vkb::sg::Material>())
	{
		const auto name = material->get_name();
		std::cout << name << std::endl;
	}
	*/

	for (auto &&mesh : scene->get_components<vkb::sg::Mesh>())
	{
		/*
		const auto  &short_name    = mesh->get_name();
		auto         image_name    = scene_path + short_name + ".ktx";
		auto         image         = vkb::sg::Image::load(image_name, image_name, vkb::sg::Image::Color);
		
		image->create_vk_image(*device);
		Texture texture;
		texture.n_mip_maps = static_cast<uint32_t>(image->get_mipmaps().size());
		assert(texture.n_mip_maps == 1);
		texture.image = std::make_unique<vkb::core::Image>(*device,
		                                                   image->get_extent(),
		                                                   image->get_format(),
		                                                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		                                                   VMA_MEMORY_USAGE_GPU_ONLY,
		                                                   VK_SAMPLE_COUNT_1_BIT,
		                                                   1,
		                                                   1,
		                                                   VK_IMAGE_TILING_OPTIMAL,
		
                                                   0);		
		
		const auto &data        = image->get_data();
		auto        data_buffer = std::make_unique<vkb::core::Buffer>(*device, data.size() * sizeof(data[0]), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT, queue_families);
		data_buffer->update(data.data(), data.size() * sizeof(data[0]), 0);
		data_buffer->flush();


		auto &texture_cmd = device->get_command_pool().request_command_buffer();
		texture_cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);

		VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		subresource_range.baseMipLevel            = 0;
		subresource_range.levelCount              = texture.n_mip_maps;

		vkb::image_layout_transition(
		    texture_cmd.get_handle(), texture.image->get_handle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

		auto              offsets              = image->get_offsets();
		VkBufferImageCopy region               = {};
		region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel       = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount     = 1;
		region.imageExtent                     = image->get_extent();
		region.bufferOffset                    = offsets[0][0];

		texture_cmd.copy_buffer_to_image(*data_buffer, *texture.image, {region});
		texture_cmd.end();

		auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
		queue.submit(texture_cmd, device->request_fence());
		device->get_fence_pool().wait();
		device->get_fence_pool().reset();

		texture.image_view = std::make_unique<vkb::core::ImageView>(*texture.image, VK_IMAGE_VIEW_TYPE_2D);

		VkDescriptorImageInfo image_descriptor;
		image_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_descriptor.imageView   = texture.image_view->get_handle();
		image_descriptor.sampler     = sampler;
		image_descriptors.push_back(image_descriptor);
		textures.emplace_back(std::move(texture));
		*/
		for (auto &&sub_mesh : mesh->get_submeshes())
		{
			SceneModel model;
			// PJ: Fix this!!!!
			//auto       texs = sub_mesh->get_material()->textures;
			//auto       first_texture = sub_mesh->get_material()->textures.at(0);
			model.texture_index      = 1;

			auto pts = CopyBuffer<glm::vec3>{}(sub_mesh->vertex_buffers, "position");
			auto uvs = CopyBuffer<glm::vec2>{}(sub_mesh->vertex_buffers, "texcoord_0");
			assert(uvs.size() == pts.size());

			model.vertices.resize(pts.size());
			for (size_t i = 0; i < pts.size(); ++i)
			{
				model.vertices[i].pt = {pts[i].x, -pts[i].y, pts[i].z};
				model.vertices[i].uv = uvs[i];
			}
			
			assert(sub_mesh->index_type == VK_INDEX_TYPE_UINT16);
			auto buffer = sub_mesh->index_buffer.get();
			if (buffer)
			{
				const size_t sz         = buffer->get_size();
				const size_t nTriangles = sz / sizeof(uint16_t) / 3;
				model.triangles.resize(nTriangles);
				auto ptr = buffer->get_data();
				assert(!!ptr);
				std::vector<uint16_t> temp_buffer(nTriangles * 3);
				memcpy(temp_buffer.data(), ptr, nTriangles * 3 * sizeof(temp_buffer[0]));
				model.triangles.resize(nTriangles);
				for (size_t i = 0; i < nTriangles; ++i)
				{
					model.triangles[i] = {
					    static_cast<uint16_t>(temp_buffer[3 * i]),
					    static_cast<uint16_t>(temp_buffer[3 * i + 1]),
					    static_cast<uint16_t>(temp_buffer[3 * i + 2])};
				}
			}
			//model.bounding_sphere = BoundingSphere(pts);
			models.emplace_back(std::move(model));
		}
	}
	
	auto textures = scene->get_components<vkb::sg::Texture>();

	std::vector<std::pair<VkImage, VkImageSubresourceRange>> imagesAndRanges;
	// static_cast<uint32_t>(scene->get_components<vkb::sg::Texture>().size())
	imagesAndRanges.reserve(textures.size());
	for (auto const &texture : textures)
	{
		imagesAndRanges.emplace_back(
		    std::make_pair(texture->get_image()->get_vk_image().get_handle(), VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, static_cast<unsigned int>(texture->get_image()->get_mipmaps().size()), 0, 1}));
	}
	
	auto &cmd = device->get_command_pool().request_command_buffer();
	cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
	vkb::image_layout_transition(cmd.get_handle(), imagesAndRanges, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	cmd.end();
	auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
	queue.submit(cmd, device->request_fence());
	device->get_fence_pool().wait();
}

void DynamicState3MultisampleRasterization::initialize_resources()
{
	size_t       vertex_buffer_size = 0, index_buffer_size = 0;
	const size_t model_buffer_size = models.size() * sizeof(SceneModel);
	for (auto &&model : models)
	{
		model.vertex_buffer_offset = vertex_buffer_size;
		model.index_buffer_offset  = index_buffer_size;

		vertex_buffer_size += model.vertices.size() * sizeof(Vertex);
		index_buffer_size += model.triangles.size() * sizeof(model.triangles[0]);
	}

	assert(vertex_buffer_size && index_buffer_size && model_buffer_size);
	auto staging_vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(), vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	auto staging_index_buffer  = std::make_unique<vkb::core::Buffer>(get_device(), index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	auto staging_model_buffer  = std::make_unique<vkb::core::Buffer>(get_device(), model_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// We will store the GPU commands in the indirect call buffer
	constexpr auto default_indirect_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	auto           indirect_flags         = default_indirect_flags;
	/*
	if (m_supports_buffer_device)
	{
		indirect_flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}
	*/
	indirect_call_buffer = std::make_unique<vkb::core::Buffer>(get_device(), models.size() * sizeof(VkDrawIndexedIndirectCommand), indirect_flags, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, queue_families);

	// Create a buffer containing the addresses of the indirect calls.
	// In this sample, the order of the addresses will match that of the other buffers, but in general they could be in any order
	const size_t address_buffer_size    = sizeof(VkDeviceAddress);
	auto         staging_address_buffer = std::make_unique<vkb::core::Buffer>(get_device(), address_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	/*
	if (m_supports_buffer_device)
	{
		auto *destPtr = (uint64_t *) staging_address_buffer->get_data();

		VkBufferDeviceAddressInfoKHR address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
		address_info.buffer    = indirect_call_buffer->get_handle();
		VkDeviceAddress srcPtr = vkGetBufferDeviceAddressKHR(device->get_handle(), &address_info);

		*destPtr = srcPtr;
	}
	*/

	for (size_t i = 0; i < models.size(); ++i)
	{
		auto &model = models[i];
		staging_vertex_buffer->update(model.vertices.data(), model.vertices.size() * sizeof(Vertex), model.vertex_buffer_offset);
		staging_index_buffer->update(model.triangles.data(), model.triangles.size() * sizeof(model.triangles[0]), model.index_buffer_offset);

		GpuModelInformation model_information;
		//model_information.bounding_sphere_center = model.bounding_sphere.center;
		//model_information.bounding_sphere_radius = model.bounding_sphere.radius;
		model_information.texture_index          = model.texture_index;
		model_information.firstIndex             = model.index_buffer_offset / (sizeof(model.triangles[0][0]));
		model_information.indexCount             = static_cast<uint32_t>(model.triangles.size());
		staging_model_buffer->update(&model_information, sizeof(GpuModelInformation), i * sizeof(GpuModelInformation));
	}

	staging_vertex_buffer->flush();
	staging_index_buffer->flush();
	staging_model_buffer->flush();

	auto &cmd = device->request_command_buffer();
	cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
	auto copy = [this, &cmd](vkb::core::Buffer &staging_buffer, VkBufferUsageFlags buffer_usage_flags) {
		auto output_buffer = std::make_unique<vkb::core::Buffer>(get_device(), staging_buffer.get_size(), buffer_usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, queue_families);
		cmd.copy_buffer(staging_buffer, *output_buffer, staging_buffer.get_size());

		vkb::BufferMemoryBarrier barrier;
		barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		cmd.buffer_memory_barrier(*output_buffer, 0, VK_WHOLE_SIZE, barrier);
		return output_buffer;
	};
	vertex_buffer            = copy(*staging_vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	index_buffer             = copy(*staging_index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	model_information_buffer = copy(*staging_model_buffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	/*
	if (m_supports_buffer_device)
	{
		// In this sample, we use a staging buffer for the device address buffer (i.e. for device exclusive memory).
		// However, since the size of each element (sizeof(uint64_t)) is smaller than the objects it's pointing to, it could instead use host-visible memory
		// for fast referencing of the underlying data
		device_address_buffer = copy(*staging_address_buffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
	}
	*/

	cmd.end();
	auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
	queue.submit(cmd, device->request_fence());
	device->get_fence_pool().wait();
}

void DynamicState3MultisampleRasterization::update_scene_uniform()
{
	if (!scene_uniform_buffer)
	{
		scene_uniform_buffer = std::make_unique<vkb::core::Buffer>(*device, sizeof(SceneUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT, queue_families);
	}
	scene_uniform.proj        = camera.matrices.perspective;
	scene_uniform.view        = camera.matrices.view;
	scene_uniform.proj_view   = scene_uniform.proj * scene_uniform.view;
	scene_uniform.model_count = static_cast<uint32_t>(models.size());

	scene_uniform_buffer->update(&scene_uniform, sizeof(scene_uniform), 0);

	scene_uniform_buffer->flush();
}

void DynamicState3MultisampleRasterization::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void DynamicState3MultisampleRasterization::cpu_cull()
{
	cpu_commands.resize(models.size());

	//VisibilityTester tester(scene_uniform.proj * scene_uniform.view);
	for (size_t i = 0; i < models.size(); ++i)
	{
		// we control visibility by changing the instance count
		auto                        &model = models[i];
		VkDrawIndexedIndirectCommand cmd{};
		cmd.firstIndex    = model.index_buffer_offset / (sizeof(model.triangles[0][0]));
		cmd.indexCount    = static_cast<uint32_t>(model.triangles.size()) * 3;
		cmd.vertexOffset  = static_cast<int32_t>(model.vertex_buffer_offset / sizeof(Vertex));
		cmd.firstInstance = i;
		cmd.instanceCount = models.size(); // tester.is_visible(model.bounding_sphere.center, model.bounding_sphere.radius);
		cpu_commands[i]   = cmd;
	}

	const auto call_buffer_size = cpu_commands.size() * sizeof(cpu_commands[0]);
	assert(!!indirect_call_buffer && indirect_call_buffer->get_size() == call_buffer_size);

	if (!cpu_staging_buffer || cpu_staging_buffer->get_size() != call_buffer_size)
	{
		cpu_staging_buffer = std::make_unique<vkb::core::Buffer>(get_device(), models.size() * sizeof(VkDrawIndexedIndirectCommand), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	cpu_staging_buffer->update(cpu_commands.data(), call_buffer_size, 0);
	cpu_staging_buffer->flush();

	auto &transfer_cmd = device->get_command_pool().request_command_buffer();
	transfer_cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
	transfer_cmd.copy_buffer(*cpu_staging_buffer, *indirect_call_buffer, call_buffer_size);
	transfer_cmd.end();

	auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
	queue.submit(transfer_cmd, device->request_fence());
	device->get_fence_pool().wait();
}


bool DynamicState3MultisampleRasterization::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.001f, 512.0f);
	//camera.set_rotation(glm::vec3(-23.5, -45, 0));
	//camera.set_translation(glm::vec3(0, 0.5, -0.2));
	camera.set_translation(glm::vec3(0, 5, -500));

	create_sampler();
	load_scene();
	initialize_resources();

	update_scene_uniform();

	prepare_pipelines();

	initialize_descriptors();

	build_command_buffers();

	/*
	prepare_supported_sample_count_list();

	depth_writeback_resolve_supported = device->is_enabled(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
	if (depth_writeback_resolve_supported)
	{
		prepare_depth_resolve_mode_list();
	}
	*/

	cpu_cull();

	/*
	load_scene("scenes/space_module/SpaceModule.gltf");

	auto &camera_node = vkb::add_free_camera(*scene, "main_camera", get_render_context().get_surface_extent());
	camera            = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

	vkb::ShaderSource scene_vs("base.vert");
	vkb::ShaderSource scene_fs("base.frag");
	//auto              scene_subpass = std::make_unique<vkb::ForwardSubpass>(get_render_context(), std::move(scene_vs), std::move(scene_fs), *scene, *camera);
	//scene_pipeline                  = std::make_unique<vkb::RenderPipeline>();
	//scene_pipeline->add_subpass(std::move(scene_subpass));
	vkb::ShaderSource postprocessing_vs("postprocessing/postprocessing.vert");
	postprocessing_pipeline = std::make_unique<vkb::PostProcessingPipeline>(get_render_context(), std::move(postprocessing_vs));
	postprocessing_pipeline->add_pass().add_subpass(vkb::ShaderSource("postprocessing/outline.frag"));

	//update_pipelines();

	stats->request_stats({vkb::StatIndex::frame_times,
	                      vkb::StatIndex::gpu_ext_read_bytes,
	                      vkb::StatIndex::gpu_ext_write_bytes});

	//gui = std::make_unique<vkb::Gui>(*this, *window, stats.get());
	*/

	prepared = true;
	return true;
}
/*
void DynamicState3MultisampleRasterization::prepare_render_context()
{
	get_render_context().prepare(1, std::bind(&DynamicState3MultisampleRasterization::create_render_target, this, std::placeholders::_1));
}

std::unique_ptr<vkb::RenderTarget> DynamicState3MultisampleRasterization::create_render_target(vkb::core::Image &&swapchain_image)
{
	auto &device = swapchain_image.get_device();
	auto &extent = swapchain_image.get_extent();

	auto              depth_format        = vkb::get_suitable_depth_format(device.get_gpu().get_handle());
	bool              msaa_enabled        = sample_count != VK_SAMPLE_COUNT_1_BIT;
	VkImageUsageFlags depth_usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	VkImageUsageFlags depth_resolve_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (run_postprocessing)
	{
		// Depth needs to be read by the postprocessing subpass
		if (msaa_enabled && depth_writeback_resolve_supported && resolve_depth_on_writeback)
		{
			// Depth is resolved
			depth_usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
			depth_resolve_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		else
		{
			// Postprocessing reads multisampled depth
			depth_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			depth_resolve_usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		}
	}
	else
	{
		// Depth attachments are transient
		depth_usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		depth_resolve_usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}

	vkb::core::Image depth_image{device,
	                             extent,
	                             depth_format,
	                             depth_usage,
	                             VMA_MEMORY_USAGE_GPU_ONLY,
	                             sample_count};

	vkb::core::Image depth_resolve_image{device,
	                                     extent,
	                                     depth_format,
	                                     depth_resolve_usage,
	                                     VMA_MEMORY_USAGE_GPU_ONLY,
	                                     VK_SAMPLE_COUNT_1_BIT};

	VkImageUsageFlags color_ms_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (ColorResolve::OnWriteback == color_resolve_method)
	{
		// Writeback resolve means that the multisampled attachment
		// can be discarded at the end of the render pass
		color_ms_usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}
	else if (ColorResolve::SeparatePass == color_resolve_method)
	{
		// Multisampled attachment will be stored and
		// resolved outside the render pass
		color_ms_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	vkb::core::Image color_ms_image{device,
	                                extent,
	                                swapchain_image.get_format(),
	                                color_ms_usage,
	                                VMA_MEMORY_USAGE_GPU_ONLY,
	                                sample_count};

	VkImageUsageFlags color_resolve_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (run_postprocessing)
	{
		if (ColorResolve::SeparatePass == color_resolve_method)
		{
			// The multisampled color image will be resolved
			// to this attachment with a transfer operation
			color_resolve_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		// The resolved color image will be read by the postprocessing
		// renderpass
		color_resolve_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	vkb::core::Image color_resolve_image{device,
	                                     extent,
	                                     swapchain_image.get_format(),
	                                     color_resolve_usage,
	                                     VMA_MEMORY_USAGE_GPU_ONLY,
	                                     VK_SAMPLE_COUNT_1_BIT};

	scene_load_store.clear();
	std::vector<vkb::core::Image> images;

	// Attachment 0 - Swapchain
	// Used by the scene renderpass if postprocessing is disabled
	// Used by the postprocessing renderpass if postprocessing is enabled
	i_swapchain = 0;
	images.push_back(std::move(swapchain_image));
	scene_load_store.push_back({VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE});

	// Attachment 1 - Depth
	// Always used by the scene renderpass, may or may not be multisampled
	i_depth = 1;
	images.push_back(std::move(depth_image));
	scene_load_store.push_back({VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE});

	// Attachment 2 - Multisampled color
	// Used by the scene renderpass if MSAA is enabled
	i_color_ms = 2;
	images.push_back(std::move(color_ms_image));
	scene_load_store.push_back({VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE});

	// Attachment 3 - Resolved color
	// Used as an output by the scene renderpass if MSAA and postprocessing are enabled
	// Used as an input by the postprocessing renderpass
	i_color_resolve = 3;
	images.push_back(std::move(color_resolve_image));
	scene_load_store.push_back({VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE});

	// Attachment 4 - Resolved depth
	// Used for writeback depth resolve if MSAA is enabled and the required extension is supported
	i_depth_resolve = 4;
	images.push_back(std::move(depth_resolve_image));
	scene_load_store.push_back({VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE});

	color_atts = {i_swapchain, i_color_ms, i_color_resolve};
	depth_atts = {i_depth, i_depth_resolve};

	return std::make_unique<vkb::RenderTarget>(std::move(images));
}

/*
void DynamicState3MultisampleRasterization::update(float delta_time)
{
	if ((gui_run_postprocessing != last_gui_run_postprocessing) ||
	    (gui_sample_count != last_gui_sample_count) ||
	    (gui_color_resolve_method != last_gui_color_resolve_method) ||
	    (gui_resolve_depth_on_writeback != last_gui_resolve_depth_on_writeback) ||
	    (gui_depth_resolve_mode != last_gui_depth_resolve_mode))
	{
		run_postprocessing         = gui_run_postprocessing;
		sample_count               = gui_sample_count;
		color_resolve_method       = gui_color_resolve_method;
		resolve_depth_on_writeback = gui_resolve_depth_on_writeback;
		depth_resolve_mode         = gui_depth_resolve_mode;

		update_pipelines();

		last_gui_run_postprocessing         = gui_run_postprocessing;
		last_gui_sample_count               = gui_sample_count;
		last_gui_color_resolve_method       = gui_color_resolve_method;
		last_gui_resolve_depth_on_writeback = gui_resolve_depth_on_writeback;
		last_gui_depth_resolve_mode         = gui_depth_resolve_mode;
	}

	VulkanSample::update(delta_time);
}


void DynamicState3MultisampleRasterization::update_pipelines()
{
	bool msaa_enabled = sample_count != VK_SAMPLE_COUNT_1_BIT;
	if (run_postprocessing)
	{
		update_for_scene_and_postprocessing(msaa_enabled);
	}
	else
	{
		update_for_scene_only(msaa_enabled);
	}

	// Default swapchain usage flags
	std::set<VkImageUsageFlagBits> swapchain_usage = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT};
	if (ColorResolve::SeparatePass == color_resolve_method && !run_postprocessing)
	{
		// The multisampled color image will be resolved
		// to the swapchain with a transfer operation
		swapchain_usage.insert(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	}
	
	get_device().wait_idle();

	get_render_context().update_swapchain(swapchain_usage);
}

void DynamicState3MultisampleRasterization::update_for_scene_only(bool msaa_enabled)
{
	auto &scene_subpass = scene_pipeline->get_active_subpass();
	scene_subpass->set_sample_count(sample_count);

	if (msaa_enabled)
	{
		// Render multisampled color, to be resolved to the swapchain
		use_multisampled_color(scene_subpass, scene_load_store, i_swapchain);
	}
	else
	{
		// Render color to the swapchain
		use_singlesampled_color(scene_subpass, scene_load_store, i_swapchain);
	}

	// Depth attachment is transient, it will not be needed after the renderpass
	// If it is multisampled, there is no need to resolve it
	scene_load_store[i_depth].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	disable_depth_writeback_resolve(scene_subpass, scene_load_store);

	// Auxiliary single-sampled color attachment is not used
	scene_load_store[i_color_resolve].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Update the scene renderpass
	scene_pipeline->set_load_store(scene_load_store);
}

void DynamicState3MultisampleRasterization::update_for_scene_and_postprocessing(bool msaa_enabled)
{
	auto &scene_subpass = scene_pipeline->get_active_subpass();
	scene_subpass->set_sample_count(sample_count);

	// The color and depth attachments will be the input of the postprocessing renderpass
	if (msaa_enabled)
	{
		// Resolve multisampled color to an intermediate attachment
		use_multisampled_color(scene_subpass, scene_load_store, i_color_resolve);

		// Store multisampled depth
		// Resolve it first if enabled and supported,
		store_multisampled_depth(scene_subpass, scene_load_store);
	}
	else
	{
		// Render color to an intermediate attachment
		use_singlesampled_color(scene_subpass, scene_load_store, i_color_resolve);

		// Store single-sampled depth
		scene_load_store[i_depth].store_op = VK_ATTACHMENT_STORE_OP_STORE;
		disable_depth_writeback_resolve(scene_subpass, scene_load_store);
	}

	// Swapchain is not used in the scene renderpass
	scene_load_store[i_swapchain].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Update the scene renderpass
	scene_pipeline->set_load_store(scene_load_store);
}

void DynamicState3MultisampleRasterization::use_multisampled_color(std::unique_ptr<vkb::Subpass> &subpass, std::vector<vkb::LoadStoreInfo> &load_store, uint32_t resolve_attachment)
{
	// Render to multisampled color attachment
	subpass->set_output_attachments({i_color_ms});

	// Resolve color
	if (ColorResolve::OnWriteback == color_resolve_method)
	{
		// Good practice
		// Multisampled attachment is transient
		// This allows tilers to completely avoid writing out the multisampled attachment to memory,
		// a considerable performance and bandwidth improvement
		load_store[i_color_ms].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		// Enable writeback resolve to single-sampled attachment
		subpass->set_color_resolve_attachments({resolve_attachment});

		// Save resolved attachment
		load_store[resolve_attachment].store_op = VK_ATTACHMENT_STORE_OP_STORE;
	}
	else if (ColorResolve::SeparatePass == color_resolve_method)
	{
		// Bad practice
		// Save multisampled color attachment, will be resolved outside the renderpass
		// Storing multisampled color should be avoided
		load_store[i_color_ms].store_op = VK_ATTACHMENT_STORE_OP_STORE;

		// Disable writeback resolve
		subpass->set_color_resolve_attachments({});
		load_store[resolve_attachment].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
}

void DynamicState3MultisampleRasterization::use_singlesampled_color(std::unique_ptr<vkb::Subpass> &subpass, std::vector<vkb::LoadStoreInfo> &load_store, uint32_t output_attachment)
{
	// Render to a single-sampled attachment
	subpass->set_output_attachments({output_attachment});
	load_store[output_attachment].store_op = VK_ATTACHMENT_STORE_OP_STORE;

	// Multisampled color attachment is not used
	load_store[i_color_ms].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Disable writeback resolve
	subpass->set_color_resolve_attachments({});
}

void DynamicState3MultisampleRasterization::store_multisampled_depth(std::unique_ptr<vkb::Subpass> &subpass, std::vector<vkb::LoadStoreInfo> &load_store)
{
	if (depth_writeback_resolve_supported && resolve_depth_on_writeback)
	{
		// Good practice
		// Multisampled attachment is transient
		// This allows tilers to completely avoid writing out the multisampled attachment to memory,
		// a considerable performance and bandwidth improvement
		load_store[i_depth].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		// Enable writeback resolve to single-sampled attachment
		subpass->set_depth_stencil_resolve_attachment(i_depth_resolve);
		subpass->set_depth_stencil_resolve_mode(depth_resolve_mode);

		// Save resolved attachment
		load_store[i_depth_resolve].store_op = VK_ATTACHMENT_STORE_OP_STORE;
	}
	else
	{
		// Bad practice
		// Save multisampled depth attachment, which cannot be resolved outside the renderpass
		// Storing multisampled depth should be avoided
		load_store[i_depth].store_op = VK_ATTACHMENT_STORE_OP_STORE;

		// Disable writeback resolve
		disable_depth_writeback_resolve(subpass, load_store);
	}
}

void DynamicState3MultisampleRasterization::disable_depth_writeback_resolve(std::unique_ptr<vkb::Subpass> &subpass, std::vector<vkb::LoadStoreInfo> &load_store)
{
	// Auxiliary single-sampled depth attachment is not used
	load_store[i_depth_resolve].store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Disable writeback resolve
	subpass->set_depth_stencil_resolve_attachment(VK_ATTACHMENT_UNUSED);
	subpass->set_depth_stencil_resolve_mode(VK_RESOLVE_MODE_NONE);
}

void DynamicState3MultisampleRasterization::draw(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
	auto &views = render_target.get_views();

	auto swapchain_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = swapchain_layout;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		for (auto &i_color : color_atts)
		{
			assert(i_color < views.size());
			command_buffer.image_memory_barrier(views[i_color], memory_barrier);
			render_target.set_layout(i_color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}
	}

	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		if (run_postprocessing)
		{
			// Synchronize depth with previous depth resolve operation
			memory_barrier.dst_stage_mask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			memory_barrier.dst_access_mask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}

		for (auto &i_depth : depth_atts)
		{
			assert(i_depth < views.size());
			command_buffer.image_memory_barrier(views[i_depth], memory_barrier);
			render_target.set_layout(i_depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}
	}

	auto &extent = render_target.get_extent();

	VkViewport viewport{};
	viewport.width    = static_cast<float>(extent.width);
	viewport.height   = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	command_buffer.set_viewport(0, {viewport});

	VkRect2D scissor{};
	scissor.extent = extent;
	command_buffer.set_scissor(0, {scissor});

	scene_pipeline->draw(command_buffer, render_target);

	if (!run_postprocessing)
	{
		// If postprocessing is enabled the GUI will be drawn
		// at the end of the postprocessing renderpass
		if (gui)
		{
			gui->draw(command_buffer);
		}
	}

	command_buffer.end_render_pass();

	bool msaa_enabled = sample_count != VK_SAMPLE_COUNT_1_BIT;

	if (msaa_enabled && ColorResolve::SeparatePass == color_resolve_method)
	{
		if (run_postprocessing)
		{
			resolve_color_separate_pass(command_buffer, views, i_color_resolve, swapchain_layout);
		}
		else
		{
			resolve_color_separate_pass(command_buffer, views, i_swapchain, swapchain_layout);
		}
	}

	if (run_postprocessing)
	{
		// Run a second renderpass
		postprocessing(command_buffer, render_target, swapchain_layout, msaa_enabled);
	}

	{
		// Prepare swapchain for presentation
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = swapchain_layout;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		assert(i_swapchain < views.size());
		command_buffer.image_memory_barrier(views[i_swapchain], memory_barrier);
	}
}

void DynamicState3MultisampleRasterization::postprocessing(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target,
                                                           VkImageLayout &swapchain_layout, bool msaa_enabled)
{
	/*
	auto        depth_attachment   = (msaa_enabled && depth_writeback_resolve_supported && resolve_depth_on_writeback) ? i_depth_resolve : i_depth;
	bool        multisampled_depth = msaa_enabled && !(depth_writeback_resolve_supported && resolve_depth_on_writeback);
	std::string depth_sampler_name = multisampled_depth ? "ms_depth_sampler" : "depth_sampler";

	glm::vec4 near_far = {camera->get_far_plane(), camera->get_near_plane(), -1.0f, -1.0f};

	auto &postprocessing_pass = postprocessing_pipeline->get_pass(0);
	postprocessing_pass.set_uniform_data(near_far);

	auto &postprocessing_subpass = postprocessing_pass.get_subpass(0);
	// Unbind sampled images to prevent invalid image transitions on unused images
	postprocessing_subpass.unbind_sampled_image("depth_sampler");
	postprocessing_subpass.unbind_sampled_image("ms_depth_sampler");

	postprocessing_subpass.get_fs_variant().clear();
	if (multisampled_depth)
	{
		postprocessing_subpass.get_fs_variant().add_define("MS_DEPTH");
	}
	postprocessing_subpass
	    .bind_sampled_image(depth_sampler_name, {depth_attachment, nullptr, nullptr, depth_writeback_resolve_supported && resolve_depth_on_writeback})
	    .bind_sampled_image("color_sampler", i_color_resolve);

	// Second render pass
	// NOTE: Color and depth attachments are automatically transitioned to be bound as textures
	postprocessing_pipeline->draw(command_buffer, render_target);

	if (gui)
	{
		gui->draw(command_buffer);
	}

	command_buffer.end_render_pass();
}

void DynamicState3MultisampleRasterization::resolve_color_separate_pass(vkb::CommandBuffer &command_buffer, const std::vector<vkb::core::ImageView> &views,
                                             uint32_t color_destination, VkImageLayout &color_layout)
{
	{
		// The multisampled color is the source of the resolve operation
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_READ_BIT;

		assert(i_color_ms < views.size());
		command_buffer.image_memory_barrier(views[i_color_ms], memory_barrier);
	}

	VkImageSubresourceLayers subresource = {0};
	subresource.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.layerCount               = 1;

	VkImageResolve image_resolve = {0};
	image_resolve.srcSubresource = subresource;
	image_resolve.dstSubresource = subresource;
	image_resolve.extent         = VkExtent3D{get_render_context().get_surface_extent().width, get_render_context().get_surface_extent().height, 1};

	{
		// Prepare destination image for transfer operation
		auto color_new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = color_layout;
		memory_barrier.new_layout      = color_new_layout;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;

		color_layout = color_new_layout;

		assert(color_destination < views.size());
		command_buffer.image_memory_barrier(views[color_destination], memory_barrier);
	}

	// Resolve multisampled attachment to destination, extremely expensive
	command_buffer.resolve_image(views[i_color_ms].get_image(), views.at(color_destination).get_image(), {image_resolve});

	// Transition attachments out of transfer stage
	{
		auto color_new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = color_layout;
		memory_barrier.new_layout      = color_new_layout;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		color_layout = color_new_layout;

		command_buffer.image_memory_barrier(views[color_destination], memory_barrier);
	}

	{
		vkb::ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_READ_BIT;
		memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		command_buffer.image_memory_barrier(views[i_color_ms], memory_barrier);
	}
}

void DynamicState3MultisampleRasterization::prepare_supported_sample_count_list()
{
	VkPhysicalDeviceProperties gpu_properties;
	vkGetPhysicalDeviceProperties(get_device().get_gpu().get_handle(), &gpu_properties);

	VkSampleCountFlags supported_by_depth_and_color = gpu_properties.limits.framebufferColorSampleCounts & gpu_properties.limits.framebufferDepthSampleCounts;

	// All possible sample counts are listed here from most to least preferred as default
	// On Mali GPUs 4X MSAA is recommended as best performance/quality trade-off
	std::vector<VkSampleCountFlagBits> counts = {VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_2_BIT, VK_SAMPLE_COUNT_8_BIT,
	                                             VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_64_BIT,
	                                             VK_SAMPLE_COUNT_1_BIT};

	for (auto &count : counts)
	{
		if (supported_by_depth_and_color & count)
		{
			supported_sample_count_list.push_back(count);

			if (sample_count == VK_SAMPLE_COUNT_1_BIT)
			{
				// Set default sample count based on the priority defined above
				sample_count          = count;
				gui_sample_count      = count;
				last_gui_sample_count = count;
			}
		}
	}
}

void DynamicState3MultisampleRasterization::prepare_depth_resolve_mode_list()
{
	if (instance->is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
	{
		VkPhysicalDeviceProperties2KHR gpu_properties{};
		gpu_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		VkPhysicalDeviceDepthStencilResolvePropertiesKHR depth_resolve_properties{};
		depth_resolve_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES_KHR;
		gpu_properties.pNext           = static_cast<void *>(&depth_resolve_properties);
		vkGetPhysicalDeviceProperties2KHR(get_device().get_gpu().get_handle(), &gpu_properties);

		if (depth_resolve_properties.supportedDepthResolveModes == 0)
		{
			LOGW("No depth stencil resolve modes supported");
			depth_writeback_resolve_supported = false;
		}
		else
		{
			// All possible modes are listed here from most to least preferred as default
			std::vector<VkResolveModeFlagBits> modes = {VK_RESOLVE_MODE_SAMPLE_ZERO_BIT, VK_RESOLVE_MODE_MIN_BIT,
			                                            VK_RESOLVE_MODE_MAX_BIT, VK_RESOLVE_MODE_AVERAGE_BIT};

			for (auto &mode : modes)
			{
				if (depth_resolve_properties.supportedDepthResolveModes & mode)
				{
					supported_depth_resolve_mode_list.push_back(mode);

					if (depth_resolve_mode == VK_RESOLVE_MODE_NONE)
					{
						// Set default mode based on the priority defined above
						depth_resolve_mode          = mode;
						gui_depth_resolve_mode      = mode;
						last_gui_depth_resolve_mode = mode;
					}
				}
			}
		}
	}
}

void DynamicState3MultisampleRasterization::draw_gui()
{
	auto       msaa_enabled = sample_count != VK_SAMPLE_COUNT_1_BIT;
	const bool landscape    = camera->get_aspect_ratio() > 1.0f;
	uint32_t   lines        = landscape ? 3 : 4;

	gui->show_options_window(
	    [this, msaa_enabled, landscape]() {
		    ImGui::AlignTextToFramePadding();
		    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.4f);

		    if (ImGui::BeginCombo("##sample_count", to_string(gui_sample_count).c_str()))
		    {
		        for (size_t n = 0; n < supported_sample_count_list.size(); n++)
		        {
		            bool is_selected = (gui_sample_count == supported_sample_count_list[n]);
		            if (ImGui::Selectable(to_string(supported_sample_count_list[n]).c_str(), is_selected))
		            {
		                gui_sample_count = supported_sample_count_list[n];
		            }
		            if (is_selected)
		            {
		                ImGui::SetItemDefaultFocus();
		            }
		        }
		        ImGui::EndCombo();
		    }

		    if (landscape)
		    {
			    ImGui::SameLine();
		    }
		    ImGui::Checkbox("Post-processing (2 renderpasses)", &gui_run_postprocessing);

		    ImGui::Text("Resolve color: ");
		    ImGui::SameLine();
		    if (msaa_enabled)
		    {
			    ImGui::RadioButton("On writeback", &gui_color_resolve_method, ColorResolve::OnWriteback);
			    ImGui::SameLine();
			    ImGui::RadioButton("Separate", &gui_color_resolve_method, ColorResolve::SeparatePass);
		    }
		    else
		    {
			    ImGui::Text("n/a");
		    }

		    ImGui::Text("Resolve depth: ");
		    ImGui::SameLine();
		    if (msaa_enabled && run_postprocessing)
		    {
			    if (depth_writeback_resolve_supported)
			    {
				    ImGui::Checkbox("##resolve_depth", &gui_resolve_depth_on_writeback);
				    ImGui::SameLine();
				    ImGui::Text("On writeback");
				    ImGui::SameLine();
				    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.3f);

				    if (ImGui::BeginCombo("##resolve_mode", to_string(gui_depth_resolve_mode).c_str()))
				    {
				        for (int n = 0; n < supported_depth_resolve_mode_list.size(); n++)
				        {
				            bool is_selected = (gui_depth_resolve_mode == supported_depth_resolve_mode_list[n]);
				            if (ImGui::Selectable(to_string(supported_depth_resolve_mode_list[n]).c_str(), is_selected))
				            {
				                gui_depth_resolve_mode = supported_depth_resolve_mode_list[n];
				            }
				            if (is_selected)
				            {
				                ImGui::SetItemDefaultFocus();
				            }
				        }
				        ImGui::EndCombo();
				    }
			    }
			    else
			    {
				    ImGui::Text("Not supported");
			    }
		    }
		    else
		    {
			    ImGui::Text("n/a");
		    }
	    },
	    lines);
}
*/

void DynamicState3MultisampleRasterization::prepare_pipelines()
{
	//static_cast<uint32_t>(texture->get_image()->get_mipmaps().size())
	//static_cast<uint32_t>(texture->get_image()->get_mipmaps().size())
	//static_cast<uint32_t>(scene->get_components<vkb::sg::Texture>())
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6},
	    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * static_cast<uint32_t>(scene->get_components<vkb::sg::Texture>().size())},
	    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6}};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 3);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));

	// The model information will be used to index textures in the fragment shader,
	// as well as perform frustum culling in the compute shader
	VkDescriptorSetLayoutBinding model_information_binding{};
	model_information_binding.binding         = 0;
	model_information_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	model_information_binding.descriptorCount = 1;
	model_information_binding.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

	// This array of textures will be accessed via the instance ID
	VkDescriptorSetLayoutBinding image_array_binding{};
	image_array_binding.binding         = 1;
	image_array_binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	image_array_binding.descriptorCount = scene->get_components<vkb::sg::Texture>().size();
	image_array_binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding scene_uniform_binding{};
	scene_uniform_binding.binding         = 2;
	scene_uniform_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	scene_uniform_binding.descriptorCount = 1;
	scene_uniform_binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding command_buffer_binding{};
	command_buffer_binding.binding         = 3;
	command_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	command_buffer_binding.descriptorCount = 1;
	command_buffer_binding.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

	// Create descriptors
	auto create_descriptors = [this](const std::vector<VkDescriptorSetLayoutBinding> &set_layout_bindings, VkDescriptorSetLayout &_descriptor_set_layout, VkPipelineLayout &_pipeline_layout) {
		VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &_descriptor_set_layout));

		VkPipelineLayoutCreateInfo pipeline_layout_create_info =
		    vkb::initializers::pipeline_layout_create_info(
		        &_descriptor_set_layout,
		        1);

		VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &_pipeline_layout));
	};

	// Render pipeline
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {model_information_binding, image_array_binding, scene_uniform_binding, command_buffer_binding};
	create_descriptors(set_layout_bindings, descriptor_set_layout, pipeline_layout);

	// Compute pipeline
	// Note we don't include the texture array
	/*
	std::vector<VkDescriptorSetLayoutBinding> gpu_compute_set_layout_bindings = {model_information_binding, scene_uniform_binding, command_buffer_binding};
	create_descriptors(gpu_compute_set_layout_bindings, gpu_cull_descriptor_set_layout, gpu_cull_pipeline_layout);
	*/

	// Device address pipeline
	// Note that we don't bind the command buffer directly; instead, we use the references from the device addresses
	// This will be used in the device address shader (cull_address.comp)
	/*
	if (m_supports_buffer_device)
	{
		VkDescriptorSetLayoutBinding device_address_binding{};
		device_address_binding.binding                                           = 4;
		device_address_binding.descriptorType                                    = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		device_address_binding.descriptorCount                                   = 1;
		device_address_binding.stageFlags                                        = VK_SHADER_STAGE_COMPUTE_BIT;
		std::vector<VkDescriptorSetLayoutBinding> device_address_layout_bindings = {model_information_binding, scene_uniform_binding, device_address_binding};
		create_descriptors(device_address_layout_bindings, device_address_descriptor_set_layout, device_address_pipeline_layout);
	}
	*/

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

	VkPipelineColorBlendAttachmentState blend_attachment_state = vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	depth_stencil_state.depthBoundsTestEnable                 = VK_FALSE;
	depth_stencil_state.minDepthBounds                        = 0.f;
	depth_stencil_state.maxDepthBounds                        = 1.f;

	VkPipelineViewportStateCreateInfo viewport_state = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	VkPipelineMultisampleStateCreateInfo multisample_state = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(GpuModelInformation), VK_VERTEX_INPUT_RATE_INSTANCE),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pt)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)),
	    //vkb::initializers::vertex_input_attribute_description(1, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GpuModelInformation, bounding_sphere_center)),
	    //vkb::initializers::vertex_input_attribute_description(1, 3, VK_FORMAT_R32_SFLOAT, offsetof(GpuModelInformation, bounding_sphere_radius)),
	    vkb::initializers::vertex_input_attribute_description(1, 4, VK_FORMAT_R32_UINT, offsetof(GpuModelInformation, texture_index)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pRasterizationState          = &rasterization_state;
	pipeline_create_info.pColorBlendState             = &color_blend_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;

	const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
	    load_shader("multi_draw_indirect/multi_draw_indirect.vert", VK_SHADER_STAGE_VERTEX_BIT),
	    load_shader("multi_draw_indirect/multi_draw_indirect.frag", VK_SHADER_STAGE_FRAGMENT_BIT)};

	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

void DynamicState3MultisampleRasterization::initialize_descriptors()
{
	enum class Target
	{
		RenderPipeline,
		ComputePipeline,
		AddressPipeline
	};

	auto bind = [this](VkDescriptorSet &_descriptor_set, VkDescriptorSetLayout &_descriptor_set_layout, Target target) {
		VkDescriptorSetAllocateInfo descriptor_set_allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &_descriptor_set_layout, 1);
		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &_descriptor_set));

		VkDescriptorBufferInfo model_buffer_descriptor = create_descriptor(*model_information_buffer);
		VkWriteDescriptorSet   model_write             = vkb::initializers::write_descriptor_set(_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &model_buffer_descriptor, 1);

		VkWriteDescriptorSet texture_array_write = vkb::initializers::write_descriptor_set(_descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, image_descriptors.data(), image_descriptors.size());

		VkDescriptorBufferInfo scene_descriptor = create_descriptor(*scene_uniform_buffer);
		VkWriteDescriptorSet   scene_write      = vkb::initializers::write_descriptor_set(_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &scene_descriptor, 1);

		VkDescriptorBufferInfo draw_command_descriptor = create_descriptor(*indirect_call_buffer);
		VkWriteDescriptorSet   draw_command_write      = vkb::initializers::write_descriptor_set(_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &draw_command_descriptor, 1);

		VkDescriptorBufferInfo device_address_descriptor;
		VkWriteDescriptorSet   device_address_write;
		/*
		if (m_supports_buffer_device && device_address_buffer)
		{
			device_address_descriptor = create_descriptor(*device_address_buffer);
			device_address_write      = vkb::initializers::write_descriptor_set(_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &device_address_descriptor, 1);
		}
		*/

		std::vector<VkWriteDescriptorSet> write_descriptor_sets;
		switch (target)
		{
			case Target::RenderPipeline:
				write_descriptor_sets = {model_write, texture_array_write, scene_write, draw_command_write};
				break;
			case Target::ComputePipeline:
				write_descriptor_sets = {model_write, scene_write, draw_command_write};
				break;
			case Target::AddressPipeline:
				write_descriptor_sets = {model_write, scene_write, device_address_write};
				break;
		}

		vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
	};

	// render pipeline
	bind(descriptor_set, descriptor_set_layout, Target::RenderPipeline);

	/*
	//  compute pipeline
	bind(gpu_cull_descriptor_set, gpu_cull_descriptor_set_layout, Target::ComputePipeline);

	// Device address pipeline
	if (m_supports_buffer_device)
	{
		bind(device_address_descriptor_set, device_address_descriptor_set_layout, Target::AddressPipeline);
	}
	*/
}

/*
void MultiDrawIndirect::create_compute_pipeline()
{
	auto create = [this](VkPipelineLayout &layout, VkPipeline &_pipeline, const char *filename) {
		VkComputePipelineCreateInfo compute_create_info = vkb::initializers::compute_pipeline_create_info(layout, 0);
		compute_create_info.stage                       = load_shader(filename, VK_SHADER_STAGE_COMPUTE_BIT);

		VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1, &compute_create_info, nullptr, &_pipeline));
	};

	create(gpu_cull_pipeline_layout, gpu_cull_pipeline, "multi_draw_indirect/cull.comp");

	if (m_supports_buffer_device)
	{
		create(device_address_pipeline_layout, device_address_pipeline, "multi_draw_indirect/cull_address.comp");
	}
}
*/

/*
void DynamicState3MultisampleRasterization::prepare_pipelines()
{
	// Create a blank pipeline layout.
	// We are not binding any resources to the pipeline in this sample.
	VkPipelineLayoutCreateInfo layout_info = vkb::initializers::pipeline_layout_create_info(nullptr, 0);
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &sample_pipeline_layout));

	VkPipelineVertexInputStateCreateInfo vertex_input = vkb::initializers::pipeline_vertex_input_state_create_info();

	// Specify we will use triangle lists to draw geometry.
	VkPipelineInputAssemblyStateCreateInfo input_assembly = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	// Specify rasterization state.
	VkPipelineRasterizationStateCreateInfo raster = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blend_attachment = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo blend = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);

	// We will have one viewport and scissor box.
	VkPipelineViewportStateCreateInfo viewport = vkb::initializers::pipeline_viewport_state_create_info(1, 1);

	// Enable depth testing (using reversed depth-buffer for increased precision).
	VkPipelineDepthStencilStateCreateInfo depth_stencil = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);

	// No multisampling.
	VkPipelineMultisampleStateCreateInfo multisample = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

	// Specify that these states will be dynamic, i.e. not part of pipeline state object.
	std::array<VkDynamicState, 2>    dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic = vkb::initializers::pipeline_dynamic_state_create_info(dynamics.data(), vkb::to_u32(dynamics.size()));

	// Load our SPIR-V shaders.
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

	// Vertex stage of the pipeline
	shader_stages[0] = load_shader("triangle.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("triangle.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// We need to specify the pipeline layout and the render pass description up front as well.
	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(sample_pipeline_layout, render_pass);
	pipeline_create_info.stageCount                   = vkb::to_u32(shader_stages.size());
	pipeline_create_info.pStages                      = shader_stages.data();
	pipeline_create_info.pVertexInputState            = &vertex_input;
	pipeline_create_info.pInputAssemblyState          = &input_assembly;
	pipeline_create_info.pRasterizationState          = &raster;
	pipeline_create_info.pColorBlendState             = &blend;
	pipeline_create_info.pMultisampleState            = &multisample;
	pipeline_create_info.pViewportState               = &viewport;
	pipeline_create_info.pDepthStencilState           = &depth_stencil;
	pipeline_create_info.pDynamicState                = &dynamic;

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &sample_pipeline));
}


void DynamicState3MultisampleRasterization::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	// Clear color and depth values.
	VkClearValue clear_values[2];
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	// Begin the render pass.
	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		auto cmd = draw_cmd_buffers[i];

		// Begin command buffer.
		vkBeginCommandBuffer(cmd, &command_buffer_begin_info);

		// Set framebuffer for this command buffer.
		render_pass_begin_info.framebuffer = framebuffers[i];
		// We will add draw commands in the same command buffer.
		vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Bind the graphics pipeline.
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sample_pipeline);

		// Set viewport dynamically
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		// Set scissor dynamically
		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// Draw three vertices with one instance.
		vkCmdDraw(cmd, 3, 1, 0, 0);

		// Draw user interface.
		draw_ui(draw_cmd_buffers[i]);

		// Complete render pass.
		vkCmdEndRenderPass(cmd);

		// Complete the command buffer.
		VK_CHECK(vkEndCommandBuffer(cmd));
	}
}
*/

void DynamicState3MultisampleRasterization::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2];
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
		VkDeviceSize offsets[1] = {0};

		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, model_information_buffer->get(), offsets);
		/*
		if (m_enable_mdi && m_supports_mdi)
		{
			vkCmdDrawIndexedIndirect(draw_cmd_buffers[i], indirect_call_buffer->get_handle(), 0, cpu_commands.size(), sizeof(cpu_commands[0]));
		}
		else
		*/
		{
			for (size_t j = 0; j < cpu_commands.size(); ++j)
			{
				vkCmdDrawIndexedIndirect(draw_cmd_buffers[i], indirect_call_buffer->get_handle(), j * sizeof(cpu_commands[0]), 1, sizeof(cpu_commands[0]));
			}
		}

		//draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void DynamicState3MultisampleRasterization::create_sampler()
{
	VkSamplerCreateInfo sampler_info = vkb::initializers::sampler_create_info();
	sampler_info.magFilter           = VK_FILTER_LINEAR;
	sampler_info.minFilter           = VK_FILTER_LINEAR;
	sampler_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.mipLodBias          = 0.0f;
	sampler_info.maxAnisotropy       = 1.0f;
	sampler_info.minLod              = 0.0f;
	sampler_info.maxLod              = 1.0f;
	sampler_info.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_info, nullptr, &sampler));
}

static float z = -500.0;

void DynamicState3MultisampleRasterization::render(float delta_time)
{
	draw();

	z += 0.01;
	camera.set_translation(glm::vec3(0.0, 5, z));

	update_scene_uniform();
	/*
	if (!prepared)
	{
		return;
	}
	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();
	*/
}


std::unique_ptr<vkb::VulkanSample> create_dynamic_state3_multisample_rasterization()
{
	return std::make_unique<DynamicState3MultisampleRasterization>();
}
