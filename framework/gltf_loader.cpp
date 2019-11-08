/* Copyright (c) 2018-2019, Arm Limited and Contributors
 * Copyright (c) 2019, Sascha Willems
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

#define TINYGLTF_IMPLEMENTATION
#include "gltf_loader.h"

#include <limits>
#include <queue>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <glm/glm.hpp>
VKBP_ENABLE_WARNINGS()

#include "api_vulkan_sample.h"
#include "common/logging.h"
#include "common/utils.h"
#include "common/vk_common.h"
#include "core/device.h"
#include "core/image.h"
#include "platform/filesystem.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/image/astc.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/perspective_camera.h"
#include "scene_graph/components/sampler.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/components/transform.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"

#include <ctpl_stl.h>

namespace vkb
{
namespace
{
inline VkFilter find_min_filter(int min_filter)
{
	switch (min_filter)
	{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			return VK_FILTER_NEAREST;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			return VK_FILTER_LINEAR;
		default:
			return VK_FILTER_LINEAR;
	}
};

inline VkSamplerMipmapMode find_mipmap_mode(int min_filter)
{
	switch (min_filter)
	{
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
};

inline VkFilter find_mag_filter(int mag_filter)
{
	switch (mag_filter)
	{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
			return VK_FILTER_NEAREST;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
			return VK_FILTER_LINEAR;
		default:
			return VK_FILTER_LINEAR;
	}
};

inline VkSamplerAddressMode find_wrap_mode(int wrap)
{
	switch (wrap)
	{
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		default:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
};

inline std::vector<uint8_t> get_attribute_data(const tinygltf::Model *model, uint32_t accessorId)
{
	auto &accessor   = model->accessors.at(accessorId);
	auto &bufferView = model->bufferViews.at(accessor.bufferView);
	auto &buffer     = model->buffers.at(bufferView.buffer);

	size_t stride    = accessor.ByteStride(bufferView);
	size_t startByte = accessor.byteOffset + bufferView.byteOffset;
	size_t endByte   = startByte + accessor.count * stride;

	return {buffer.data.begin() + startByte, buffer.data.begin() + endByte};
};

inline size_t get_attribute_size(const tinygltf::Model *model, uint32_t accessorId)
{
	return model->accessors.at(accessorId).count;
};

inline size_t get_attribute_stride(const tinygltf::Model *model, uint32_t accessorId)
{
	auto &accessor   = model->accessors.at(accessorId);
	auto &bufferView = model->bufferViews.at(accessor.bufferView);

	return accessor.ByteStride(bufferView);
};

inline VkFormat get_attribute_format(const tinygltf::Model *model, uint32_t accessorId)
{
	auto &accessor = model->accessors.at(accessorId);

	VkFormat format;

	switch (accessor.componentType)
	{
		case TINYGLTF_COMPONENT_TYPE_BYTE:
		{
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_SINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_SINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_SINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_SINT}};

			format = mapped_format.at(accessor.type);

			break;
		}
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		{
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_UINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_UINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_UINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_UINT}};

			static const std::map<int, VkFormat> mapped_format_normalize = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_UNORM},
			                                                                {TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_UNORM},
			                                                                {TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_UNORM},
			                                                                {TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_UNORM}};

			if (accessor.normalized)
			{
				format = mapped_format_normalize.at(accessor.type);
			}
			else
			{
				format = mapped_format.at(accessor.type);
			}

			break;
		}
		case TINYGLTF_COMPONENT_TYPE_SHORT:
		{
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_SINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_SINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_SINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_SINT}};

			format = mapped_format.at(accessor.type);

			break;
		}
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		{
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R16_UINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R16G16_UINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R16G16B16_UINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R16G16B16A16_UINT}};

			static const std::map<int, VkFormat> mapped_format_normalize = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R16_UNORM},
			                                                                {TINYGLTF_TYPE_VEC2, VK_FORMAT_R16G16_UNORM},
			                                                                {TINYGLTF_TYPE_VEC3, VK_FORMAT_R16G16B16_UNORM},
			                                                                {TINYGLTF_TYPE_VEC4, VK_FORMAT_R16G16B16A16_UNORM}};

			if (accessor.normalized)
			{
				format = mapped_format_normalize.at(accessor.type);
			}
			else
			{
				format = mapped_format.at(accessor.type);
			}

			break;
		}
		case TINYGLTF_COMPONENT_TYPE_INT:
		{
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_SINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R32G32_SINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R32G32B32_SINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R32G32B32A32_SINT}};

			format = mapped_format.at(accessor.type);

			break;
		}
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		{
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_UINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R32G32_UINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R32G32B32_UINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R32G32B32A32_UINT}};

			format = mapped_format.at(accessor.type);

			break;
		}
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
		{
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_SFLOAT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R32G32_SFLOAT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R32G32B32_SFLOAT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R32G32B32A32_SFLOAT}};

			format = mapped_format.at(accessor.type);

			break;
		}
		default:
		{
			format = VK_FORMAT_UNDEFINED;
			break;
		}
	}

	return format;
};

inline std::vector<uint8_t> convert_data(const std::vector<uint8_t> &srcData, uint32_t srcStride, uint32_t dstStride)
{
	auto elem_count = to_u32(srcData.size()) / srcStride;

	std::vector<uint8_t> result(elem_count * dstStride);

	for (uint32_t idxSrc = 0, idxDst = 0;
	     idxSrc < srcData.size() && idxDst < result.size();
	     idxSrc += srcStride, idxDst += dstStride)
	{
		std::copy(srcData.begin() + idxSrc, srcData.begin() + idxSrc + srcStride, result.begin() + idxDst);
	}

	return result;
}

inline void upload_image_to_gpu(CommandBuffer &command_buffer, core::Buffer &staging_buffer, sg::Image &image)
{
	// Clean up the image data, as they are copied in the staging buffer
	image.clear_data();

	{
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_HOST_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;

		command_buffer.image_memory_barrier(image.get_vk_image_view(), memory_barrier);
	}

	// Create a buffer image copy for every mip level
	auto &mipmaps = image.get_mipmaps();

	std::vector<VkBufferImageCopy> buffer_copy_regions(mipmaps.size());

	for (size_t i = 0; i < mipmaps.size(); ++i)
	{
		auto &mipmap      = mipmaps[i];
		auto &copy_region = buffer_copy_regions[i];

		copy_region.bufferOffset     = mipmap.offset;
		copy_region.imageSubresource = image.get_vk_image_view().get_subresource_layers();
		// Update miplevel
		copy_region.imageSubresource.mipLevel = mipmap.level;
		copy_region.imageExtent               = mipmap.extent;
	}

	command_buffer.copy_buffer_to_image(staging_buffer, image.get_vk_image(), buffer_copy_regions);

	{
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		command_buffer.image_memory_barrier(image.get_vk_image_view(), memory_barrier);
	}
}
}        // namespace

GLTFLoader::GLTFLoader(Device &device) :
    device{device}
{
}

std::unique_ptr<sg::Scene> GLTFLoader::read_scene_from_file(const std::string &file_name)
{
	std::string err;
	std::string warn;

	tinygltf::TinyGLTF gltf_loader;

	std::string gltf_file = vkb::fs::path::get(vkb::fs::path::Type::Assets) + file_name;

	bool importResult = gltf_loader.LoadASCIIFromFile(&model, &err, &warn, gltf_file.c_str());

	if (!importResult)
	{
		LOGE("Failed to load gltf file {}.", gltf_file.c_str());

		return nullptr;
	}

	if (!err.empty())
	{
		LOGE("Error loading gltf model: {}.", err.c_str());

		return nullptr;
	}

	if (!warn.empty())
	{
		LOGI("{}", warn.c_str());
	}

	size_t pos = file_name.find_last_of('/');

	model_path = file_name.substr(0, pos);

	if (pos == std::string::npos)
	{
		model_path.clear();
	}

	return std::make_unique<sg::Scene>(load_scene());
}

std::unique_ptr<sg::SubMesh> GLTFLoader::read_model_from_file(const std::string &file_name, uint32_t index)
{
	std::string err;
	std::string warn;

	tinygltf::TinyGLTF gltf_loader;

	std::string gltf_file = vkb::fs::path::get(vkb::fs::path::Type::Assets) + file_name;

	bool importResult = gltf_loader.LoadASCIIFromFile(&model, &err, &warn, gltf_file.c_str());

	if (!importResult)
	{
		LOGE("Failed to load gltf file {}.", gltf_file.c_str());

		return nullptr;
	}

	if (!err.empty())
	{
		LOGE("Error loading gltf model: {}.", err.c_str());

		return nullptr;
	}

	if (!warn.empty())
	{
		LOGI("{}", warn.c_str());
	}

	size_t pos = file_name.find_last_of('/');

	model_path = file_name.substr(0, pos);

	if (pos == std::string::npos)
	{
		model_path.clear();
	}

	return std::move(load_model(index));
}

sg::Scene GLTFLoader::load_scene()
{
	auto scene = sg::Scene();

	scene.set_name("gltf_scene");

	// Load samplers
	std::vector<std::unique_ptr<sg::Sampler>>
	    sampler_components(model.samplers.size());

	for (size_t sampler_index = 0; sampler_index < model.samplers.size(); sampler_index++)
	{
		auto sampler                      = parse_sampler(model.samplers.at(sampler_index));
		sampler_components[sampler_index] = std::move(sampler);
	}

	scene.set_components(std::move(sampler_components));

	Timer timer;
	timer.start();

	// Load images
	auto thread_count = std::thread::hardware_concurrency();
	thread_count      = thread_count == 0 ? 1 : thread_count;
	ctpl::thread_pool thread_pool(thread_count);

	auto image_count = to_u32(model.images.size());

	std::vector<std::future<std::unique_ptr<sg::Image>>> image_component_futures;
	for (size_t image_index = 0; image_index < image_count; image_index++)
	{
		auto fut = thread_pool.push(
		    [this, image_index](size_t) {
			    auto image = parse_image(model.images.at(image_index));

			    LOGI("Loaded gltf image #{} ({})", image_index, model.images.at(image_index).uri.c_str());

			    return image;
		    });

		image_component_futures.push_back(std::move(fut));
	}

	std::vector<std::unique_ptr<sg::Image>> image_components;
	for (auto &fut : image_component_futures)
	{
		image_components.push_back(fut.get());
	}

	// Upload images to GPU
	std::vector<core::Buffer> transient_buffers;

	auto &command_buffer = device.request_command_buffer();

	command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	for (size_t image_index = 0; image_index < image_count; image_index++)
	{
		auto &image = image_components.at(image_index);

		core::Buffer stage_buffer{device,
		                          image->get_data().size(),
		                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                          VMA_MEMORY_USAGE_CPU_ONLY};

		stage_buffer.update(image->get_data());

		upload_image_to_gpu(command_buffer, stage_buffer, *image);

		transient_buffers.push_back(std::move(stage_buffer));
	}

	command_buffer.end();

	auto &queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	queue.submit(command_buffer, device.request_fence());

	device.get_fence_pool().wait();
	device.get_fence_pool().reset();
	device.get_command_pool().reset_pool();
	device.wait_idle();

	transient_buffers.clear();

	scene.set_components(std::move(image_components));

	auto elapsed_time = timer.stop();

	LOGI("Time spent loading images: {} seconds across {} threads.", vkb::to_string(elapsed_time), thread_count);

	// Load textures
	auto images          = scene.get_components<sg::Image>();
	auto samplers        = scene.get_components<sg::Sampler>();
	auto default_sampler = create_default_sampler();

	for (auto &gltf_texture : model.textures)
	{
		auto texture = parse_texture(gltf_texture);

		texture->set_image(*images.at(gltf_texture.source));

		if (gltf_texture.sampler >= 0 && gltf_texture.sampler < static_cast<int>(samplers.size()))
		{
			texture->set_sampler(*samplers.at(gltf_texture.sampler));
		}
		else
		{
			if (gltf_texture.name.empty())
			{
				gltf_texture.name = images.at(gltf_texture.source)->get_name();
			}

			LOGW("Sampler not found for texture {}, possible GLTF error", gltf_texture.name);
			texture->set_sampler(*default_sampler);
		}

		scene.add_component(std::move(texture));
	}

	scene.add_component(std::move(default_sampler));

	// Load materials
	bool                            has_textures = scene.has_component<sg::Texture>();
	std::vector<vkb::sg::Texture *> textures;
	if (has_textures)
	{
		textures = scene.get_components<sg::Texture>();
	}
	
	for (auto &gltf_material : model.materials)
	{
		auto material = parse_material(gltf_material);

		for (auto &gltf_value : gltf_material.values)
		{
			if (gltf_value.first.find("Texture") != std::string::npos)
			{
				std::string tex_name = to_snake_case(gltf_value.first);

				material->textures[tex_name] = textures.at(gltf_value.second.TextureIndex());
			}
		}

		for (auto &gltf_value : gltf_material.additionalValues)
		{
			if (gltf_value.first.find("Texture") != std::string::npos)
			{
				std::string tex_name = to_snake_case(gltf_value.first);

				material->textures[tex_name] = textures.at(gltf_value.second.TextureIndex());
			}
		}

		scene.add_component(std::move(material));
	}

	auto default_material = create_default_material();

	// Load meshes
	auto materials = scene.get_components<sg::PBRMaterial>();

	command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	for (auto &gltf_mesh : model.meshes)
	{
		auto mesh = parse_mesh(gltf_mesh);

		for (auto &gltf_primitive : gltf_mesh.primitives)
		{
			auto submesh = std::make_unique<sg::SubMesh>();

			std::vector<glm::vec3> vertex_positions;
			std::vector<uint16_t>  vertex_indices;

			for (auto &attribute : gltf_primitive.attributes)
			{
				std::string attrib_name = attribute.first;
				std::transform(attrib_name.begin(), attrib_name.end(), attrib_name.begin(), ::tolower);

				// Read in buffer data
				auto vertex_data = get_attribute_data(&model, attribute.second);

				if (attrib_name == "position")
				{
					submesh->vertices_count = to_u32(model.accessors.at(attribute.second).count);

					const glm::vec3 *vertices = reinterpret_cast<glm::vec3 *>(vertex_data.data());
					vertex_positions          = {vertices, vertices + submesh->vertices_count};
				}

				core::Buffer stage_buffer{device,
				                          vertex_data.size(),
				                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				                          VMA_MEMORY_USAGE_CPU_ONLY};

				stage_buffer.update(vertex_data);

				core::Buffer buffer{device,
				                    vertex_data.size(),
				                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				                    VMA_MEMORY_USAGE_GPU_ONLY};

				command_buffer.copy_buffer(stage_buffer, buffer, vertex_data.size());

				auto pair = std::make_pair(attrib_name, std::move(buffer));
				submesh->vertex_buffers.insert(std::move(pair));

				transient_buffers.push_back(std::move(stage_buffer));

				sg::VertexAttribute attrib;
				attrib.format = get_attribute_format(&model, attribute.second);
				attrib.stride = to_u32(get_attribute_stride(&model, attribute.second));

				submesh->set_attribute(attrib_name, attrib);
			}

			if (gltf_primitive.indices >= 0)
			{
				submesh->vertex_indices = to_u32(get_attribute_size(&model, gltf_primitive.indices));

				auto format     = get_attribute_format(&model, gltf_primitive.indices);
				auto index_data = get_attribute_data(&model, gltf_primitive.indices);

				switch (format)
				{
					case VK_FORMAT_R8_UINT:
						index_data          = convert_data(index_data, 1, 2);
						submesh->index_type = VK_INDEX_TYPE_UINT16;
						break;
					case VK_FORMAT_R16_UINT:
						submesh->index_type = VK_INDEX_TYPE_UINT16;
						break;
					case VK_FORMAT_R32_UINT:
						submesh->index_type = VK_INDEX_TYPE_UINT32;
						break;
					default:
						LOGE("gltf primitive has invalid format type");
						break;
				}

				const uint16_t *indices = reinterpret_cast<uint16_t *>(index_data.data());
				vertex_indices          = {indices, indices + submesh->vertex_indices};

				core::Buffer stage_buffer{device,
				                          index_data.size(),
				                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				                          VMA_MEMORY_USAGE_CPU_ONLY};

				stage_buffer.update(index_data);

				submesh->index_buffer = std::make_unique<core::Buffer>(device,
				                                                       index_data.size(),
				                                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				                                                       VMA_MEMORY_USAGE_GPU_ONLY);

				command_buffer.copy_buffer(stage_buffer, *submesh->index_buffer, index_data.size());

				transient_buffers.push_back(std::move(stage_buffer));
			}

			if (gltf_primitive.material < 0)
			{
				submesh->set_material(*default_material);
			}
			else
			{
				submesh->set_material(*materials.at(gltf_primitive.material));
			}

			// Only update if there are vertices existing in the position buffer
			if (vertex_positions.size() > 0)
			{
				mesh->update_bounds(vertex_positions, vertex_indices);
			}

			mesh->add_submesh(*submesh);

			scene.add_component(std::move(submesh));
		}

		scene.add_component(std::move(mesh));
	}

	command_buffer.end();

	queue.submit(command_buffer, device.request_fence());

	device.get_fence_pool().wait();
	device.get_fence_pool().reset();
	device.get_command_pool().reset_pool();

	transient_buffers.clear();

	scene.add_component(std::move(default_material));

	// Load cameras
	for (auto &gltf_camera : model.cameras)
	{
		auto camera = parse_camera(gltf_camera);
		scene.add_component(std::move(camera));
	}

	// Load nodes
	auto meshes = scene.get_components<sg::Mesh>();

	std::vector<std::unique_ptr<sg::Node>> nodes;

	for (size_t node_index = 0; node_index < model.nodes.size(); ++node_index)
	{
		auto gltf_node = model.nodes[node_index];
		auto node      = parse_node(gltf_node, node_index);

		if (gltf_node.mesh >= 0)
		{
			auto mesh = meshes.at(gltf_node.mesh);

			node->set_component(*mesh);

			mesh->add_node(*node);
		}

		if (gltf_node.camera >= 0)
		{
			auto cameras = scene.get_components<sg::Camera>();
			auto camera  = cameras.at(gltf_node.camera);

			node->set_component(*camera);

			camera->set_node(*node);
		}

		nodes.push_back(std::move(node));
	}

	// Load scenes
	std::queue<std::pair<sg::Node &, int>> traverse_nodes;

	for (auto &gltf_scene : model.scenes)
	{
		auto root_node = std::make_unique<sg::Node>(0, gltf_scene.name);

		for (auto node_index : gltf_scene.nodes)
		{
			traverse_nodes.push(std::make_pair(std::ref(*root_node), node_index));
		}

		while (!traverse_nodes.empty())
		{
			auto node_it = traverse_nodes.front();
			traverse_nodes.pop();

			auto &current_node       = *nodes.at(node_it.second);
			auto &traverse_root_node = node_it.first;

			current_node.set_parent(traverse_root_node);
			traverse_root_node.add_child(current_node);

			for (auto child_node_index : model.nodes[node_it.second].children)
			{
				traverse_nodes.push(std::make_pair(std::ref(traverse_root_node), child_node_index));
			}
		}

		scene.add_child(*root_node);
		nodes.push_back(std::move(root_node));
	}

	// Store nodes into the scene
	scene.set_nodes(std::move(nodes));

	// Create node for the default camera
	auto camera_node = std::make_unique<sg::Node>(-1, "default_camera");

	auto default_camera = create_default_camera();
	default_camera->set_node(*camera_node);
	camera_node->set_component(*default_camera);
	scene.add_component(std::move(default_camera));

	scene.add_child(*camera_node);
	scene.add_node(std::move(camera_node));

	return scene;
}

std::unique_ptr<sg::SubMesh> GLTFLoader::load_model(uint32_t index)
{
	auto submesh = std::make_unique<sg::SubMesh>();

	std::vector<core::Buffer> transient_buffers;

	auto &queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	auto &command_buffer = device.request_command_buffer();

	command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	auto &gltf_mesh = model.meshes.at(index);

	auto &gltf_primitive = gltf_mesh.primitives.at(0);

	std::vector<Vertex> vertex_data;

	const float *   pos     = nullptr;
	const float *   normals = nullptr;
	const float *   uvs     = nullptr;
	const uint16_t *joints  = nullptr;
	const float *   weights = nullptr;

	// Position attribute is required
	auto & accessor     = model.accessors[gltf_primitive.attributes.find("POSITION")->second];
	size_t vertex_count = accessor.count;
	auto & buffer_view  = model.bufferViews[accessor.bufferView];
	pos                 = reinterpret_cast<const float *>(&(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));

	if (gltf_primitive.attributes.find("NORMAL") != gltf_primitive.attributes.end())
	{
		accessor    = model.accessors[gltf_primitive.attributes.find("NORMAL")->second];
		buffer_view = model.bufferViews[accessor.bufferView];
		normals     = reinterpret_cast<const float *>(&(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
	}

	if (gltf_primitive.attributes.find("TEXCOORD_0") != gltf_primitive.attributes.end())
	{
		accessor    = model.accessors[gltf_primitive.attributes.find("TEXCOORD_0")->second];
		buffer_view = model.bufferViews[accessor.bufferView];
		uvs         = reinterpret_cast<const float *>(&(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
	}

	// Skinning
	// Joints
	if (gltf_primitive.attributes.find("JOINTS_0") != gltf_primitive.attributes.end())
	{
		accessor    = model.accessors[gltf_primitive.attributes.find("JOINTS_0")->second];
		buffer_view = model.bufferViews[accessor.bufferView];
		joints      = reinterpret_cast<const uint16_t *>(&(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
	}

	if (gltf_primitive.attributes.find("WEIGHTS_0") != gltf_primitive.attributes.end())
	{
		accessor    = model.accessors[gltf_primitive.attributes.find("WEIGHTS_0")->second];
		buffer_view = model.bufferViews[accessor.bufferView];
		weights     = reinterpret_cast<const float *>(&(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
	}

	bool has_skin = (joints && weights);

	for (size_t v = 0; v < vertex_count; v++)
	{
		Vertex vert{};
		vert.pos    = glm::vec4(glm::make_vec3(&pos[v * 3]), 1.0f);
		vert.normal = glm::normalize(glm::vec3(normals ? glm::make_vec3(&normals[v * 3]) : glm::vec3(0.0f)));
		vert.uv     = uvs ? glm::make_vec2(&uvs[v * 2]) : glm::vec3(0.0f);

		vert.joint0  = has_skin ? glm::vec4(glm::make_vec4(&joints[v * 4])) : glm::vec4(0.0f);
		vert.weight0 = has_skin ? glm::make_vec4(&weights[v * 4]) : glm::vec4(0.0f);
		vertex_data.push_back(vert);
	}

	core::Buffer stage_buffer{device,
	                          vertex_data.size() * sizeof(Vertex),
	                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                          VMA_MEMORY_USAGE_CPU_ONLY};

	stage_buffer.update(vertex_data.data(), vertex_data.size() * sizeof(Vertex));

	core::Buffer buffer{device,
	                    vertex_data.size() * sizeof(Vertex),
	                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                    VMA_MEMORY_USAGE_GPU_ONLY};

	command_buffer.copy_buffer(stage_buffer, buffer, vertex_data.size() * sizeof(Vertex));

	auto pair = std::make_pair("vertex_buffer", std::move(buffer));
	submesh->vertex_buffers.insert(std::move(pair));

	transient_buffers.push_back(std::move(stage_buffer));

	if (gltf_primitive.indices >= 0)
	{
		submesh->vertex_indices = to_u32(get_attribute_size(&model, gltf_primitive.indices));

		auto format     = get_attribute_format(&model, gltf_primitive.indices);
		auto index_data = get_attribute_data(&model, gltf_primitive.indices);

		switch (format)
		{
			case VK_FORMAT_R32_UINT:
			{
				// Correct format
				break;
			}
			case VK_FORMAT_R16_UINT:
			{
				index_data = convert_data(index_data, 2, 4);
				break;
			}
			case VK_FORMAT_R8_UINT:
			{
				index_data = convert_data(index_data, 1, 4);
				break;
			}
			default:
			{
				break;
			}
		}

		//Always do uint32
		submesh->index_type = VK_INDEX_TYPE_UINT32;

		core::Buffer stage_buffer{device,
		                          index_data.size(),
		                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                          VMA_MEMORY_USAGE_CPU_ONLY};

		stage_buffer.update(index_data);

		submesh->index_buffer = std::make_unique<core::Buffer>(device,
		                                                       index_data.size(),
		                                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		                                                       VMA_MEMORY_USAGE_GPU_ONLY);

		command_buffer.copy_buffer(stage_buffer, *submesh->index_buffer, index_data.size());

		transient_buffers.push_back(std::move(stage_buffer));
	}

	command_buffer.end();

	queue.submit(command_buffer, device.request_fence());

	device.get_fence_pool().wait();
	device.get_fence_pool().reset();
	device.get_command_pool().reset_pool();

	return std::move(submesh);
}

std::unique_ptr<sg::Node> GLTFLoader::parse_node(const tinygltf::Node &gltf_node, size_t index) const
{
	auto node = std::make_unique<sg::Node>(index, gltf_node.name);

	auto &transform = node->get_component<sg::Transform>();

	if (!gltf_node.translation.empty())
	{
		glm::vec3 translation;

		std::transform(gltf_node.translation.begin(), gltf_node.translation.end(), glm::value_ptr(translation), TypeCast<double, float>{});

		transform.set_translation(translation);
	}

	if (!gltf_node.rotation.empty())
	{
		glm::quat rotation;

		std::transform(gltf_node.rotation.begin(), gltf_node.rotation.end(), glm::value_ptr(rotation), TypeCast<double, float>{});

		transform.set_rotation(rotation);
	}

	if (!gltf_node.scale.empty())
	{
		glm::vec3 scale;

		std::transform(gltf_node.scale.begin(), gltf_node.scale.end(), glm::value_ptr(scale), TypeCast<double, float>{});

		transform.set_scale(scale);
	}

	if (!gltf_node.matrix.empty())
	{
		glm::mat4 matrix;

		std::transform(gltf_node.matrix.begin(), gltf_node.matrix.end(), glm::value_ptr(matrix), TypeCast<double, float>{});

		transform.set_matrix(matrix);
	}

	return node;
}

std::unique_ptr<sg::Camera> GLTFLoader::parse_camera(const tinygltf::Camera &gltf_camera) const
{
	std::unique_ptr<sg::Camera> camera;

	if (gltf_camera.type == "perspective")
	{
		auto perspective_camera = std::make_unique<sg::PerspectiveCamera>(gltf_camera.name);

		perspective_camera->set_aspect_ratio(static_cast<float>(gltf_camera.perspective.aspectRatio));
		perspective_camera->set_field_of_view(static_cast<float>(gltf_camera.perspective.yfov));
		perspective_camera->set_near_plane(static_cast<float>(gltf_camera.perspective.znear));
		perspective_camera->set_far_plane(static_cast<float>(gltf_camera.perspective.zfar));

		camera = std::move(perspective_camera);
	}
	else
	{
		LOGW("Camera type not supported");
	}

	return camera;
}

std::unique_ptr<sg::Mesh> GLTFLoader::parse_mesh(const tinygltf::Mesh &gltf_mesh) const
{
	return std::make_unique<sg::Mesh>(gltf_mesh.name);
}

std::unique_ptr<sg::PBRMaterial> GLTFLoader::parse_material(const tinygltf::Material &gltf_material) const
{
	auto material = std::make_unique<sg::PBRMaterial>(gltf_material.name);

	for (auto &gltf_value : gltf_material.values)
	{
		if (gltf_value.first == "baseColorFactor")
		{
			const auto &color_factor    = gltf_value.second.ColorFactor();
			material->base_color_factor = glm::vec4(color_factor[0], color_factor[1], color_factor[2], color_factor[3]);
		}
		else if (gltf_value.first == "metallicFactor")
		{
			material->metallic_factor = static_cast<float>(gltf_value.second.Factor());
		}
		else if (gltf_value.first == "roughnessFactor")
		{
			material->roughness_factor = static_cast<float>(gltf_value.second.Factor());
		}
	}

	for (auto &gltf_value : gltf_material.additionalValues)
	{
		if (gltf_value.first == "emissiveFactor")
		{
			const auto &emissive_factor = gltf_value.second.number_array;

			material->emissive = glm::vec3(emissive_factor[0], emissive_factor[1], emissive_factor[2]);
		}
		else if (gltf_value.first == "alphaMode")
		{
			if (gltf_value.second.string_value == "BLEND")
			{
				material->alpha_mode = vkb::sg::AlphaMode::Blend;
			}
			else if (gltf_value.second.string_value == "OPAQUE")
			{
				material->alpha_mode = vkb::sg::AlphaMode::Opaque;
			}
			else if (gltf_value.second.string_value == "MASK")
			{
				material->alpha_mode = vkb::sg::AlphaMode::Mask;
			}
		}
		else if (gltf_value.first == "alphaCutoff")
		{
			material->alpha_cutoff = static_cast<float>(gltf_value.second.number_value);
		}
		else if (gltf_value.first == "doubleSided")
		{
			material->double_sided = gltf_value.second.bool_value;
		}
	}

	return material;
}

std::unique_ptr<sg::Image> GLTFLoader::parse_image(tinygltf::Image &gltf_image) const
{
	std::unique_ptr<sg::Image> image{nullptr};

	if (!gltf_image.image.empty())
	{
		// Image embedded in gltf file
		auto mipmap = sg::Mipmap{
		    /* .level = */ 0,
		    /* .offset = */ 0,
		    /* .extent = */ {/* .width = */ static_cast<uint32_t>(gltf_image.width),
		                     /* .height = */ static_cast<uint32_t>(gltf_image.height),
		                     /* .depth = */ 1u}};
		std::vector<sg::Mipmap> mipmaps{mipmap};
		image = std::make_unique<sg::Image>(gltf_image.name, std::move(gltf_image.image), std::move(mipmaps));
	}
	else
	{
		// Load image from uri
		auto image_uri = model_path + "/" + gltf_image.uri;
		image          = sg::Image::load(gltf_image.name, image_uri);
	}

	// Check whether the format is supported by the GPU
	if (sg::is_astc(image->get_format()))
	{
		if (!device.is_image_format_supported(image->get_format()))
		{
			LOGW("ASTC not supported: decoding {}", image->get_name());
			image = std::make_unique<sg::Astc>(*image);
			image->generate_mipmaps();
		}
	}

	image->create_vk_image(device);

	return image;
}

