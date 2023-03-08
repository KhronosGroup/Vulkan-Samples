/* Copyright (c) 2018-2023, Arm Limited and Contributors
 * Copyright (c) 2019-2023, Sascha Willems
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
#if defined(__ANDROID__)
#include "platform/android/asset_manager.h"
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#include "gltf_loader.h"

#include <limits>
#include <queue>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
#include <glm/gtc/type_ptr.hpp>
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
#include "scene_graph/components/light.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/perspective_camera.h"
#include "scene_graph/components/sampler.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/components/transform.h"
#include "scene_graph/node.h"
#include "scene_graph/scene.h"
#include "scene_graph/scripts/animation.h"

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
	assert(accessorId < model->accessors.size());
	auto &accessor = model->accessors[accessorId];
	assert(accessor.bufferView < model->bufferViews.size());
	auto &bufferView = model->bufferViews[accessor.bufferView];
	assert(bufferView.buffer < model->buffers.size());
	auto &buffer = model->buffers[bufferView.buffer];

	size_t stride    = accessor.ByteStride(bufferView);
	size_t startByte = accessor.byteOffset + bufferView.byteOffset;
	size_t endByte   = startByte + accessor.count * stride;

	return {buffer.data.begin() + startByte, buffer.data.begin() + endByte};
};

inline size_t get_attribute_size(const tinygltf::Model *model, uint32_t accessorId)
{
	assert(accessorId < model->accessors.size());
	return model->accessors[accessorId].count;
};

inline size_t get_attribute_stride(const tinygltf::Model *model, uint32_t accessorId)
{
	assert(accessorId < model->accessors.size());
	auto &accessor = model->accessors[accessorId];
	assert(accessor.bufferView < model->bufferViews.size());
	auto &bufferView = model->bufferViews[accessor.bufferView];

	return accessor.ByteStride(bufferView);
};

inline VkFormat get_attribute_format(const tinygltf::Model *model, uint32_t accessorId)
{
	assert(accessorId < model->accessors.size());
	auto &accessor = model->accessors[accessorId];

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

inline std::vector<uint8_t> convert_underlying_data_stride(const std::vector<uint8_t> &src_data, uint32_t src_stride, uint32_t dst_stride)
{
	auto elem_count = to_u32(src_data.size()) / src_stride;

	std::vector<uint8_t> result(elem_count * dst_stride);

	for (uint32_t idxSrc = 0, idxDst = 0;
	     idxSrc < src_data.size() && idxDst < result.size();
	     idxSrc += src_stride, idxDst += dst_stride)
	{
		std::copy(src_data.begin() + idxSrc, src_data.begin() + idxSrc + src_stride, result.begin() + idxDst);
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
		memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		command_buffer.image_memory_barrier(image.get_vk_image_view(), memory_barrier);
	}
}

static inline bool texture_needs_srgb_colorspace(const std::string &name)
{
	// The gltf spec states that the base and emissive textures MUST be encoded with the sRGB
	// transfer function. All other texture types are linear.
	if (name == "baseColorTexture" || name == "emissiveTexture")
	{
		return true;
	}

	// metallicRoughnessTexture, normalTexture & occlusionTexture must be linear
	assert(name == "metallicRoughnessTexture" || name == "normalTexture" || name == "occlusionTexture");
	return false;
}

}        // namespace

std::unordered_map<std::string, bool> GLTFLoader::supported_extensions = {
    {KHR_LIGHTS_PUNCTUAL_EXTENSION, false}};

GLTFLoader::GLTFLoader(Device const &device) :
    device{device}
{
}

std::unique_ptr<sg::Scene> GLTFLoader::read_scene_from_file(const std::string &file_name, int scene_index)
{
	std::string err;
	std::string warn;

	tinygltf::TinyGLTF gltf_loader;

	std::string gltf_file = vkb::fs::path::get(vkb::fs::path::Type::Assets) + file_name;
#if defined(__ANDROID__)
	if(tinygltf::asset_manager == nullptr)
	{
		tinygltf::asset_manager = vkb::fs::AssetManager::get_android_asset_manager();
	}
#endif

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

	return std::make_unique<sg::Scene>(load_scene(scene_index));
}

std::unique_ptr<sg::SubMesh> GLTFLoader::read_model_from_file(const std::string &file_name, uint32_t index)
{
	std::string err;
	std::string warn;

	tinygltf::TinyGLTF gltf_loader;

	std::string gltf_file = vkb::fs::path::get(vkb::fs::path::Type::Assets) + file_name;
#if defined(__ANDROID__)
	if(tinygltf::asset_manager == nullptr)
	{
		tinygltf::asset_manager = vkb::fs::AssetManager::get_android_asset_manager();
	}
#endif

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

sg::Scene GLTFLoader::load_scene(int scene_index)
{
	auto scene = sg::Scene();

	scene.set_name("gltf_scene");

	// Check extensions
	for (auto &used_extension : model.extensionsUsed)
	{
		auto it = supported_extensions.find(used_extension);

		// Check if extension isn't supported by the GLTFLoader
		if (it == supported_extensions.end())
		{
			// If extension is required then we shouldn't allow the scene to be loaded
			if (std::find(model.extensionsRequired.begin(), model.extensionsRequired.end(), used_extension) != model.extensionsRequired.end())
			{
				throw std::runtime_error("Cannot load glTF file. Contains a required unsupported extension: " + used_extension);
			}
			else
			{
				// Otherwise, if extension isn't required (but is in the file) then print a warning to the user
				LOGW("glTF file contains an unsupported extension, unexpected results may occur: {}", used_extension);
			}
		}
		else
		{
			// Extension is supported, so enable it
			LOGI("glTF file contains extension: {}", used_extension);
			it->second = true;
		}
	}

	// Load lights
	std::vector<std::unique_ptr<sg::Light>> light_components = parse_khr_lights_punctual();

	scene.set_components(std::move(light_components));

	// Load samplers
	std::vector<std::unique_ptr<sg::Sampler>>
	    sampler_components(model.samplers.size());

	for (size_t sampler_index = 0; sampler_index < model.samplers.size(); sampler_index++)
	{
		auto sampler                      = parse_sampler(model.samplers[sampler_index]);
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
			    auto image = parse_image(model.images[image_index]);

			    LOGI("Loaded gltf image #{} ({})", image_index, model.images[image_index].uri.c_str());

			    return image;
		    });

		image_component_futures.push_back(std::move(fut));
	}

	std::vector<std::unique_ptr<sg::Image>> image_components;

	// Upload images to GPU. We do this in batches of 64MB of data to avoid needing
	// double the amount of memory (all the images and all the corresponding buffers).
	// This helps keep memory footprint lower which is helpful on smaller devices.
	size_t image_index = 0;
	while (image_index < image_count)
	{
		std::vector<core::Buffer> transient_buffers;

		auto &command_buffer = device.request_command_buffer();

		command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);

		size_t batch_size = 0;

		// Deal with 64MB of image data at a time to keep memory footprint low
		while (image_index < image_count && batch_size < 64 * 1024 * 1024)
		{
			// Wait for this image to complete loading, then stage for upload
			image_components.push_back(image_component_futures[image_index].get());

			auto &image = image_components[image_index];

			core::Buffer stage_buffer{device,
			                          image->get_data().size(),
			                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			                          VMA_MEMORY_USAGE_CPU_ONLY};

			batch_size += image->get_data().size();

			stage_buffer.update(image->get_data());

			upload_image_to_gpu(command_buffer, stage_buffer, *image);

			transient_buffers.push_back(std::move(stage_buffer));

			image_index++;
		}

		command_buffer.end();

		auto &queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

		queue.submit(command_buffer, device.request_fence());

		device.get_fence_pool().wait();
		device.get_fence_pool().reset();
		device.get_command_pool().reset_pool();
		device.wait_idle();

		// Remove the staging buffers for the batch we just processed
		transient_buffers.clear();
	}

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

		assert(gltf_texture.source < images.size());
		texture->set_image(*images[gltf_texture.source]);

		if (gltf_texture.sampler >= 0 && gltf_texture.sampler < static_cast<int>(samplers.size()))
		{
			texture->set_sampler(*samplers[gltf_texture.sampler]);
		}
		else
		{
			if (gltf_texture.name.empty())
			{
				gltf_texture.name = images[gltf_texture.source]->get_name();
			}

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

				assert(gltf_value.second.TextureIndex() < textures.size());
				vkb::sg::Texture *tex = textures[gltf_value.second.TextureIndex()];

				if (texture_needs_srgb_colorspace(gltf_value.first))
				{
					tex->get_image()->coerce_format_to_srgb();
				}

				material->textures[tex_name] = tex;
			}
		}

		for (auto &gltf_value : gltf_material.additionalValues)
		{
			if (gltf_value.first.find("Texture") != std::string::npos)
			{
				std::string tex_name = to_snake_case(gltf_value.first);

				assert(gltf_value.second.TextureIndex() < textures.size());
				vkb::sg::Texture *tex = textures[gltf_value.second.TextureIndex()];

				if (texture_needs_srgb_colorspace(gltf_value.first))
				{
					tex->get_image()->coerce_format_to_srgb();
				}

				material->textures[tex_name] = tex;
			}
		}

		scene.add_component(std::move(material));
	}

	auto default_material = create_default_material();

	// Load meshes
	auto materials = scene.get_components<sg::PBRMaterial>();

	for (auto &gltf_mesh : model.meshes)
	{
		auto mesh = parse_mesh(gltf_mesh);

		for (size_t i_primitive = 0; i_primitive < gltf_mesh.primitives.size(); i_primitive++)
		{
			const auto &gltf_primitive = gltf_mesh.primitives[i_primitive];

			auto submesh_name = fmt::format("'{}' mesh, primitive #{}", gltf_mesh.name, i_primitive);
			auto submesh      = std::make_unique<sg::SubMesh>(std::move(submesh_name));

			for (auto &attribute : gltf_primitive.attributes)
			{
				std::string attrib_name = attribute.first;
				std::transform(attrib_name.begin(), attrib_name.end(), attrib_name.begin(), ::tolower);

				auto vertex_data = get_attribute_data(&model, attribute.second);

				if (attrib_name == "position")
				{
					assert(attribute.second < model.accessors.size());
					submesh->vertices_count = to_u32(model.accessors[attribute.second].count);
				}

				core::Buffer buffer{device,
				                    vertex_data.size(),
				                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				                    VMA_MEMORY_USAGE_GPU_TO_CPU};
				buffer.update(vertex_data);
				buffer.set_debug_name(fmt::format("'{}' mesh, primitive #{}: '{}' vertex buffer",
				                                  gltf_mesh.name, i_primitive, attrib_name));

				submesh->vertex_buffers.insert(std::make_pair(attrib_name, std::move(buffer)));

				sg::VertexAttribute attrib;
				attrib.format = get_attribute_format(&model, attribute.second);
				attrib.stride = to_u32(get_attribute_stride(&model, attribute.second));

				submesh->set_attribute(attrib_name, attrib);
			}

			if (gltf_primitive.indices >= 0)
			{
				submesh->vertex_indices = to_u32(get_attribute_size(&model, gltf_primitive.indices));

				auto format = get_attribute_format(&model, gltf_primitive.indices);

				auto index_data = get_attribute_data(&model, gltf_primitive.indices);

				switch (format)
				{
					case VK_FORMAT_R8_UINT:
						// Converts uint8 data into uint16 data, still represented by a uint8 vector
						index_data          = convert_underlying_data_stride(index_data, 1, 2);
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

				submesh->index_buffer = std::make_unique<core::Buffer>(device,
				                                                       index_data.size(),
				                                                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				                                                       VMA_MEMORY_USAGE_GPU_TO_CPU);
				submesh->index_buffer->set_debug_name(fmt::format("'{}' mesh, primitive #{}: index buffer",
				                                                  gltf_mesh.name, i_primitive));

				submesh->index_buffer->update(index_data);
			}
			else
			{
				submesh->vertices_count = to_u32(get_attribute_size(&model, gltf_primitive.attributes.at("POSITION")));
			}

			if (gltf_primitive.material < 0)
			{
				submesh->set_material(*default_material);
			}
			else
			{
				assert(gltf_primitive.material < materials.size());
				submesh->set_material(*materials[gltf_primitive.material]);
			}

			mesh->add_submesh(*submesh);

			scene.add_component(std::move(submesh));
		}

		scene.add_component(std::move(mesh));
	}

	device.get_fence_pool().wait();
	device.get_fence_pool().reset();
	device.get_command_pool().reset_pool();

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
			assert(gltf_node.mesh < meshes.size());
			auto mesh = meshes[gltf_node.mesh];

			node->set_component(*mesh);

			mesh->add_node(*node);
		}

		if (gltf_node.camera >= 0)
		{
			auto cameras = scene.get_components<sg::Camera>();
			assert(gltf_node.camera < cameras.size());
			auto camera = cameras[gltf_node.camera];

			node->set_component(*camera);

			camera->set_node(*node);
		}

		if (auto extension = get_extension(gltf_node.extensions, KHR_LIGHTS_PUNCTUAL_EXTENSION))
		{
			auto lights      = scene.get_components<sg::Light>();
			int  light_index = extension->Get("light").Get<int>();
			assert(light_index < lights.size());
			auto light = lights[light_index];

			node->set_component(*light);

			light->set_node(*node);
		}

		nodes.push_back(std::move(node));
	}

	std::vector<std::unique_ptr<sg::Animation>> animations;

	// Load animations
	for (size_t animation_index = 0; animation_index < model.animations.size(); ++animation_index)
	{
		auto &gltf_animation = model.animations[animation_index];

		std::vector<sg::AnimationSampler> samplers;

		for (size_t sampler_index = 0; sampler_index < gltf_animation.samplers.size(); ++sampler_index)
		{
			auto gltf_sampler = gltf_animation.samplers[sampler_index];

			sg::AnimationSampler sampler;
			if (gltf_sampler.interpolation == "LINEAR")
			{
				sampler.type = sg::AnimationType::Linear;
			}
			else if (gltf_sampler.interpolation == "STEP")
			{
				sampler.type = sg::AnimationType::Step;
			}
			else if (gltf_sampler.interpolation == "CUBICSPLINE")
			{
				sampler.type = sg::AnimationType::CubicSpline;
			}
			else
			{
				LOGW("Gltf animation sampler #{} has unknown interpolation value", sampler_index);
			}

			auto input_accessor      = model.accessors[gltf_sampler.input];
			auto input_accessor_data = get_attribute_data(&model, gltf_sampler.input);

			const float *data = reinterpret_cast<const float *>(input_accessor_data.data());
			for (size_t i = 0; i < input_accessor.count; ++i)
			{
				sampler.inputs.push_back(data[i]);
			}

			auto output_accessor      = model.accessors[gltf_sampler.output];
			auto output_accessor_data = get_attribute_data(&model, gltf_sampler.output);

			switch (output_accessor.type)
			{
				case TINYGLTF_TYPE_VEC3:
				{
					const glm::vec3 *data = reinterpret_cast<const glm::vec3 *>(output_accessor_data.data());
					for (size_t i = 0; i < output_accessor.count; ++i)
					{
						sampler.outputs.push_back(glm::vec4(data[i], 0.0f));
					}
					break;
				}
				case TINYGLTF_TYPE_VEC4:
				{
					const glm::vec4 *data = reinterpret_cast<const glm::vec4 *>(output_accessor_data.data());
					for (size_t i = 0; i < output_accessor.count; ++i)
					{
						sampler.outputs.push_back(glm::vec4(data[i]));
					}
					break;
				}
				default:
				{
					LOGW("Gltf animation sampler #{} has unknown output data type", sampler_index);
					continue;
				}
			}

			samplers.push_back(sampler);
		}

		auto animation = std::make_unique<sg::Animation>(gltf_animation.name);

		for (size_t channel_index = 0; channel_index < gltf_animation.channels.size(); ++channel_index)
		{
			auto &gltf_channel = gltf_animation.channels[channel_index];

			sg::AnimationTarget target;
			if (gltf_channel.target_path == "translation")
			{
				target = sg::AnimationTarget::Translation;
			}
			else if (gltf_channel.target_path == "rotation")
			{
				target = sg::AnimationTarget::Rotation;
			}
			else if (gltf_channel.target_path == "scale")
			{
				target = sg::AnimationTarget::Scale;
			}
			else if (gltf_channel.target_path == "weights")
			{
				LOGW("Gltf animation channel #{} has unsupported target path: {}", channel_index, gltf_channel.target_path);
				continue;
			}
			else
			{
				LOGW("Gltf animation channel #{} has unknown target path", channel_index);
				continue;
			}

			float start_time{std::numeric_limits<float>::max()};
			float end_time{std::numeric_limits<float>::min()};

			for (auto input : samplers[gltf_channel.sampler].inputs)
			{
				if (input < start_time)
				{
					start_time = input;
				}
				if (input > end_time)
				{
					end_time = input;
				}
			}

			animation->update_times(start_time, end_time);

			animation->add_channel(*nodes[gltf_channel.target_node], target, samplers[gltf_channel.sampler]);
		}

		animations.push_back(std::move(animation));
	}

	scene.set_components(std::move(animations));

	// Load scenes
	std::queue<std::pair<sg::Node &, int>> traverse_nodes;

	tinygltf::Scene *gltf_scene{nullptr};

	if (scene_index >= 0 && scene_index < static_cast<int>(model.scenes.size()))
	{
		gltf_scene = &model.scenes[scene_index];
	}
	else if (model.defaultScene >= 0 && model.defaultScene < static_cast<int>(model.scenes.size()))
	{
		gltf_scene = &model.scenes[model.defaultScene];
	}
	else if (model.scenes.size() > 0)
	{
		gltf_scene = &model.scenes[0];
	}

	if (!gltf_scene)
	{
		throw std::runtime_error("Couldn't determine which scene to load!");
	}

	auto root_node = std::make_unique<sg::Node>(0, gltf_scene->name);

	for (auto node_index : gltf_scene->nodes)
	{
		traverse_nodes.push(std::make_pair(std::ref(*root_node), node_index));
	}

	while (!traverse_nodes.empty())
	{
		auto node_it = traverse_nodes.front();
		traverse_nodes.pop();

		assert(node_it.second < nodes.size());
		auto &current_node       = *nodes[node_it.second];
		auto &traverse_root_node = node_it.first;

		current_node.set_parent(traverse_root_node);
		traverse_root_node.add_child(current_node);

		for (auto child_node_index : model.nodes[node_it.second].children)
		{
			traverse_nodes.push(std::make_pair(std::ref(current_node), child_node_index));
		}
	}

	scene.set_root_node(*root_node);
	nodes.push_back(std::move(root_node));

	// Store nodes into the scene
	scene.set_nodes(std::move(nodes));

	// Create node for the default camera
	auto camera_node = std::make_unique<sg::Node>(-1, "default_camera");

	auto default_camera = create_default_camera();
	default_camera->set_node(*camera_node);
	camera_node->set_component(*default_camera);
	scene.add_component(std::move(default_camera));

	scene.get_root_node().add_child(*camera_node);
	scene.add_node(std::move(camera_node));

	if (!scene.has_component<vkb::sg::Light>())
	{
		// Add a default light if none are present
		vkb::add_directional_light(scene, glm::quat({glm::radians(-90.0f), 0.0f, glm::radians(30.0f)}));
	}

	return scene;
}

std::unique_ptr<sg::SubMesh> GLTFLoader::load_model(uint32_t index)
{
	auto submesh = std::make_unique<sg::SubMesh>();

	std::vector<core::Buffer> transient_buffers;

	auto &queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	auto &command_buffer = device.request_command_buffer();

	command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	assert(index < model.meshes.size());
	auto &gltf_mesh = model.meshes[index];

	assert(!gltf_mesh.primitives.empty());
	auto &gltf_primitive = gltf_mesh.primitives[0];

	std::vector<Vertex> vertex_data;

	const float    *pos     = nullptr;
	const float    *normals = nullptr;
	const float    *uvs     = nullptr;
	const uint16_t *joints  = nullptr;
	const float    *weights = nullptr;

	// Position attribute is required
	auto  &accessor     = model.accessors[gltf_primitive.attributes.find("POSITION")->second];
	size_t vertex_count = accessor.count;
	auto  &buffer_view  = model.bufferViews[accessor.bufferView];
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
				index_data = convert_underlying_data_stride(index_data, 2, 4);
				break;
			}
			case VK_FORMAT_R8_UINT:
			{
				index_data = convert_underlying_data_stride(index_data, 1, 4);
				break;
			}
			default:
			{
				break;
			}
		}

		// Always do uint32
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
		image          = sg::Image::load(gltf_image.name, image_uri, vkb::sg::Image::Unknown);
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
	vk_sampler.set_debug_name(gltf_sampler.name);

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

std::vector<std::unique_ptr<sg::Light>> GLTFLoader::parse_khr_lights_punctual()
{
	if (is_extension_enabled(KHR_LIGHTS_PUNCTUAL_EXTENSION))
	{
		if (model.extensions.find(KHR_LIGHTS_PUNCTUAL_EXTENSION) == model.extensions.end() || !model.extensions.at(KHR_LIGHTS_PUNCTUAL_EXTENSION).Has("lights"))
		{
			return {};
		}
		auto &khr_lights = model.extensions.at(KHR_LIGHTS_PUNCTUAL_EXTENSION).Get("lights");

		std::vector<std::unique_ptr<sg::Light>> light_components(khr_lights.ArrayLen());

		for (size_t light_index = 0; light_index < khr_lights.ArrayLen(); ++light_index)
		{
			auto &khr_light = khr_lights.Get(static_cast<int>(light_index));

			// Spec states a light has to have a type to be valid
			if (!khr_light.Has("type"))
			{
				LOGE("KHR_lights_punctual extension: light {} doesn't have a type!", light_index);
				throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
			}

			auto light = std::make_unique<sg::Light>(khr_light.Get("name").Get<std::string>());

			sg::LightType       type;
			sg::LightProperties properties;

			// Get type
			auto &gltf_light_type = khr_light.Get("type").Get<std::string>();
			if (gltf_light_type == "point")
			{
				type = sg::LightType::Point;
			}
			else if (gltf_light_type == "spot")
			{
				type = sg::LightType::Spot;
			}
			else if (gltf_light_type == "directional")
			{
				type = sg::LightType::Directional;
			}
			else
			{
				LOGE("KHR_lights_punctual extension: light type '{}' is invalid", gltf_light_type);
				throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
			}

			// Get properties
			if (khr_light.Has("color"))
			{
				properties.color = glm::vec3(
				    static_cast<float>(khr_light.Get("color").Get(0).Get<double>()),
				    static_cast<float>(khr_light.Get("color").Get(1).Get<double>()),
				    static_cast<float>(khr_light.Get("color").Get(2).Get<double>()));
			}

			if (khr_light.Has("intensity"))
			{
				properties.intensity = static_cast<float>(khr_light.Get("intensity").Get<double>());
			}

			if (type != sg::LightType::Directional)
			{
				properties.range = static_cast<float>(khr_light.Get("range").Get<double>());
				if (type != sg::LightType::Point)
				{
					if (!khr_light.Has("spot"))
					{
						LOGE("KHR_lights_punctual extension: spot light doesn't have a 'spot' property set", gltf_light_type);
						throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
					}

					properties.inner_cone_angle = static_cast<float>(khr_light.Get("spot").Get("innerConeAngle").Get<double>());

					if (khr_light.Get("spot").Has("outerConeAngle"))
					{
						properties.outer_cone_angle = static_cast<float>(khr_light.Get("spot").Get("outerConeAngle").Get<double>());
					}
					else
					{
						// Spec states default value is PI/4
						properties.outer_cone_angle = glm::pi<float>() / 4.0f;
					}
				}
			}
			else if (type == sg::LightType::Directional || type == sg::LightType::Spot)
			{
				// The spec states that the light will inherit the transform of the node.
				// The light's direction is defined as the 3-vector (0.0, 0.0, -1.0) and
				// the rotation of the node orients the light accordingly.
				properties.direction = glm::vec3(0.0f, 0.0f, -1.0f);
			}

			light->set_light_type(type);
			light->set_properties(properties);

			light_components[light_index] = std::move(light);
		}

		return light_components;
	}
	else
	{
		return {};
	}
}

bool GLTFLoader::is_extension_enabled(const std::string &requested_extension)
{
	auto it = supported_extensions.find(requested_extension);
	if (it != supported_extensions.end())
	{
		return it->second;
	}
	else
	{
		return false;
	}
}

tinygltf::Value *GLTFLoader::get_extension(tinygltf::ExtensionMap &tinygltf_extensions, const std::string &extension)
{
	auto it = tinygltf_extensions.find(extension);
	if (it != tinygltf_extensions.end())
	{
		return &it->second;
	}
	else
	{
		return nullptr;
	}
}
}        // namespace vkb
