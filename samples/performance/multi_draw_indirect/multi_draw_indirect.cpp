/* Copyright (c) 2021-2022, Holochip Corporation
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

#include "multi_draw_indirect.h"
#include "gltf_loader.h"
#include "ktx.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"

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
		auto &         buffer = iter->second;
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

MultiDrawIndirect::MultiDrawIndirect()
{
	set_api_version(VK_API_VERSION_1_2);
	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, true /* optional */);
}

MultiDrawIndirect::~MultiDrawIndirect()
{
	if (device)
	{
		vertex_buffer.reset();
		index_buffer.reset();
		model_information_buffer.reset();
		scene_uniform_buffer.reset();

		vkDestroyPipeline(device->get_handle(), pipeline, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(device->get_handle(), pipeline_layout, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(device->get_handle(), descriptor_set_layout, VK_NULL_HANDLE);
		vkDestroySampler(device->get_handle(), sampler, VK_NULL_HANDLE);

		vkDestroyPipeline(device->get_handle(), gpu_cull_pipeline, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(device->get_handle(), gpu_cull_pipeline_layout, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(device->get_handle(), gpu_cull_descriptor_set_layout, VK_NULL_HANDLE);

		vkDestroyPipeline(device->get_handle(), device_address_pipeline, VK_NULL_HANDLE);
		vkDestroyPipelineLayout(device->get_handle(), device_address_pipeline_layout, VK_NULL_HANDLE);
		vkDestroyDescriptorSetLayout(device->get_handle(), device_address_descriptor_set_layout, VK_NULL_HANDLE);

		device_address_buffer.reset();

		cpu_staging_buffer.reset();
		indirect_call_buffer.reset();
	}
}

void MultiDrawIndirect::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().multiDrawIndirect)
	{
		gpu.get_mutable_requested_features().multiDrawIndirect = VK_TRUE;
		m_supports_mdi                                         = true;
	}

	if (gpu.get_features().drawIndirectFirstInstance)
	{
		gpu.get_mutable_requested_features().drawIndirectFirstInstance = VK_TRUE;
		m_supports_first_instance                                      = true;
	}

	// Query whether the device supports buffer device addresses
	VkPhysicalDeviceVulkan12Features features12;
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.pNext = VK_NULL_HANDLE;
	VkPhysicalDeviceFeatures2 features2;
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.pNext = &features12;
	vkGetPhysicalDeviceFeatures2(gpu.get_handle(), &features2);

	m_supports_buffer_device = features12.bufferDeviceAddress;

	if (m_supports_buffer_device)
	{
		auto &features = gpu.request_extension_features<VkPhysicalDeviceBufferDeviceAddressFeaturesKHR>(
		    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR);
		features.bufferDeviceAddress = VK_TRUE;
	}

	// This sample references 128 objects. We need to check whether this is supported by the device
	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(gpu.get_handle(), &physical_device_properties);

	if (physical_device_properties.limits.maxPerStageDescriptorSamplers < 128)
	{
		throw std::runtime_error(fmt::format(FMT_STRING("This sample requires at least 128 descriptor samplers, but device only supports {:d}"), physical_device_properties.limits.maxPerStageDescriptorSamplers));
	}
}

void MultiDrawIndirect::build_command_buffers()
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

		VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
		VkDeviceSize offsets[1] = {0};

		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, model_information_buffer->get(), offsets);

		if (m_enable_mdi && m_supports_mdi)
		{
			vkCmdDrawIndexedIndirect(draw_cmd_buffers[i], indirect_call_buffer->get_handle(), 0, cpu_commands.size(), sizeof(cpu_commands[0]));
		}
		else
		{
			for (size_t j = 0; j < cpu_commands.size(); ++j)
			{
				vkCmdDrawIndexedIndirect(draw_cmd_buffers[i], indirect_call_buffer->get_handle(), j * sizeof(cpu_commands[0]), 1, sizeof(cpu_commands[0]));
			}
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void MultiDrawIndirect::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("GPU Rendering"))
	{
		static const std::array<const char *, 2> supported = {"Not supported", "Supported"};
		drawer.text("Multi-Draw Indirect: %s", supported[this->m_supports_mdi]);
		drawer.text("drawIndirectFirstInstance: %s", supported[this->m_supports_first_instance]);
		drawer.text("Device buffer address: %s", supported[this->m_supports_buffer_device]);

		drawer.text("");
		uint32_t instance_count = 0;

		if (render_mode == RenderMode::GPU || render_mode == RenderMode::GPU_DEVICE_ADDRESS)
		{
			// copy over the GPU-culled data to the CPU command so that we can count the number of instances
			assert(!!indirect_call_buffer && !!cpu_staging_buffer && indirect_call_buffer->get_size() == cpu_staging_buffer->get_size());
			assert(cpu_commands.size() * sizeof(cpu_commands[0]) == cpu_staging_buffer->get_size());

			auto &cmd = device->request_command_buffer();
			cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			cmd.copy_buffer(*indirect_call_buffer, *cpu_staging_buffer, cpu_staging_buffer->get_size());
			cmd.end();

			auto &queue = device->get_queue_by_flags(VK_QUEUE_COMPUTE_BIT, 0);
			queue.submit(cmd, device->request_fence());
			device->get_fence_pool().wait();

			memcpy(cpu_commands.data(), cpu_staging_buffer->get_data(), cpu_staging_buffer->get_size());
		}

		for (auto &&cmd : cpu_commands)
		{
			instance_count += cmd.instanceCount;
		}
		drawer.text("Instances: %d / %d", instance_count, 256);

		m_requires_rebuild |= drawer.checkbox("Enable multi-draw", &m_enable_mdi);
		drawer.checkbox("Freeze culling", &m_freeze_cull);

		int32_t render_selection = render_mode;
		if (drawer.combo_box("Cull mode", &render_selection, {"CPU", "GPU", "GPU Device Address"}))
		{
			m_requires_rebuild = true;
			render_mode        = static_cast<RenderMode>(render_selection);
		}
	}
}

