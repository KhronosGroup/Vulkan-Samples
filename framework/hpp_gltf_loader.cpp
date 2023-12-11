/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_gltf_loader.h"
#include "common/helpers.h"
#include "common/hpp_utils.h"
#include "core/hpp_command_buffer.h"
#include "hpp_api_vulkan_sample.h"
#include "scene_graph/components/hpp_camera.h"
#include "scene_graph/components/hpp_image.h"
#include "scene_graph/components/hpp_light.h"
#include "scene_graph/components/hpp_mesh.h"
#include "scene_graph/components/hpp_pbr_material.h"
#include "scene_graph/components/hpp_perspective_camera.h"
#include "scene_graph/components/hpp_sampler.h"
#include "scene_graph/components/hpp_texture.h"
#include "scene_graph/components/image/hpp_astc.h"
#include "scene_graph/hpp_node.h"
#include "scene_graph/hpp_scene.h"
#include "scene_graph/scripts/hpp_animation.h"

#include <vulkan/vulkan.hpp>

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
#include <glm/gtc/type_ptr.hpp>
VKBP_ENABLE_WARNINGS()

/**
 * @brief Helper Function to change array type T to array type Y
 * Create a struct that can be used with std::transform so that we do not need to recreate lambda functions
 * @param T
 * @param Y
 */
template <class T, class Y>
struct TypeCast
{
	Y operator()(T value) const noexcept
	{
		return static_cast<Y>(value);
	}
};

namespace detail
{
vk::Format map_format(int type, int component_type, bool normalized);

std::vector<uint8_t> copy_strided_data(uint8_t const *src_data, size_t src_count, uint32_t src_stride, uint32_t dst_stride)
{
	std::vector<uint8_t> result(src_count * dst_stride);

	size_t src_size = src_count * src_stride;
	for (size_t idxSrc = 0, idxDst = 0; idxSrc < src_size; idxSrc += src_stride, idxDst += dst_stride)
	{
		assert(idxDst < result.size());
		std::copy(src_data + idxSrc, src_data + idxSrc + src_stride, result.begin() + idxDst);
	}

	return result;
}

std::pair<uint8_t const *, size_t> get_attribute_data(const tinygltf::Model *model, uint32_t accessorId)
{
	assert(accessorId < model->accessors.size());
	auto &accessor = model->accessors[accessorId];
	assert(accessor.bufferView < model->bufferViews.size());
	auto &bufferView = model->bufferViews[accessor.bufferView];
	assert(bufferView.buffer < model->buffers.size());
	auto &buffer = model->buffers[bufferView.buffer];

	size_t stride     = accessor.ByteStride(bufferView);
	size_t start_byte = accessor.byteOffset + bufferView.byteOffset;

	return std::make_pair(buffer.data.data() + start_byte, accessor.count * stride);
}

std::pair<float const *, size_t> get_attribute_data(tinygltf::Model const &model, tinygltf::Primitive const &primitive, std::string const &attribute_name,
                                                    int type)
{
	float const *data       = nullptr;
	size_t       data_count = 0;

	auto attribute_it = primitive.attributes.find(attribute_name);
	if (attribute_it != primitive.attributes.end())
	{
		assert(attribute_it->second < model.accessors.size());
		auto &accessor = model.accessors[attribute_it->second];
		assert(accessor.type == type);
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
		auto &buffer_view = model.bufferViews[accessor.bufferView];
		data              = reinterpret_cast<const float *>(&(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]));
		data_count        = accessor.count;
	}
	return std::make_pair(data, data_count);
}

vk::Format get_attribute_format(const tinygltf::Model *model, uint32_t accessorId)
{
	assert(accessorId < model->accessors.size());
	auto &accessor = model->accessors[accessorId];
	return map_format(accessor.type, accessor.componentType, accessor.normalized);
};

size_t get_attribute_size(const tinygltf::Model *model, uint32_t accessorId)
{
	assert(accessorId < model->accessors.size());
	return model->accessors[accessorId].count;
};

size_t get_attribute_stride(const tinygltf::Model *model, uint32_t accessorId)
{
	assert(accessorId < model->accessors.size());
	auto &accessor = model->accessors[accessorId];
	assert(accessor.bufferView < model->bufferViews.size());
	auto &bufferView = model->bufferViews[accessor.bufferView];

	return accessor.ByteStride(bufferView);
};

std::tuple<unsigned char const *, size_t, size_t, vk::Format> get_index_data(tinygltf::Model const &model, int indices_index)
{
	assert(indices_index < model.accessors.size());
	auto &accessor = model.accessors[indices_index];
	assert(accessor.bufferView < model.bufferViews.size());
	auto &bufferView = model.bufferViews[accessor.bufferView];
	assert(bufferView.buffer < model.buffers.size());
	auto &buffer = model.buffers[bufferView.buffer];

	size_t start_byte = accessor.byteOffset + bufferView.byteOffset;

	return std::make_tuple(buffer.data.data() + start_byte, accessor.count, accessor.ByteStride(bufferView),
	                       map_format(accessor.type, accessor.componentType, accessor.normalized));
}

std::tuple<bool, tinygltf::Model, std::string> load_model(std::string const &file_name)
{
	tinygltf::TinyGLTF gltf_loader;
	tinygltf::Model    model;
	std::string        err;
	std::string        warn;
	std::string        gltf_file    = vkb::fs::path::get(vkb::fs::path::Type::Assets) + file_name;
	bool               importResult = gltf_loader.LoadASCIIFromFile(&model, &err, &warn, gltf_file.c_str());

	if (!importResult)
	{
		LOGE("Failed to load gltf file {}.", gltf_file.c_str());
		return std::make_tuple(false, model, "");
	}

	if (!err.empty())
	{
		LOGE("Error loading gltf model: {}.", err.c_str());
		return std::make_tuple(false, model, "");
	}

	if (!warn.empty())
	{
		LOGI("{}", warn.c_str());
	}

	size_t      pos        = file_name.find_last_of('/');
	std::string model_path = (pos == std::string::npos) ? "" : file_name.substr(0, pos);

	return std::make_tuple(true, model, model_path);
}