std::unique_ptr<sg::Sampler> GLTFLoader::parse_sampler(const tinygltf::Sampler &gltf_sampler) const
{
	auto name = gltf_sampler.name;

	VkFilter min_filter = find_min_filter(gltf_sampler.minFilter);
	VkFilter mag_filter = find_mag_filter(gltf_sampler.magFilter);

	VkSamplerMipmapMode mipmap_mode = find_mipmap_mode(gltf_sampler.minFilter);

	VkSamplerAddressMode address_mode_u = find_wrap_mode(gltf_sampler.wrapS);
	VkSamplerAddressMode address_mode_v = find_wrap_mode(gltf_sampler.wrapT);
	VkSamplerAddressMode address_mode_w = find_wrap_mode(gltf_sampler.wrapR);

	VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	sampler_info.magFilter    = mag_filter;
	sampler_info.minFilter    = min_filter;
	sampler_info.mipmapMode   = mipmap_mode;
	sampler_info.addressModeU = address_mode_u;
	sampler_info.addressModeV = address_mode_v;
	sampler_info.addressModeW = address_mode_w;
	sampler_info.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler_info.maxLod       = std::numeric_limits<float>::max();

	core::Sampler vk_sampler{device, sampler_info};

	return std::make_unique<sg::Sampler>(name, std::move(vk_sampler));
}