void MultiDrawIndirect::create_sampler()
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

bool MultiDrawIndirect::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::FirstPerson;
	camera.set_perspective(60.0f, (float) width / (float) height, 0.001f, 512.0f);
	camera.set_rotation(glm::vec3(-23.5, -45, 0));
	camera.set_translation(glm::vec3(0, 0.5, -0.2));

	if (!compute_queue)
	{
		compute_queue = &device->get_queue_by_flags(VK_QUEUE_COMPUTE_BIT, 0);
	}

	queue_families.clear();
	for (auto &&queue_bit : {VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT})
	{
		const auto index = device->get_queue_by_flags(queue_bit, 0).get_family_index();
		if (std::find(queue_families.cbegin(), queue_families.cend(), index) == queue_families.cend())
		{
			queue_families.emplace_back(index);
		}
	}

	create_sampler();
	load_scene();
	initialize_resources();
	update_scene_uniform();
	create_pipeline();
	create_compute_pipeline();
	initialize_descriptors();
	build_command_buffers();
	cpu_cull();        // initialize buffer
	run_cull();

	prepared = true;

	return true;
}

void MultiDrawIndirect::load_scene()
{
	assert(!!device);
	vkb::GLTFLoader   loader{*device};
	const std::string scene_path = "scenes/vokselia/";
	auto              scene      = loader.read_scene_from_file(scene_path + "vokselia.gltf");

	assert(!!scene);
	for (auto &&mesh : scene->get_components<vkb::sg::Mesh>())
	{
		const size_t texture_index = textures.size();
		const auto & short_name    = mesh->get_name();
		auto         image_name    = scene_path + short_name + ".ktx";
		auto         image         = vkb::sg::Image::load(image_name, image_name, vkb::sg::Image::Color);

		image->create_vk_image(*device);
		Texture texture;
		texture.n_mip_maps = uint32_t(image->get_mipmaps().size());
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

		VkImageMemoryBarrier image_barrier = vkb::initializers::image_memory_barrier();
		image_barrier.srcAccessMask        = 0;
		image_barrier.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_barrier.image                = texture.image->get_handle();
		image_barrier.subresourceRange     = subresource_range;
		image_barrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
		image_barrier.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		vkCmdPipelineBarrier(texture_cmd.get_handle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

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

		for (auto &&sub_mesh : mesh->get_submeshes())
		{
			SceneModel model;
			model.texture_index = texture_index;

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
					    uint16_t(temp_buffer[3 * i]),
					    uint16_t(temp_buffer[3 * i + 1]),
					    uint16_t(temp_buffer[3 * i + 2])};
				}
			}
			model.bounding_sphere = BoundingSphere(pts);
			models.emplace_back(std::move(model));
		}
	}

	auto &cmd = device->get_command_pool().request_command_buffer();
	cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_NULL_HANDLE);
	std::vector<VkImageMemoryBarrier> image_barriers;
	image_barriers.reserve(textures.size());
	for (auto &&texture : textures)
	{
		VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		subresource_range.baseMipLevel            = 0;
		subresource_range.levelCount              = texture.n_mip_maps;

		VkImageMemoryBarrier image_barrier = vkb::initializers::image_memory_barrier();
		image_barrier.srcAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_barrier.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;
		image_barrier.image                = texture.image->get_handle();
		image_barrier.subresourceRange     = subresource_range;
		image_barrier.oldLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		image_barrier.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		image_barriers.emplace_back(image_barrier);
	}
	vkCmdPipelineBarrier(cmd.get_handle(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(image_barriers.size()), image_barriers.data());
	cmd.end();
	auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
	queue.submit(cmd, device->request_fence());
	device->get_fence_pool().wait();
}