vk::Format map_format(int type, int component_type, bool normalized)
{
	assert(!normalized || (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) || (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT));
	switch (type)
	{
		case TINYGLTF_TYPE_SCALAR:
			switch (component_type)
			{
				case TINYGLTF_COMPONENT_TYPE_BYTE:
					return vk::Format::eR8Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					return normalized ? vk::Format::eR8Unorm : vk::Format::eR8Uint;
				case TINYGLTF_COMPONENT_TYPE_SHORT:
					return vk::Format::eR16Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					return normalized ? vk::Format::eR16Unorm : vk::Format::eR16Uint;
				case TINYGLTF_COMPONENT_TYPE_INT:
					return vk::Format::eR32Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					return vk::Format::eR32Uint;
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
					return vk::Format::eR32Sfloat;
				case TINYGLTF_COMPONENT_TYPE_DOUBLE:
					return vk::Format::eR64Sfloat;
				default:
					assert(false);
					return vk::Format::eUndefined;
			}
			break;

		case TINYGLTF_TYPE_VEC2:
			switch (component_type)
			{
				case TINYGLTF_COMPONENT_TYPE_BYTE:
					return vk::Format::eR8G8Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					return normalized ? vk::Format::eR8G8Unorm : vk::Format::eR8G8Uint;
				case TINYGLTF_COMPONENT_TYPE_SHORT:
					return vk::Format::eR16G16Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					return normalized ? vk::Format::eR16G16Unorm : vk::Format::eR16G16Uint;
				case TINYGLTF_COMPONENT_TYPE_INT:
					return vk::Format::eR32G32Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					return vk::Format::eR32G32Uint;
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
					return vk::Format::eR32G32Sfloat;
				case TINYGLTF_COMPONENT_TYPE_DOUBLE:
					return vk::Format::eR64G64Sfloat;
				default:
					assert(false);
					return vk::Format::eUndefined;
			}
			break;

		case TINYGLTF_TYPE_VEC3:
			switch (component_type)
			{
				case TINYGLTF_COMPONENT_TYPE_BYTE:
					return vk::Format::eR8G8B8Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					return normalized ? vk::Format::eR8G8B8Unorm : vk::Format::eR8G8B8Uint;
				case TINYGLTF_COMPONENT_TYPE_SHORT:
					return vk::Format::eR16G16B16Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					return normalized ? vk::Format::eR16G16B16Unorm : vk::Format::eR16G16B16Uint;
				case TINYGLTF_COMPONENT_TYPE_INT:
					return vk::Format::eR32G32B32Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					return vk::Format::eR32G32B32Uint;
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
					return vk::Format::eR32G32B32Sfloat;
				case TINYGLTF_COMPONENT_TYPE_DOUBLE:
					return vk::Format::eR64G64B64Sfloat;
				default:
					assert(false);
					return vk::Format::eUndefined;
			}
			break;

		case TINYGLTF_TYPE_VEC4:
			switch (component_type)
			{
				case TINYGLTF_COMPONENT_TYPE_BYTE:
					return vk::Format::eR8G8B8A8Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					return normalized ? vk::Format::eR8G8B8A8Unorm : vk::Format::eR8G8B8A8Uint;
				case TINYGLTF_COMPONENT_TYPE_SHORT:
					return vk::Format::eR16G16B16A16Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					return normalized ? vk::Format::eR16G16B16A16Unorm : vk::Format::eR16G16B16A16Uint;
				case TINYGLTF_COMPONENT_TYPE_INT:
					return vk::Format::eR32G32B32A32Sint;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					return vk::Format::eR32G32B32A32Uint;
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
					return vk::Format::eR32G32B32A32Sfloat;
				case TINYGLTF_COMPONENT_TYPE_DOUBLE:
					return vk::Format::eR64G64B64A64Sfloat;
				default:
					assert(false);
					return vk::Format::eUndefined;
			}
			break;

		default:
			assert(false);
			return vk::Format::eUndefined;
	}
}

vk::Filter map_mag_filter(int gltf_filter)
{
	switch (gltf_filter)
	{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
			return vk::Filter::eNearest;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		default:
			return vk::Filter::eLinear;
	}
}

vk::Filter map_min_filter(int gltf_filter)
{
	switch (gltf_filter)
	{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			return vk::Filter::eNearest;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
		default:
			return vk::Filter::eLinear;
	}
}

vk::SamplerMipmapMode map_mipmap_mode(int gltf_filter)
{
	switch (gltf_filter)
	{
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			return vk::SamplerMipmapMode::eNearest;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
		default:
			return vk::SamplerMipmapMode::eLinear;
	}
}

vk::SamplerAddressMode map_wrap_mode(int gltf_wrap)
{
	switch (gltf_wrap)
	{
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			return vk::SamplerAddressMode::eClampToEdge;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			return vk::SamplerAddressMode::eMirroredRepeat;
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
		default:
			return vk::SamplerAddressMode::eRepeat;
	}
}

std::vector<HPPMeshlet> prepare_meshlets(std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> const &submesh, std::vector<unsigned char> const &index_data)
{
	std::vector<HPPMeshlet> meshlets;
	HPPMeshlet              meshlet;
	meshlet.vertex_count = 0;
	meshlet.index_count  = 0;

	std::set<uint32_t> vertices;        // set for unique vertices

	// index_data is unsigned char type, casting to uint32_t* will give proper value
	uint32_t const *indices = reinterpret_cast<uint32_t const *>(index_data.data());

	for (uint32_t i = 0; i < submesh->vertex_indices; i++)
	{
		meshlet.indices[meshlet.index_count] = *(indices + i);

		if (vertices.insert(meshlet.indices[meshlet.index_count]).second)
		{
			++meshlet.vertex_count;
		}

		++meshlet.index_count;

		if (meshlet.vertex_count == HPPMeshlet::MAX_VERTICES || meshlet.index_count == HPPMeshlet::MAX_INDICES || i == submesh->vertex_indices - 1)
		{
			uint32_t counter = 0;
			for (auto v : vertices)
			{
				meshlet.vertices[counter++] = v;
			}
			uint32_t triangle_check = meshlet.index_count % 3;
			if (triangle_check)
			{
				// each meshlet needs to contain full primitives
				meshlet.index_count -= triangle_check;
				i -= triangle_check;
				triangle_check = 0;
			}

			meshlets.push_back(meshlet);
			meshlet.vertex_count = 0;
			meshlet.index_count  = 0;
			vertices.clear();
		}
	}
	return meshlets;
}

void upload_image_to_gpu(vkb::core::HPPCommandBuffer &command_buffer, vkb::core::HPPBuffer const &staging_buffer, vkb::scene_graph::components::HPPImage &image)
{
	// Clean up the image data, as they are copied in the staging buffer
	image.clear_data();
	vkb::common::image_layout_transition(command_buffer.get_handle(), image.get_vk_image().get_handle(), vk::ImageLayout::eUndefined,
	                                     vk::ImageLayout::eTransferDstOptimal, image.get_vk_image_view().get_subresource_range());

	// Create a buffer image copy for every mip level
	auto                            &mipmaps = image.get_mipmaps();
	std::vector<vk::BufferImageCopy> buffer_copy_regions;
	buffer_copy_regions.reserve(mipmaps.size());
	for (auto const &mipmap : mipmaps)
	{
		buffer_copy_regions.emplace_back(vk::BufferImageCopy(mipmap.offset, {}, {}, image.get_vk_image_view().get_subresource_layers(), {}, mipmap.extent));
		buffer_copy_regions.back().imageSubresource.mipLevel = mipmap.level;        // Update miplevel
	}
	command_buffer.copy_buffer_to_image(staging_buffer, image.get_vk_image(), buffer_copy_regions);

	vkb::common::image_layout_transition(command_buffer.get_handle(), image.get_vk_image().get_handle(), vk::ImageLayout::eTransferDstOptimal,
	                                     vk::ImageLayout::eShaderReadOnlyOptimal, image.get_vk_image_view().get_subresource_range());
}

bool texture_needs_srgb_colorspace(const std::string &name)
{
	// The gltf spec states that the base and emissive textures MUST be encoded with the sRGB
	// transfer function. All other texture types are linear.
	assert((name == "baseColorTexture") || (name == "emissiveTexture") || (name == "metallicRoughnessTexture") || (name == "normalTexture") ||
	       (name == "occlusionTexture"));
	return (name == "baseColorTexture") || (name == "emissiveTexture");
}
}        // namespace detail