std::unique_ptr<sg::Texture> GLTFLoader::parse_texture(const tinygltf::Texture &gltf_texture) const
{
	return std::make_unique<sg::Texture>(gltf_texture.name);
}

std::unique_ptr<sg::PBRMaterial> GLTFLoader::create_default_material()
{
	tinygltf::Material gltf_material;
	return parse_material(gltf_material);
}

std::unique_ptr<sg::Sampler> GLTFLoader::create_default_sampler()
{
	tinygltf::Sampler gltf_sampler;

	gltf_sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
	gltf_sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;

	gltf_sampler.wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
	gltf_sampler.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
	gltf_sampler.wrapR = TINYGLTF_TEXTURE_WRAP_REPEAT;

	return parse_sampler(gltf_sampler);
}

std::unique_ptr<sg::Camera> GLTFLoader::create_default_camera()
{
	tinygltf::Camera gltf_camera;

	gltf_camera.name = "default_camera";
	gltf_camera.type = "perspective";

	gltf_camera.perspective.aspectRatio = 1.77f;
	gltf_camera.perspective.yfov        = 1.0f;
	gltf_camera.perspective.znear       = 0.1f;
	gltf_camera.perspective.zfar        = 1000.0f;

	return parse_camera(gltf_camera);
}
}        // namespace vkb