void MultiDrawIndirect::initialize_resources()
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
	if (m_supports_buffer_device)
	{
		indirect_flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}
	indirect_call_buffer = std::make_unique<vkb::core::Buffer>(get_device(), models.size() * sizeof(VkDrawIndexedIndirectCommand), indirect_flags, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, queue_families);

	// Create a buffer containing the addresses of the indirect calls.
	// In this sample, the order of the addresses will match that of the other buffers, but in general they could be in any order
	const size_t address_buffer_size    = sizeof(VkDeviceAddress);
	auto         staging_address_buffer = std::make_unique<vkb::core::Buffer>(get_device(), address_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	if (m_supports_buffer_device)
	{
		auto *destPtr = (uint64_t *) staging_address_buffer->get_data();

		VkBufferDeviceAddressInfoKHR address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
		address_info.buffer    = indirect_call_buffer->get_handle();
		VkDeviceAddress srcPtr = vkGetBufferDeviceAddressKHR(device->get_handle(), &address_info);

		*destPtr = srcPtr;
	}

	for (size_t i = 0; i < models.size(); ++i)
	{
		auto &model = models[i];
		staging_vertex_buffer->update(model.vertices.data(), model.vertices.size() * sizeof(Vertex), model.vertex_buffer_offset);
		staging_index_buffer->update(model.triangles.data(), model.triangles.size() * sizeof(model.triangles[0]), model.index_buffer_offset);

		GpuModelInformation model_information;
		model_information.bounding_sphere_center = model.bounding_sphere.center;
		model_information.bounding_sphere_radius = model.bounding_sphere.radius;
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
	if (m_supports_buffer_device)
	{
		// In this sample, we use a staging buffer for the device address buffer (i.e. for device exclusive memory).
		// However, since the size of each element (sizeof(uint64_t)) is smaller than the objects it's pointing to, it could instead use host-visible memory
		// for fast referencing of the underlying data
		device_address_buffer = copy(*staging_address_buffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
	}

	cmd.end();
	auto &queue = device->get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
	queue.submit(cmd, device->request_fence());
	device->get_fence_pool().wait();
}

void MultiDrawIndirect::create_pipeline()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6},
	    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * static_cast<uint32_t>(textures.size())},
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
	image_array_binding.descriptorCount = textures.size();
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
	std::vector<VkDescriptorSetLayoutBinding> gpu_compute_set_layout_bindings = {model_information_binding, scene_uniform_binding, command_buffer_binding};
	create_descriptors(gpu_compute_set_layout_bindings, gpu_cull_descriptor_set_layout, gpu_cull_pipeline_layout);

	// Device address pipeline
	// Note that we don't bind the command buffer directly; instead, we use the references from the device addresses
	// This will be used in the device address shader (cull_address.comp)
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
	    vkb::initializers::vertex_input_attribute_description(1, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GpuModelInformation, bounding_sphere_center)),
	    vkb::initializers::vertex_input_attribute_description(1, 3, VK_FORMAT_R32_SFLOAT, offsetof(GpuModelInformation, bounding_sphere_radius)),
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

void MultiDrawIndirect::initialize_descriptors()
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
		if (m_supports_buffer_device && device_address_buffer)
		{
			device_address_descriptor = create_descriptor(*device_address_buffer);
			device_address_write      = vkb::initializers::write_descriptor_set(_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &device_address_descriptor, 1);
		}

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

	//  compute pipeline
	bind(gpu_cull_descriptor_set, gpu_cull_descriptor_set_layout, Target::ComputePipeline);

	// Device address pipeline
	if (m_supports_buffer_device)
	{
		bind(device_address_descriptor_set, device_address_descriptor_set_layout, Target::AddressPipeline);
	}
}

void MultiDrawIndirect::update_scene_uniform()
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

void MultiDrawIndirect::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void MultiDrawIndirect::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	if (render_mode == GPU_DEVICE_ADDRESS && !m_supports_buffer_device)
	{
		render_mode = GPU;
	}

	if (m_requires_rebuild)
	{
		build_command_buffers();
		m_requires_rebuild = false;
	}

	draw();

	update_scene_uniform();

	if (!m_freeze_cull)
	{
		run_cull();
	}
	device->get_fence_pool().wait();
	device->get_fence_pool().reset();
}

void MultiDrawIndirect::finish()
{
}

void MultiDrawIndirect::run_cull()
{
	switch (render_mode)
	{
		case RenderMode::CPU:
			cpu_cull();
			break;
		case RenderMode::GPU:
		case RenderMode::GPU_DEVICE_ADDRESS:
			run_gpu_cull();
			break;
	}
}