namespace vkb
{
/// The extensions that the HPPGLTFLoader can load mapped to whether they should be enabled or not
std::unordered_map<std::string, bool> HPPGLTFLoader::supported_extensions = {{KHR_LIGHTS_PUNCTUAL_EXTENSION, false}};

HPPGLTFLoader::HPPGLTFLoader(vkb::core::HPPDevice &device) :
    device{device}
{}

std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> HPPGLTFLoader::read_model_from_file(const std::string &file_name, uint32_t index, bool storage_buffer)
{
	bool success;
	std::tie(success, model, model_path) = detail::load_model(file_name);
	assert(success);

	return std::move(load_model(index, storage_buffer));
}

std::unique_ptr<vkb::scene_graph::HPPScene> HPPGLTFLoader::read_scene_from_file(const std::string &file_name, int scene_index)
{
	bool success;
	std::tie(success, model, model_path) = detail::load_model(file_name);
	assert(success);

	return std::make_unique<vkb::scene_graph::HPPScene>(load_scene(scene_index));
}

void HPPGLTFLoader::add_default_camera(vkb::scene_graph::HPPScene &scene) const
{
	auto default_camera      = create_default_camera();
	auto default_camera_node = std::make_unique<vkb::scene_graph::HPPNode>(-1, "default_camera");
	default_camera->set_node(*default_camera_node);
	default_camera_node->set_component(*default_camera);
	scene.add_component(std::move(default_camera));
	scene.get_root_node().add_child(*default_camera_node);
	scene.add_node(std::move(default_camera_node));
}

void HPPGLTFLoader::check_extensions() const
{
	for (auto const &used_extension : model.extensionsUsed)
	{
		auto it = supported_extensions.find(used_extension);

		// Check if extension isn't supported by the HPPGLTFLoader
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
}

std::unique_ptr<vkb::scene_graph::components::HPPCamera> HPPGLTFLoader::create_default_camera() const
{
	tinygltf::Camera gltf_camera;

	gltf_camera.name                    = "default_camera";
	gltf_camera.type                    = "perspective";
	gltf_camera.perspective.aspectRatio = 1.77f;
	gltf_camera.perspective.yfov        = 1.0f;
	gltf_camera.perspective.znear       = 0.1f;
	gltf_camera.perspective.zfar        = 1000.0f;

	return parse_camera(gltf_camera);
}

std::unique_ptr<vkb::scene_graph::components::HPPPBRMaterial> HPPGLTFLoader::create_default_material() const
{
	tinygltf::Material gltf_material;
	gltf_material.name = "default_material";
	return parse_material(gltf_material, {});
}

std::unique_ptr<vkb::scene_graph::components::HPPSampler> HPPGLTFLoader::create_default_sampler() const
{
	tinygltf::Sampler gltf_sampler;

	gltf_sampler.name      = "default_sampler";
	gltf_sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
	gltf_sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
	gltf_sampler.wrapS     = TINYGLTF_TEXTURE_WRAP_REPEAT;
	gltf_sampler.wrapT     = TINYGLTF_TEXTURE_WRAP_REPEAT;
	gltf_sampler.wrapR     = TINYGLTF_TEXTURE_WRAP_REPEAT;

	return parse_sampler(gltf_sampler);
}

tinygltf::Value const *HPPGLTFLoader::get_extension(tinygltf::ExtensionMap const &tinygltf_extensions, const std::string &extension) const
{
	auto it = tinygltf_extensions.find(extension);
	return (it != tinygltf_extensions.end()) ? &it->second : nullptr;
}

bool HPPGLTFLoader::is_extension_enabled(const std::string &requested_extension) const
{
	auto it = supported_extensions.find(requested_extension);
	return (it != supported_extensions.end()) ? it->second : false;
}

std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> HPPGLTFLoader::load_model(uint32_t mesh_index, bool storage_buffer)
{
	auto submesh = std::make_unique<vkb::scene_graph::components::HPPSubMesh>();

	assert(mesh_index < model.meshes.size());
	auto &gltf_mesh = model.meshes[mesh_index];

	assert(!gltf_mesh.primitives.empty());
	auto &gltf_primitive = gltf_mesh.primitives[0];
	if (1 < gltf_mesh.primitives.size())
	{
		LOGW("HPPGLTFLoader: ignoring #{} primitives", gltf_mesh.primitives.size() - 1);
	}

	// Position attribute is required
	auto [positions, vertex_count] = detail::get_attribute_data(model, gltf_primitive, "POSITION", TINYGLTF_TYPE_VEC3);
	assert(positions && (0 < vertex_count));

	auto [normals, normals_count] = detail::get_attribute_data(model, gltf_primitive, "NORMAL", TINYGLTF_TYPE_VEC3);
	assert(!normals || (vertex_count == normals_count));

	auto [tex_coords, tex_coords_count] = detail::get_attribute_data(model, gltf_primitive, "TEXCOORD_0", TINYGLTF_TYPE_VEC2);
	assert(!tex_coords || (vertex_count == tex_coords_count));

	// Skinning: joints
	auto [joints, joints_count] = detail::get_attribute_data(model, gltf_primitive, "JOINTS_0", TINYGLTF_TYPE_VEC4);
	assert(!joints || (vertex_count == joints_count));

	// Skinning: weights
	auto [weights, weights_count] = detail::get_attribute_data(model, gltf_primitive, "WEIGHTS_0", TINYGLTF_TYPE_VEC4);
	assert(!weights || (vertex_count == weights_count));

	bool has_skin = (joints && weights);

	std::vector<vkb::core::HPPBuffer> transient_buffers;

	auto &command_buffer = device.get_command_pool().request_command_buffer();
	command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	if (storage_buffer)
	{
		std::vector<HPPAlignedVertex> aligned_vertex_data;
		for (size_t v = 0; v < vertex_count; v++)
		{
			HPPAlignedVertex vert{};
			vert.pos    = glm::vec4(glm::make_vec3(&positions[v * 3]), 1.0f);
			vert.normal = normals ? glm::vec4(glm::normalize(glm::make_vec3(&normals[v * 3])), 0.0f) : glm::vec4(0.0f);
			aligned_vertex_data.push_back(vert);
		}

		vkb::core::HPPBuffer stage_buffer{device, aligned_vertex_data.size() * sizeof(HPPAlignedVertex), vk::BufferUsageFlagBits::eTransferSrc,
		                                  VMA_MEMORY_USAGE_CPU_ONLY};

		stage_buffer.update(aligned_vertex_data.data(), aligned_vertex_data.size() * sizeof(HPPAlignedVertex));

		vkb::core::HPPBuffer buffer{device, aligned_vertex_data.size() * sizeof(HPPAlignedVertex),
		                            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY};

		command_buffer.copy_buffer(stage_buffer, buffer, aligned_vertex_data.size() * sizeof(HPPAlignedVertex));

		submesh->set_vertex_buffer("vertex_buffer", std::move(buffer));

		transient_buffers.push_back(std::move(stage_buffer));
	}
	else
	{
		std::vector<HPPVertex> vertex_data;
		vertex_data.reserve(vertex_count);
		for (size_t v = 0; v < vertex_count; v++)
		{
			vertex_data.emplace_back(HPPVertex(glm::make_vec3(&positions[v * 3]), normals ? glm::normalize(glm::make_vec3(&normals[v * 3])) : glm::vec3(0.0f),
			                                   tex_coords ? glm::make_vec2(&tex_coords[v * 2]) : glm::vec2(0.0f),
			                                   has_skin ? glm::make_vec4(&joints[v * 4]) : glm::vec4(0.0f),
			                                   has_skin ? glm::make_vec4(&weights[v * 4]) : glm::vec4(0.0f)));
		}

		vkb::core::HPPBuffer stage_buffer{device, vertex_data.size() * sizeof(HPPVertex), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY};

		stage_buffer.update(vertex_data.data(), vertex_data.size() * sizeof(HPPVertex));

		vkb::core::HPPBuffer buffer{device, vertex_data.size() * sizeof(HPPVertex),
		                            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_GPU_ONLY};

		command_buffer.copy_buffer(stage_buffer, buffer, vertex_data.size() * sizeof(HPPVertex));

		submesh->set_vertex_buffer("vertex_buffer", std::move(buffer));

		transient_buffers.push_back(std::move(stage_buffer));
	}

	if (gltf_primitive.indices >= 0)
	{
		auto [indices, index_count, stride, format] = detail::get_index_data(model, gltf_primitive.indices);
		submesh->vertex_indices                     = to_u32(index_count);

		std::vector<unsigned char> index_data;
		switch (format)
		{
			case vk::Format::eR8Uint:
				assert(stride == 1);
				index_data = detail::copy_strided_data(indices, index_count, 1, 4);
				break;
			case vk::Format::eR16Uint:
				assert(stride == 2);
				index_data = detail::copy_strided_data(indices, index_count, 2, 4);
				break;
			case vk::Format::eR32Uint:
				assert(stride == 4);
				index_data.assign(indices, indices + index_count * stride);
				// Correct format
				break;
			default:
				assert(false);
		}

		// Always do uint32
		submesh->set_index_type(vk::IndexType::eUint32);

		if (storage_buffer)
		{
			// prepare meshlets
			std::vector<HPPMeshlet> meshlets = detail::prepare_meshlets(submesh, index_data);

			// vertex_indices and index_buffer are used for meshlets now
			submesh->vertex_indices = (uint32_t) meshlets.size();

			vkb::core::HPPBuffer stage_buffer{device, meshlets.size() * sizeof(HPPMeshlet), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY};

			stage_buffer.update(meshlets.data(), meshlets.size() * sizeof(HPPMeshlet));

			submesh->set_index_buffer(std::make_unique<vkb::core::HPPBuffer>(device, meshlets.size() * sizeof(HPPMeshlet),
			                                                                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
			                                                                 VMA_MEMORY_USAGE_GPU_ONLY));

			command_buffer.copy_buffer(stage_buffer, submesh->get_index_buffer(), meshlets.size() * sizeof(HPPMeshlet));

			transient_buffers.push_back(std::move(stage_buffer));
		}
		else
		{
			vkb::core::HPPBuffer stage_buffer{device, index_data.size(), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY};

			stage_buffer.update(index_data);

			submesh->set_index_buffer(std::make_unique<vkb::core::HPPBuffer>(
			    device, index_data.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_GPU_ONLY));

			command_buffer.copy_buffer(stage_buffer, submesh->get_index_buffer(), index_data.size());

			transient_buffers.push_back(std::move(stage_buffer));
		}
	}

	command_buffer.end();

	auto &queue = device.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
	queue.submit(command_buffer, device.get_fence_pool().request_fence());

	device.get_fence_pool().wait();
	device.get_fence_pool().reset();
	device.get_command_pool().reset_pool();

	return std::move(submesh);
}

vkb::scene_graph::HPPScene HPPGLTFLoader::load_scene(int scene_index)
{
	auto scene = vkb::scene_graph::HPPScene();
	scene.set_name("gltf_scene");

	check_extensions();
	scene.set_components(parse_khr_lights_punctual());
	scene.set_components(parse_samplers());

	Timer timer;
	timer.start();

	auto thread_count = std::thread::hardware_concurrency();
	thread_count      = thread_count == 0 ? 1 : thread_count;

	scene.set_components(parse_images(thread_count));

	auto elapsed_time = timer.stop();
	LOGI("Time spent loading images: {} seconds across {} threads.", vkb::to_string(elapsed_time), thread_count);

	scene.set_components(parse_textures(scene));         // needs samplers and images already parsed
	scene.set_components(parse_materials(scene));        // needs textures already parsed
	scene.set_components(parse_meshes(scene));           // needs materials already parsed
	scene.set_components(parse_cameras());
	scene.set_nodes(parse_nodes(scene));                    // needs cameras, lights, and messhes already parsed
	scene.set_components(parse_animations(scene));          // needs nodes already parsed
	scene.add_node(parse_scene(scene, scene_index));        // needs nodes already parsed
	scene.set_root_node(*scene.get_nodes().back());

	add_default_camera(scene);

	if (!scene.has_component<vkb::scene_graph::components::HPPLight>())
	{
		// Add a default light if none are present
		vkb::common::add_directional_light(scene, glm::quat({glm::radians(-90.0f), 0.0f, glm::radians(30.0f)}));
	}

	return scene;
}

std::unique_ptr<vkb::scene_graph::scripts::HPPAnimation>
    HPPGLTFLoader::parse_animation(tinygltf::Animation const &gltf_animation, std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>> const &nodes) const
{
	auto animation = std::make_unique<vkb::scene_graph::scripts::HPPAnimation>(gltf_animation.name);

	std::vector<vkb::sg::AnimationSampler> samplers = parse_animation_samplers(gltf_animation.samplers);
	parse_animation_channels(gltf_animation.channels, samplers, nodes, *animation);

	return animation;
}

void HPPGLTFLoader::parse_animation_channels(std::vector<tinygltf::AnimationChannel> const &channels, std::vector<vkb::sg::AnimationSampler> const &samplers,
                                             std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>> const &nodes,
                                             vkb::scene_graph::scripts::HPPAnimation                       &animation) const
{
	for (size_t channel_index = 0; channel_index < channels.size(); ++channel_index)
	{
		auto &gltf_channel = channels[channel_index];

		vkb::sg::AnimationTarget target;
		if (gltf_channel.target_path == "translation")
		{
			target = vkb::sg::AnimationTarget::Translation;
		}
		else if (gltf_channel.target_path == "rotation")
		{
			target = vkb::sg::AnimationTarget::Rotation;
		}
		else if (gltf_channel.target_path == "scale")
		{
			target = vkb::sg::AnimationTarget::Scale;
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

		assert(gltf_channel.sampler < samplers.size());
		auto const &inputs     = samplers[gltf_channel.sampler].inputs;
		auto        min_max_it = std::minmax_element(inputs.begin(), inputs.end());
		animation.update_times(min_max_it.first != inputs.end() ? *min_max_it.first : std::numeric_limits<float>::max(),
		                       min_max_it.second != inputs.end() ? *min_max_it.second : std::numeric_limits<float>::min());

		assert(gltf_channel.target_node < nodes.size());
		animation.add_channel(*nodes[gltf_channel.target_node], target, samplers[gltf_channel.sampler]);
	}
}

vkb::sg::AnimationSampler HPPGLTFLoader::parse_animation_sampler(tinygltf::AnimationSampler const &gltf_sampler, int sampler_index) const
{
	vkb::sg::AnimationSampler sampler;

	if (gltf_sampler.interpolation == "LINEAR")
	{
		sampler.type = vkb::sg::AnimationType::Linear;
	}
	else if (gltf_sampler.interpolation == "STEP")
	{
		sampler.type = vkb::sg::AnimationType::Step;
	}
	else if (gltf_sampler.interpolation == "CUBICSPLINE")
	{
		sampler.type = vkb::sg::AnimationType::CubicSpline;
	}
	else
	{
		LOGW("Gltf animation sampler #{} has unknown interpolation value, falling back to LINEAR", sampler_index);
		sampler.type = vkb::sg::AnimationType::Linear;
	}

	assert(gltf_sampler.input < model.accessors.size());
	auto input_accessor      = model.accessors[gltf_sampler.input];
	auto input_accessor_data = detail::get_attribute_data(&model, gltf_sampler.input);

	const float *data = reinterpret_cast<const float *>(input_accessor_data.first);
	sampler.inputs.reserve(input_accessor.count);
	for (size_t i = 0; i < input_accessor.count; ++i)
	{
		sampler.inputs.push_back(data[i]);
	}

	assert(gltf_sampler.output < model.accessors.size());
	auto output_accessor      = model.accessors[gltf_sampler.output];
	auto output_accessor_data = detail::get_attribute_data(&model, gltf_sampler.output);

	sampler.outputs.reserve(output_accessor.count);
	switch (output_accessor.type)
	{
		case TINYGLTF_TYPE_VEC3:
		{
			const glm::vec3 *data = reinterpret_cast<const glm::vec3 *>(output_accessor_data.first);
			for (size_t i = 0; i < output_accessor.count; ++i)
			{
				sampler.outputs.push_back(glm::vec4(data[i], 0.0f));
			}
			break;
		}
		case TINYGLTF_TYPE_VEC4:
		{
			const glm::vec4 *data = reinterpret_cast<const glm::vec4 *>(output_accessor_data.first);
			for (size_t i = 0; i < output_accessor.count; ++i)
			{
				sampler.outputs.push_back(data[i]);
			}
			break;
		}
		default:
			LOGE("Gltf animation sampler #{} has unknown output data type", sampler_index);
			break;
	}

	return sampler;
}

std::vector<vkb::sg::AnimationSampler> HPPGLTFLoader::parse_animation_samplers(std::vector<tinygltf::AnimationSampler> const &gltf_samplers) const
{
	std::vector<vkb::sg::AnimationSampler> animation_samplers;
	animation_samplers.reserve(gltf_samplers.size());

	for (size_t sampler_index = 0; sampler_index < gltf_samplers.size(); ++sampler_index)
	{
		animation_samplers.push_back(parse_animation_sampler(gltf_samplers[sampler_index], static_cast<int>(sampler_index)));
	}

	return animation_samplers;
}

std::vector<std::unique_ptr<vkb::scene_graph::scripts::HPPAnimation>> HPPGLTFLoader::parse_animations(vkb::scene_graph::HPPScene const &scene) const
{
	std::vector<std::unique_ptr<vkb::scene_graph::scripts::HPPAnimation>> animations;
	animations.reserve(model.animations.size());

	auto const &nodes = scene.get_nodes();

	for (auto const &gltf_animation : model.animations)
	{
		animations.push_back(parse_animation(gltf_animation, nodes));
	}

	return animations;
}

std::unique_ptr<vkb::scene_graph::components::HPPCamera> HPPGLTFLoader::parse_camera(const tinygltf::Camera &gltf_camera) const
{
	std::unique_ptr<vkb::scene_graph::components::HPPCamera> camera;

	if (gltf_camera.type == "perspective")
	{
		auto perspective_camera = std::make_unique<vkb::scene_graph::components::HPPPerspectiveCamera>(gltf_camera.name);

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

std::vector<std::unique_ptr<vkb::scene_graph::components::HPPCamera>> HPPGLTFLoader::parse_cameras() const
{
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPCamera>> cameras;
	cameras.reserve(model.cameras.size() + 1);

	for (auto &gltf_camera : model.cameras)
	{
		cameras.push_back(parse_camera(gltf_camera));
	}

	return cameras;
}

std::unique_ptr<vkb::scene_graph::components::HPPImage> HPPGLTFLoader::parse_image(tinygltf::Image &gltf_image) const
{
	std::unique_ptr<vkb::scene_graph::components::HPPImage> image{nullptr};

	if (!gltf_image.image.empty())
	{
		// Image embedded in gltf file
		auto                                                 mipmap = vkb::scene_graph::components::HPPMipmap{/* .level = */ 0,
                                                              /* .offset = */ 0,
                                                              /* .extent = */
                                                              {/* .width = */ static_cast<uint32_t>(gltf_image.width),
                                                               /* .height = */ static_cast<uint32_t>(gltf_image.height),
                                                               /* .depth = */ 1u}};
        std::vector<vkb::scene_graph::components::HPPMipmap> mipmaps{mipmap};
		image = std::make_unique<vkb::scene_graph::components::HPPImage>(gltf_image.name, std::move(gltf_image.image), std::move(mipmaps));
	}
	else
	{
		// Load image from uri
		auto image_uri = model_path + "/" + gltf_image.uri;
		image          = vkb::scene_graph::components::HPPImage::load(gltf_image.name, image_uri, vkb::scene_graph::components::HPPImage::Unknown);
	}

	// Check whether the format is supported by the GPU
	if (vkb::scene_graph::components::is_astc(image->get_format()))
	{
		if (!device.get_gpu().is_image_format_supported(image->get_format()))
		{
			LOGW("ASTC not supported: decoding {}", image->get_name());
			image = std::make_unique<vkb::scene_graph::components::HPPAstc>(*image);
			image->generate_mipmaps();
		}
	}

	image->create_vk_image(device);

	return image;
}

std::vector<std::future<std::unique_ptr<vkb::scene_graph::components::HPPImage>>> HPPGLTFLoader::parse_image_futures(ctpl::thread_pool &thread_pool)
{
	std::vector<std::future<std::unique_ptr<vkb::scene_graph::components::HPPImage>>> image_futures;

	auto image_count = model.images.size();
	image_futures.reserve(image_count);
	for (size_t image_index = 0; image_index < image_count; image_index++)
	{
		auto fut = thread_pool.push([this, image_index](size_t) {
			auto image = parse_image(model.images[image_index]);

			LOGI("Loaded gltf image #{} ({})", image_index, model.images[image_index].uri.c_str());

			return image;
		});

		image_futures.push_back(std::move(fut));
	}

	return image_futures;
}

std::vector<std::unique_ptr<vkb::scene_graph::components::HPPImage>> HPPGLTFLoader::parse_images(unsigned int thread_count)
{
	ctpl::thread_pool                                                                 thread_pool(thread_count);
	std::vector<std::future<std::unique_ptr<vkb::scene_graph::components::HPPImage>>> image_futures = parse_image_futures(thread_pool);
	return upload_images(image_futures);
}

std::unique_ptr<vkb::scene_graph::components::HPPLight> HPPGLTFLoader::parse_khr_light(tinygltf::Value const &khr_light, int light_index) const
{
	// Spec states a light has to have a type to be valid
	if (!khr_light.Has("type"))
	{
		LOGE("KHR_lights_punctual extension: light {} doesn't have a type!", light_index);
		throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
	}

	vkb::sg::LightType type = parse_khr_light_type(khr_light.Get("type").Get<std::string>());

	auto light = std::make_unique<vkb::scene_graph::components::HPPLight>(khr_light.Get("name").Get<std::string>());
	light->set_light_type(type);
	light->set_properties(parse_khr_light_properties(khr_light, type));

	return light;
}

vkb::sg::LightProperties HPPGLTFLoader::parse_khr_light_properties(tinygltf::Value const &khr_light, vkb::sg::LightType type) const
{
	vkb::sg::LightProperties properties{};

	if (khr_light.Has("color"))
	{
		tinygltf::Value const &color = khr_light.Get("color");
		properties.color             = glm::vec3(static_cast<float>(color.Get(0).Get<double>()), static_cast<float>(color.Get(1).Get<double>()),
		                                         static_cast<float>(color.Get(2).Get<double>()));
	}

	if (khr_light.Has("intensity"))
	{
		properties.intensity = static_cast<float>(khr_light.Get("intensity").Get<double>());
	}

	if ((type == vkb::sg::LightType::Point) || (type == vkb::sg::LightType::Spot))
	{
		assert(khr_light.Has("range"));
		properties.range = static_cast<float>(khr_light.Get("range").Get<double>());
	}
	if (type == vkb::sg::LightType::Spot)
	{
		if (!khr_light.Has("spot"))
		{
			LOGE("KHR_lights_punctual extension: spot light doesn't have a 'spot' property set", khr_light.Get("type").Get<std::string>());
			throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
		}

		tinygltf::Value const &spot = khr_light.Get("spot");
		properties.inner_cone_angle = static_cast<float>(spot.Get("innerConeAngle").Get<double>());

		if (spot.Has("outerConeAngle"))
		{
			properties.outer_cone_angle = static_cast<float>(spot.Get("outerConeAngle").Get<double>());
		}
		else
		{
			// Spec states default value is PI/4
			properties.outer_cone_angle = glm::pi<float>() / 4.0f;
		}
	}
	if ((type == vkb::sg::LightType::Directional) || (type == vkb::sg::LightType::Spot))
	{
		// The spec states that the light will inherit the transform of the node.
		// The light's direction is defined as the 3-vector (0.0, 0.0, -1.0) and
		// the rotation of the node orients the light accordingly.
		properties.direction = glm::vec3(0.0f, 0.0f, -1.0f);
	}

	return properties;
}

vkb::sg::LightType HPPGLTFLoader::parse_khr_light_type(std::string const &type) const
{
	if (type == "point")
	{
		return vkb::sg::LightType::Point;
	}
	else if (type == "spot")
	{
		return vkb::sg::LightType::Spot;
	}
	else if (type == "directional")
	{
		return vkb::sg::LightType::Directional;
	}
	else
	{
		LOGE("KHR_lights_punctual extension: light type '{}' is invalid", type);
		throw std::runtime_error("Couldn't load glTF file, KHR_lights_punctual extension is invalid");
	}
}

std::vector<std::unique_ptr<vkb::scene_graph::components::HPPLight>> HPPGLTFLoader::parse_khr_lights_punctual() const
{
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPLight>> light_components;

	auto model_it = model.extensions.find(KHR_LIGHTS_PUNCTUAL_EXTENSION);
	if (is_extension_enabled(KHR_LIGHTS_PUNCTUAL_EXTENSION) && (model_it != model.extensions.end()) && model_it->second.Has("lights"))
	{
		auto &khr_lights = model_it->second.Get("lights");

		light_components.reserve(khr_lights.ArrayLen());
		for (size_t light_index = 0; light_index < khr_lights.ArrayLen(); ++light_index)
		{
			int idx = static_cast<int>(light_index);
			light_components.push_back(parse_khr_light(khr_lights.Get(idx), idx));
		}
	}
	return light_components;
}

std::unique_ptr<vkb::scene_graph::components::HPPPBRMaterial>
    HPPGLTFLoader::parse_material(const tinygltf::Material &gltf_material, std::vector<vkb::scene_graph::components::HPPTexture *> const &textures) const
{
	auto material = std::make_unique<vkb::scene_graph::components::HPPPBRMaterial>(gltf_material.name);

	parse_material_values(gltf_material.values, *material, textures);
	parse_material_values(gltf_material.additionalValues, *material, textures);

	return material;
}

void HPPGLTFLoader::parse_material_values(tinygltf::ParameterMap const &parameters, vkb::scene_graph::components::HPPPBRMaterial &material,
                                          std::vector<vkb::scene_graph::components::HPPTexture *> const &textures) const
{
	for (auto &parameter : parameters)
	{
		if (parameter.first == "alphaCutoff")
		{
			material.alpha_cutoff = static_cast<float>(parameter.second.number_value);
		}
		else if (parameter.first == "alphaMode")
		{
			if (parameter.second.string_value == "BLEND")
			{
				material.alpha_mode = vkb::sg::AlphaMode::Blend;
			}
			else if (parameter.second.string_value == "OPAQUE")
			{
				material.alpha_mode = vkb::sg::AlphaMode::Opaque;
			}
			else if (parameter.second.string_value == "MASK")
			{
				material.alpha_mode = vkb::sg::AlphaMode::Mask;
			}
			else
			{
				LOGW("[HPPGLTFLoader] material {} : parameter {} : unhandled value {}", material.get_name(), parameter.first, parameter.second.string_value);
			}
		}
		else if (parameter.first == "baseColorFactor")
		{
			const auto &color_factor   = parameter.second.ColorFactor();
			material.base_color_factor = glm::vec4(color_factor[0], color_factor[1], color_factor[2], color_factor[3]);
		}
		else if (parameter.first == "doubleSided")
		{
			material.double_sided = parameter.second.bool_value;
		}
		else if (parameter.first == "emissiveFactor")
		{
			const auto &emissive_factor = parameter.second.number_array;
			material.emissive           = glm::vec3(emissive_factor[0], emissive_factor[1], emissive_factor[2]);
		}
		else if (parameter.first == "metallicFactor")
		{
			material.metallic_factor = static_cast<float>(parameter.second.Factor());
		}
		else if (parameter.first == "name")
		{
			assert(material.get_name() == parameter.second.string_value);
		}
		else if (parameter.first == "roughnessFactor")
		{
			material.roughness_factor = static_cast<float>(parameter.second.Factor());
		}
		else if (parameter.first.find("Texture") != std::string::npos)
		{
			std::string tex_name = to_snake_case(parameter.first);

			assert(parameter.second.TextureIndex() < textures.size());
			vkb::scene_graph::components::HPPTexture *tex = textures[parameter.second.TextureIndex()];

			if (detail::texture_needs_srgb_colorspace(parameter.first))
			{
				tex->get_image()->coerce_format_to_srgb();
			}

			material.set_texture(tex_name, tex);
		}
		else
		{
			LOGW("[HPPGLTFLoader] material {} : unhandled parameter {}", material.get_name(), parameter.first);
		}
	}
}

std::vector<std::unique_ptr<vkb::scene_graph::components::HPPPBRMaterial>> HPPGLTFLoader::parse_materials(vkb::scene_graph::HPPScene const &scene) const
{
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPPBRMaterial>> materials;
	materials.reserve(model.materials.size() + 1);

	bool                                                    has_textures = scene.has_component<vkb::scene_graph::components::HPPTexture>();
	std::vector<vkb::scene_graph::components::HPPTexture *> textures;
	if (has_textures)
	{
		textures = scene.get_components<vkb::scene_graph::components::HPPTexture>();
	}

	for (auto &gltf_material : model.materials)
	{
		materials.push_back(parse_material(gltf_material, textures));
	}
	materials.push_back(create_default_material());

	return materials;
}

std::unique_ptr<vkb::scene_graph::components::HPPMesh>
    HPPGLTFLoader::parse_mesh(vkb::scene_graph::HPPScene &scene, const tinygltf::Mesh &gltf_mesh,
                              std::vector<vkb::scene_graph::components::HPPPBRMaterial *> const &materials) const
{
	auto mesh = std::make_unique<vkb::scene_graph::components::HPPMesh>(gltf_mesh.name);

	for (size_t primitive_index = 0; primitive_index < gltf_mesh.primitives.size(); primitive_index++)
	{
		auto submesh = parse_primitive(gltf_mesh.primitives[primitive_index], gltf_mesh.name, primitive_index, materials);
		mesh->add_submesh(*submesh);
		scene.add_component(std::move(submesh));
	}

	return mesh;
}

std::vector<std::unique_ptr<vkb::scene_graph::components::HPPMesh>> HPPGLTFLoader::parse_meshes(vkb::scene_graph::HPPScene &scene) const
{
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPMesh>> meshes;
	meshes.reserve(model.meshes.size());

	auto materials = scene.get_components<vkb::scene_graph::components::HPPPBRMaterial>();
	for (auto &gltf_mesh : model.meshes)
	{
		meshes.push_back(parse_mesh(scene, gltf_mesh, materials));
	}

	return meshes;
}

std::unique_ptr<vkb::scene_graph::HPPNode> HPPGLTFLoader::parse_node(const tinygltf::Node &gltf_node, size_t index,
                                                                     std::vector<vkb::scene_graph::components::HPPMesh *> const   &meshes,
                                                                     std::vector<vkb::scene_graph::components::HPPCamera *> const &cameras,
                                                                     std::vector<vkb::scene_graph::components::HPPLight *> const  &lights) const
{
	auto node = std::make_unique<vkb::scene_graph::HPPNode>(index, gltf_node.name);
	parse_node_transform(gltf_node, node->get_transform());
	parse_node_mesh(gltf_node, meshes, *node);
	parse_node_camera(gltf_node, cameras, *node);
	parse_node_extension(gltf_node, lights, *node);

	return node;
}

void HPPGLTFLoader::parse_node_camera(const tinygltf::Node &gltf_node, std::vector<vkb::scene_graph::components::HPPCamera *> const &cameras,
                                      vkb::scene_graph::HPPNode &node) const
{
	if (gltf_node.camera >= 0)
	{
		assert(gltf_node.camera < cameras.size());
		auto camera = cameras[gltf_node.camera];

		node.set_component(*camera);
		camera->set_node(node);
	}
}

void HPPGLTFLoader::parse_node_extension(const tinygltf::Node &gltf_node, std::vector<vkb::scene_graph::components::HPPLight *> const &lights,
                                         vkb::scene_graph::HPPNode &node) const
{
	if (auto extension = get_extension(gltf_node.extensions, KHR_LIGHTS_PUNCTUAL_EXTENSION))
	{
		int light_index = extension->Get("light").Get<int>();
		assert(light_index < lights.size());
		auto light = lights[light_index];

		node.set_component(*light);
		light->set_node(node);
	}
}

void HPPGLTFLoader::parse_node_mesh(const tinygltf::Node &gltf_node, std::vector<vkb::scene_graph::components::HPPMesh *> const &meshes,
                                    vkb::scene_graph::HPPNode &node) const
{
	if (gltf_node.mesh >= 0)
	{
		assert(gltf_node.mesh < meshes.size());
		auto mesh = meshes[gltf_node.mesh];

		node.set_component(*mesh);
		mesh->add_node(node);
	}
}

void HPPGLTFLoader::parse_node_transform(tinygltf::Node const &gltf_node, vkb::scene_graph::components::HPPTransform &transform) const
{
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
}

std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>> HPPGLTFLoader::parse_nodes(vkb::scene_graph::HPPScene const &scene) const
{
	std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>> nodes;
	nodes.reserve(model.nodes.size());

	auto meshes  = scene.get_components<vkb::scene_graph::components::HPPMesh>();
	auto cameras = scene.get_components<vkb::scene_graph::components::HPPCamera>();
	auto lights  = scene.get_components<vkb::scene_graph::components::HPPLight>();
	for (size_t node_index = 0; node_index < model.nodes.size(); ++node_index)
	{
		nodes.emplace_back(parse_node(model.nodes[node_index], node_index, meshes, cameras, lights));
	}

	return nodes;
}

std::unique_ptr<vkb::scene_graph::components::HPPSubMesh>
    HPPGLTFLoader::parse_primitive(tinygltf::Primitive const &gltf_primitive, std::string const &mesh_name, size_t primitive_index,
                                   std::vector<vkb::scene_graph::components::HPPPBRMaterial *> const &materials) const
{
	auto submesh_name = fmt::format("'{}' mesh, primitive #{}", mesh_name, primitive_index);
	auto submesh      = std::make_unique<vkb::scene_graph::components::HPPSubMesh>(submesh_name);

	parse_primitive_attributes(gltf_primitive.attributes, mesh_name, primitive_index, *submesh);
	parse_primitive_indices(gltf_primitive.indices, mesh_name, primitive_index, *submesh);
	parse_primitive_material(gltf_primitive.material, materials, *submesh);

	return submesh;
}

void HPPGLTFLoader::parse_primitive_attributes(std::map<std::string, int> const &gltf_attributes, std::string const &mesh_name, size_t primitive_index,
                                               vkb::scene_graph::components::HPPSubMesh &submesh) const
{
	for (auto &gltf_attribute : gltf_attributes)
	{
		std::string attrib_name = gltf_attribute.first;
		std::transform(attrib_name.begin(), attrib_name.end(), attrib_name.begin(), ::tolower);

		if (attrib_name == "position")
		{
			assert(gltf_attribute.second < model.accessors.size());
			submesh.vertices_count = to_u32(model.accessors[gltf_attribute.second].count);
		}

		auto                 vertex_data = detail::get_attribute_data(&model, gltf_attribute.second);
		vkb::core::HPPBuffer buffer{device, vertex_data.second, vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU};
		buffer.update(vertex_data.first, vertex_data.second);
		buffer.set_debug_name(fmt::format("'{}' mesh, primitive #{}: '{}' vertex buffer", mesh_name, primitive_index, attrib_name));
		submesh.set_vertex_buffer(attrib_name, std::move(buffer));

		vkb::scene_graph::components::HPPVertexAttribute vertex_attribute;
		vertex_attribute.format = detail::get_attribute_format(&model, gltf_attribute.second);
		vertex_attribute.stride = to_u32(detail::get_attribute_stride(&model, gltf_attribute.second));
		submesh.set_attribute(attrib_name, vertex_attribute);
	}
}

void HPPGLTFLoader::parse_primitive_indices(int indicesId, std::string const &mesh_name, size_t primitive_index,
                                            vkb::scene_graph::components::HPPSubMesh &submesh) const
{
	if (0 <= indicesId)
	{
		submesh.vertex_indices = to_u32(detail::get_attribute_size(&model, indicesId));

		auto format = detail::get_attribute_format(&model, indicesId);

		auto indices = detail::get_attribute_data(&model, indicesId);

		std::vector<uint8_t> index_data;
		switch (format)
		{
			case vk::Format::eR8Uint:
				// Converts uint8 data into uint16 data, still represented by a uint8 vector
				index_data = detail::copy_strided_data(indices.first, indices.second, 1, 2);
				indices    = std::make_pair(index_data.data(), index_data.size());
				submesh.set_index_type(vk::IndexType::eUint16);
				break;
			case vk::Format::eR16Uint:
				submesh.set_index_type(vk::IndexType::eUint16);
				break;
			case vk::Format::eR32Uint:
				submesh.set_index_type(vk::IndexType::eUint32);
				break;
			default:
				LOGE("gltf primitive has invalid format type");
				break;
		}

		std::unique_ptr<vkb::core::HPPBuffer> index_buffer =
		    std::make_unique<vkb::core::HPPBuffer>(device, indices.second, vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
		index_buffer->set_debug_name(fmt::format("'{}' mesh, primitive #{}: index buffer", mesh_name, primitive_index));
		index_buffer->update(indices.first, indices.second);
		submesh.set_index_buffer(std::move(index_buffer));
	}
}

void HPPGLTFLoader::parse_primitive_material(int materialId, std::vector<vkb::scene_graph::components::HPPPBRMaterial *> const &materials,
                                             vkb::scene_graph::components::HPPSubMesh &submesh) const
{
	if (materialId < 0)
	{
		assert(materials.back()->get_name() == "default_material");
		submesh.set_material(*materials.back());
	}
	else
	{
		assert(materialId < materials.size());
		submesh.set_material(*materials[materialId]);
	}
}

std::unique_ptr<vkb::scene_graph::components::HPPSampler> HPPGLTFLoader::parse_sampler(const tinygltf::Sampler &gltf_sampler) const
{
	vk::SamplerCreateInfo sampler_info{};
	sampler_info.magFilter    = detail::map_mag_filter(gltf_sampler.magFilter);
	sampler_info.minFilter    = detail::map_min_filter(gltf_sampler.minFilter);
	sampler_info.mipmapMode   = detail::map_mipmap_mode(gltf_sampler.minFilter);
	sampler_info.addressModeU = detail::map_wrap_mode(gltf_sampler.wrapS);
	sampler_info.addressModeV = detail::map_wrap_mode(gltf_sampler.wrapT);
	sampler_info.addressModeW = detail::map_wrap_mode(gltf_sampler.wrapR);
	sampler_info.borderColor  = vk::BorderColor::eFloatOpaqueWhite;
	sampler_info.maxLod       = std::numeric_limits<float>::max();

	vkb::core::HPPSampler vk_sampler{device, sampler_info};
	vk_sampler.set_debug_name(gltf_sampler.name);

	return std::make_unique<vkb::scene_graph::components::HPPSampler>(gltf_sampler.name, std::move(vk_sampler));
}

std::vector<std::unique_ptr<vkb::scene_graph::components::HPPSampler>> HPPGLTFLoader::parse_samplers() const
{
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPSampler>> samplers;

	samplers.reserve(model.samplers.size() + 1);
	for (size_t sampler_index = 0; sampler_index < model.samplers.size(); sampler_index++)
	{
		samplers.push_back(parse_sampler(model.samplers[sampler_index]));
	}
	samplers.push_back(create_default_sampler());

	return samplers;
}

std::vector<std::unique_ptr<vkb::scene_graph::components::HPPTexture>> HPPGLTFLoader::parse_textures(vkb::scene_graph::HPPScene &scene) const
{
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPTexture>> textures;

	if (!model.textures.empty())
	{
		textures.reserve(model.textures.size());

		auto images   = scene.get_components<vkb::scene_graph::components::HPPImage>();
		auto samplers = scene.get_components<vkb::scene_graph::components::HPPSampler>();

		for (auto &gltf_texture : model.textures)
		{
			textures.push_back(parse_texture(gltf_texture, images, samplers));
		}
	}

	return textures;
}

std::vector<std::unique_ptr<vkb::scene_graph::components::HPPImage>>
    HPPGLTFLoader::upload_images(std::vector<std::future<std::unique_ptr<vkb::scene_graph::components::HPPImage>>> &image_futures) const
{
	auto image_count = model.images.size();

	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPImage>> images;
	images.reserve(image_count);

	// Upload images to GPU. We do this in batches of 64MB of data to avoid needing
	// double the amount of memory (all the images and all the corresponding buffers).
	// This helps keep memory footprint lower which is helpful on smaller devices.
	for (size_t image_index = 0; image_index < image_count;)
	{
		std::vector<vkb::core::HPPBuffer> transient_buffers;

		auto &command_buffer = device.get_command_pool().request_command_buffer();
		command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		size_t batch_size = 0;

		// Deal with 64MB of image data at a time to keep memory footprint low
		while (image_index < image_count && batch_size < 64 * 1024 * 1024)
		{
			// Wait for this image to complete loading, then stage for upload
			images.push_back(image_futures[image_index].get());

			auto &image = images.back();
			batch_size += image->get_data().size();

			vkb::core::HPPBuffer stage_buffer{device, image->get_data().size(), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY};
			stage_buffer.update(image->get_data());

			detail::upload_image_to_gpu(command_buffer, stage_buffer, *image);

			transient_buffers.push_back(std::move(stage_buffer));

			image_index++;
		}

		command_buffer.end();

		auto &queue = device.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);

		queue.submit(command_buffer, device.get_fence_pool().request_fence());

		device.get_fence_pool().wait();
		device.get_fence_pool().reset();
		device.get_command_pool().reset_pool();
		device.get_handle().waitIdle();

		// Remove the staging buffers for the batch we just processed
		transient_buffers.clear();
	}

	return images;
}

std::unique_ptr<vkb::scene_graph::HPPNode> HPPGLTFLoader::parse_scene(vkb::scene_graph::HPPScene const &scene, int scene_index) const
{
	tinygltf::Scene const *gltf_scene{nullptr};

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

	auto root_node = std::make_unique<vkb::scene_graph::HPPNode>(0, gltf_scene->name);

	std::queue<std::pair<vkb::scene_graph::HPPNode &, int>> traverse_nodes;
	for (auto node_index : gltf_scene->nodes)
	{
		traverse_nodes.push(std::make_pair(std::ref(*root_node), node_index));
	}

	auto const &nodes = scene.get_nodes();
	while (!traverse_nodes.empty())
	{
		auto traverse_node = traverse_nodes.front();
		traverse_nodes.pop();

		assert(traverse_node.second < nodes.size());
		auto &current_node       = *nodes[traverse_node.second];
		auto &traverse_root_node = traverse_node.first;

		current_node.set_parent(traverse_root_node);
		traverse_root_node.add_child(current_node);

		for (auto child_node_index : model.nodes[traverse_node.second].children)
		{
			traverse_nodes.push(std::make_pair(std::ref(current_node), child_node_index));
		}
	}

	return root_node;
}

std::unique_ptr<vkb::scene_graph::components::HPPTexture>
    HPPGLTFLoader::parse_texture(const tinygltf::Texture &gltf_texture, std::vector<vkb::scene_graph::components::HPPImage *> const &images,
                                 std::vector<vkb::scene_graph::components::HPPSampler *> const &samplers) const
{
	auto texture = std::make_unique<vkb::scene_graph::components::HPPTexture>(gltf_texture.name);

	assert(gltf_texture.source < images.size());
	texture->set_image(*images[gltf_texture.source]);

	if ((0 <= gltf_texture.sampler) && (gltf_texture.sampler < static_cast<int>(samplers.size())))
	{
		texture->set_sampler(*samplers[gltf_texture.sampler]);
	}
	else
	{
		assert(samplers.back()->get_name() == "default_sampler");
		texture->set_sampler(*samplers.back());
	}

	return texture;
}
}        // namespace vkb