void MultiDrawIndirect::run_gpu_cull()
{
	assert(!!gpu_cull_pipeline);
	auto                     cmd   = device->create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo begin = vkb::initializers::command_buffer_begin_info();

	vkBeginCommandBuffer(cmd, &begin);
	auto bind = [&cmd](VkPipeline &_pipeline, VkPipelineLayout &_pipeline_layout, VkDescriptorSet &_descriptor_set) {
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline_layout, 0, 1, &_descriptor_set, 0, nullptr);
	};

	if (render_mode == RenderMode::GPU)
	{
		bind(gpu_cull_pipeline, gpu_cull_pipeline_layout, gpu_cull_descriptor_set);
	}
	else
	{
		bind(device_address_pipeline, device_address_pipeline_layout, device_address_descriptor_set);
	}

	const uint32_t dispatch_x = !models.empty() ? 1 + static_cast<uint32_t>((models.size() - 1) / 64) : 1;
	vkCmdDispatch(cmd, dispatch_x, 1, 1);
	vkEndCommandBuffer(cmd);

	VkSubmitInfo submit       = vkb::initializers::submit_info();
	submit.commandBufferCount = 1;
	submit.pCommandBuffers    = &cmd;

	vkQueueSubmit(compute_queue->get_handle(), 1, &submit, device->request_fence());
	device->get_fence_pool().wait();
	device->get_fence_pool().reset();
	// we're done so dealloc it from the pool.
	vkFreeCommandBuffers(device->get_handle(), device->get_command_pool().get_handle(), 1, &cmd);
}

namespace
{
/**
 * @brief Test for visibility using bounding sphere.
 * See https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
 */
struct VisibilityTester
{
	explicit VisibilityTester(glm::mat4x4 view_matrix) :
	    planes(get_view_planes(view_matrix))
	{}
	std::array<glm::vec4, 6> planes;

	static std::array<glm::vec4, 6> get_view_planes(const glm::mat4x4 &mat)
	{
		using namespace glm;
		std::array<vec4, 6> out{};
		for (auto i = 0; i < 3; ++i)
		{
			for (size_t j = 0; j < 2; ++j)
			{
				const float sign = j ? 1.f : -1.f;
				for (auto k = 0; k < 4; ++k)
				{
					out[2 * i + j][k] = mat[k][3] + sign * mat[k][i];
				}
			}
		}

		// normalize plane; see Appendix A.2
		for (auto &&plane : out)
		{
			plane /= float(length(vec3(plane.xyz)));
		}
		return out;
	}

	bool is_visible(glm::vec3 origin, float radius) const
	{
		using namespace glm;
		std::array<int, 4> V{0, 1, 4, 5};
		return std::all_of(V.begin(), V.end(), [this, origin, radius](size_t i) {
			const auto &plane = planes[i];
			return dot(origin, vec3(plane.xyz)) + plane.w + radius >= 0;
		});
	}
};

}        // namespace

void MultiDrawIndirect::cpu_cull()
{
	cpu_commands.resize(models.size());

	VisibilityTester tester(scene_uniform.proj * scene_uniform.view);
	for (size_t i = 0; i < models.size(); ++i)
	{
		// we control visibility by changing the instance count
		auto &                       model = models[i];
		VkDrawIndexedIndirectCommand cmd{};
		cmd.firstIndex    = model.index_buffer_offset / (sizeof(model.triangles[0][0]));
		cmd.indexCount    = static_cast<uint32_t>(model.triangles.size()) * 3;
		cmd.vertexOffset  = static_cast<int32_t>(model.vertex_buffer_offset / sizeof(Vertex));
		cmd.firstInstance = i;
		cmd.instanceCount = tester.is_visible(model.bounding_sphere.center, model.bounding_sphere.radius);
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

std::unique_ptr<vkb::VulkanSample> create_multi_draw_indirect()
{
	return std::make_unique<MultiDrawIndirect>();
}

MultiDrawIndirect::BoundingSphere::BoundingSphere(const std::vector<glm::vec3> &pts)
{
	if (pts.empty())
	{
		return;
	}

	// This is a simple method of calculating a bounding sphere.
	// For finding an optimal bounding sphere, see Welzl's algorithm
	this->center = {0, 0, 0};
	for (auto &&pt : pts)
	{
		this->center += pt;
	}
	this->center /= float(pts.size());
	this->radius = glm::distance2(pts[0], this->center);
	for (size_t i = 1; i < pts.size(); ++i)
	{
		this->radius = std::max(this->radius, glm::distance2(pts[i], this->center));
	}
	this->radius = std::nextafter(sqrtf(this->radius), std::numeric_limits<float>::max());
}
