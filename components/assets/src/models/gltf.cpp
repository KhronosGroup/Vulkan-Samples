#include "components/models/gltf.hpp"

#include <fstream>        // < required for tiny gltf to build

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_NO_FS
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include <Tracy.hpp>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include <components/images/image.hpp>
#include <components/images/ktx.hpp>
#include <components/images/stb.hpp>
#include <components/scene_graph/components/material.hpp>
#include <components/scene_graph/components/mesh.hpp>
#include <components/vfs/helpers.hpp>

struct pair_hash
{
	template <class T1, class T2>
	std::size_t operator()(const std::pair<T1, T2> &pair) const
	{
		return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
	}
};

namespace components
{
namespace models
{
// Helpers

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

inline VkFilter to_vulkan_filter(int min_filter)
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

inline VkSamplerMipmapMode to_vulkan_mipmap_mode(int min_filter)
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

inline VkSamplerAddressMode to_vulkan_address_mode(int wrap)
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

inline sg::TextureType to_texture_type(const std::string &type_str)
{
	if (type_str == "baseColorTexture")
	{
		return sg::TextureType::BaseColorTexture;
	}
	else if (type_str == "normalTexture")
	{
		return sg::TextureType::NormalTexture;
	}
	else if (type_str == "occlusionTexture")
	{
		return sg::TextureType::OcclusionTexture;
	}
	else if (type_str == "emissiveTexture")
	{
		return sg::TextureType::EmissiveTexture;
	}
	else if (type_str == "metallicRoughnessTexture")
	{
		return sg::TextureType::MetallicRoughnessTexture;
	}

	return sg::TextureType::Max;
}

inline VkPrimitiveTopology to_primitive_topology(const int topology)
{
	if (topology == TINYGLTF_MODE_POINTS)
	{
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	}
	else if (topology == TINYGLTF_MODE_LINE)
	{
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	}
	else if (topology == TINYGLTF_MODE_LINE_LOOP)
	{
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;        // ! Not Supported
	}
	else if (topology == TINYGLTF_MODE_LINE_STRIP)
	{
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	}
	else if (topology == TINYGLTF_MODE_TRIANGLES)
	{
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
	else if (topology == TINYGLTF_MODE_TRIANGLE_STRIP)
	{
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	}
	else if (topology == TINYGLTF_MODE_TRIANGLE_FAN)
	{
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	}

	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

inline sg::AttributeType to_attribute(const std::string &attrib)
{
	if (attrib == "POSITION")
	{
		return sg::AttributeType::Position;
	}
	else if (attrib == "NORMAL")
	{
		return sg::AttributeType::Normal;
	}
	else if (attrib == "TANGENT")
	{
		return sg::AttributeType::Tangent;
	}
	else if (attrib == "TEXCOORD_0")
	{
		return sg::AttributeType::TexCoord_0;
	}
	else if (attrib == "TEXCOORD_1")
	{
		return sg::AttributeType::TexCoord_1;
	}
	else if (attrib == "COLOR_0")
	{
		return sg::AttributeType::Color_0;
	}
	else if (attrib == "JOINTS_0")
	{
		return sg::AttributeType::Joints_0;
	}
	else if (attrib == "WEIGHTS_0")
	{
		return sg::AttributeType::Weights_0;
	}
	return sg::AttributeType::Max;
}

inline VkFormat get_attribute_format(const tinygltf::Accessor &accessor)
{
	switch (accessor.componentType)
	{
		case TINYGLTF_COMPONENT_TYPE_BYTE: {
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_SINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_SINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_SINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_SINT}};

			return mapped_format.at(accessor.type);
		}
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
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
				return mapped_format_normalize.at(accessor.type);
			}

			return mapped_format.at(accessor.type);
		}
		case TINYGLTF_COMPONENT_TYPE_SHORT: {
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R8_SINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R8G8_SINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R8G8B8_SINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R8G8B8A8_SINT}};

			return mapped_format.at(accessor.type);
		}
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
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
				return mapped_format_normalize.at(accessor.type);
			}

			return mapped_format.at(accessor.type);
		}
		case TINYGLTF_COMPONENT_TYPE_INT: {
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_SINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R32G32_SINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R32G32B32_SINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R32G32B32A32_SINT}};

			return mapped_format.at(accessor.type);
		}
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_UINT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R32G32_UINT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R32G32B32_UINT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R32G32B32A32_UINT}};

			return mapped_format.at(accessor.type);
		}
		case TINYGLTF_COMPONENT_TYPE_FLOAT: {
			static const std::map<int, VkFormat> mapped_format = {{TINYGLTF_TYPE_SCALAR, VK_FORMAT_R32_SFLOAT},
			                                                      {TINYGLTF_TYPE_VEC2, VK_FORMAT_R32G32_SFLOAT},
			                                                      {TINYGLTF_TYPE_VEC3, VK_FORMAT_R32G32B32_SFLOAT},
			                                                      {TINYGLTF_TYPE_VEC4, VK_FORMAT_R32G32B32A32_SFLOAT}};

			return mapped_format.at(accessor.type);
		}
		default: {
			return VK_FORMAT_UNDEFINED;
		}
	}
}

// Helpers

struct FSContext
{
	FSContext(vfs::FileSystem &fs, const std::string &wd) :
	    fs{fs}, model_working_directory{wd}
	{
	}

	vfs::FileSystem &fs;
	std::string      model_working_directory;
};

bool file_exists(const std::string &abs_filename, void *user_data)
{
	auto &context = *reinterpret_cast<FSContext *>(user_data);
	return context.fs.file_exists(vfs::helpers::join({context.model_working_directory, abs_filename}));
}

std::string expand_file_path(const std::string &filepath, void *user_data)
{
	return vfs::helpers::sanitize(filepath);
}

bool read_whole_file(std::vector<unsigned char> *out, std::string *err, const std::string &filepath, void *user_data)
{
	auto &context = *reinterpret_cast<FSContext *>(user_data);

	std::shared_ptr<vfs::Blob> blob;

	auto path = filepath;

	if (filepath.substr(0, context.model_working_directory.size()) != context.model_working_directory)
	{
		path = vfs::helpers::join({context.model_working_directory, filepath});
	}

	if (context.fs.read_file(path, &blob) != vfs::status::Success)
	{
		return false;
	}

	*out = blob->binary();

	return true;
}

bool write_whole_file(std::string *err, const std::string &filepath, const std::vector<unsigned char> &contents, void *user_data)
{
	auto &context = *reinterpret_cast<FSContext *>(user_data);
	return false;
}

tinygltf::FsCallbacks fs_callbacks(FSContext &context)
{
	tinygltf::FsCallbacks cb{};
	cb.FileExists     = &file_exists;
	cb.ExpandFilePath = &expand_file_path;
	cb.ReadWholeFile  = &read_whole_file;
	cb.WriteWholeFile = &write_whole_file;
	cb.user_data      = reinterpret_cast<void *>(&context);
	return cb;
}

StackErrorPtr GltfLoader::load_from_file(const std::string &model_name, vfs::FileSystem &fs, const std::string &path, sg::NodePtr *o_root) const
{
	ZoneScopedN("GltfLoader::load_from_file");

	FSContext fs_context{
	    fs,
	    vfs::helpers::get_directory(path),
	};

	tinygltf::TinyGLTF loader;
	loader.SetFsCallbacks(fs_callbacks(fs_context));

	tinygltf::Model model;

	std::string err;
	std::string warn;
	bool        result{false};

	if (path.substr(path.length() - 5, 5) == ".gltf")
	{
		result = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	}
	else if (path.substr(path.length() - 4, 4) == ".glb")
	{
		result = loader.LoadBinaryFromFile(&model, &err, &warn, path);
	}
	else
	{
		return StackError::unique("could not parse file: " + path + "\n\tMake sure it is a .gltf or .glb file", "models/gltf.cpp", __LINE__);
	}

	if (!result)
	{
		std::string reason;

		if (!err.empty())
		{
			reason += err;
		}

		if (!warn.empty())
		{
			if (!reason.empty())
			{
				reason += "\n";
			}

			reason += warn;
		}

		return StackError::unique("failed to load: " + path + "\n\treason: " + reason, "models/gltf.cpp", __LINE__);
	}

	// Process Resources

	// Process Buffers

	std::vector<sg::Buffer> buffers;

	{
		ZoneScopedN("buffers");

		buffers.reserve(model.bufferViews.size());
		for (auto &gltf_buffer_view : model.bufferViews)
		{
			auto &gltf_buffer = model.buffers[gltf_buffer_view.buffer];
			auto  start       = gltf_buffer_view.byteOffset;
			auto  end         = start + gltf_buffer_view.byteLength;
			buffers.push_back(std::make_shared<std::vector<uint8_t>>(gltf_buffer.data.data() + start, gltf_buffer.data.data() + end));
		}
	}

	// Process Samplers

	std::vector<sg::Sampler> samplers;

	{
		ZoneScopedN("samplers");

		samplers.reserve(model.samplers.size());
		for (auto &gltf_sampler : model.samplers)
		{
			sg::Sampler sampler;
			sampler.min_filter     = to_vulkan_filter(gltf_sampler.minFilter);
			sampler.mag_filter     = to_vulkan_filter(gltf_sampler.magFilter);
			sampler.mipmap_mode    = to_vulkan_mipmap_mode(gltf_sampler.minFilter);
			sampler.address_mode_u = to_vulkan_address_mode(gltf_sampler.wrapS);
			sampler.address_mode_v = to_vulkan_address_mode(gltf_sampler.wrapT);
			samplers.push_back(sampler);
		}
	}

	// Process Images
	std::vector<images::ImagePtr> images;

	{
		ZoneScopedN("images");

		auto ktx_loader = std::make_unique<images::KtxLoader>();
		auto stb_loader = std::make_unique<images::StbLoader>();

		std::unordered_map<std::string, images::ImageLoader *> loaders = {};

		loaders[".ktx"] = ktx_loader.get();
		loaders[".png"] = stb_loader.get();

		images.reserve(model.images.size());

		for (auto &gltf_image : model.images)
		{
			auto path      = vfs::helpers::join({fs_context.model_working_directory, gltf_image.uri});
			auto extension = vfs::helpers::get_file_extension(path);

			auto loader = loaders.find(extension);

			if (loader == loaders.end())
			{
				return StackError::unique("no image loader found for " + path, "models/gltf.cpp", __LINE__);
			}

			images::ImagePtr image;

			if (auto err = loader->second->load_from_file(gltf_image.name, fs, path, &image))
			{
				err->push("failed to load image", "models/gltf.cpp", __LINE__);
				return std::move(err);
			}

			images.push_back(image);
		}
	}

	// Process Textures

	std::vector<sg::Texture> textures;

	{
		ZoneScopedN("textures");

		textures.reserve(model.textures.size());

		for (auto &gltf_texture : model.textures)
		{
			sg::Texture texture;

			if (gltf_texture.sampler > 0)
			{
				texture.sampler = samplers[gltf_texture.sampler];
			}
			else
			{
				// Default Sampler
				texture.sampler = sg::Sampler{
				    /* min_filter = */ VK_FILTER_LINEAR,
				    /* mag_filter = */ VK_FILTER_LINEAR,
				    /* mipmap_mode = */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
				    /* address_mode_u = */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
				    /* address_mode_v = */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
				};
			}

			if (gltf_texture.source > 0)
			{
				texture.image = images[gltf_texture.source];
			}
		}
	}

	// Process All Materials

	std::vector<sg::MaterialPtr> materials;

	{
		ZoneScopedN("images");

		materials.reserve(model.materials.size());

		for (auto &gltf_material : model.materials)
		{
			sg::MaterialPtr material = std::make_shared<sg::Material>();

			for (auto &gltf_value : gltf_material.values)
			{
				auto texture_type = to_texture_type(gltf_value.first);
				if (texture_type != sg::TextureType::Max)
				{
					if (gltf_value.second.TextureIndex() >= 0)
					{
						material->textures[texture_type] = textures[gltf_value.second.TextureIndex()];
					}
				}
				else if (gltf_value.first == "baseColorFactor")
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
				auto texture_type = to_texture_type(gltf_value.first);
				if (texture_type != sg::TextureType::Max)
				{
					if (gltf_value.second.TextureIndex() >= 0)
					{
						material->textures[texture_type] = textures[gltf_value.second.TextureIndex()];
					}
				}
				else if (gltf_value.first == "emissiveFactor")
				{
					const auto &emissive_factor = gltf_value.second.number_array;

					material->emissive_factor = glm::vec3(emissive_factor[0], emissive_factor[1], emissive_factor[2]);
				}
				else if (gltf_value.first == "alphaMode")
				{
					if (gltf_value.second.string_value == "BLEND")
					{
						material->alpha_mode = sg::AlphaMode::Blend;
					}
					else if (gltf_value.second.string_value == "OPAQUE")
					{
						material->alpha_mode = sg::AlphaMode::Opaque;
					}
					else if (gltf_value.second.string_value == "MASK")
					{
						material->alpha_mode = sg::AlphaMode::Mask;
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

			materials.push_back(material);
		}
	}
	// Process Meshes

	std::unordered_map<std::pair<size_t, size_t>, sg::NodePtr, pair_hash> meshes;

	{
		ZoneScopedN("meshes");

		{
			size_t mesh_count = 0;

			for (auto &gltf_mesh : model.meshes)
			{
				gltf_mesh.primitives.size();
			}

			meshes.reserve(mesh_count);
		}

		for (size_t mesh_index = 0; mesh_index < model.meshes.size(); mesh_index++)
		{
			auto &gltf_mesh = model.meshes[mesh_index];

			for (size_t primitive_index = 0; primitive_index < gltf_mesh.primitives.size(); primitive_index++)
			{
				std::pair<size_t, size_t> key{mesh_index, primitive_index};

				sg::Mesh mesh;

				auto &gltf_primitive = gltf_mesh.primitives[primitive_index];

				for (auto &attribute : gltf_primitive.attributes)
				{
					auto type = to_attribute(attribute.first);

					if (type == sg::AttributeType::Max)
					{
						// not supported - yet?
						continue;
					}

					auto &gltf_accessor = model.accessors[attribute.second];

					sg::VertexAttribute v_attrib;
					v_attrib.count  = static_cast<uint32_t>(gltf_accessor.count);
					v_attrib.offset = static_cast<uint32_t>(gltf_accessor.byteOffset);
					v_attrib.stride = static_cast<uint32_t>(gltf_accessor.ByteStride(model.bufferViews[gltf_accessor.bufferView]));
					v_attrib.format = get_attribute_format(gltf_accessor);

					auto &buffer    = buffers[gltf_accessor.bufferView];
					v_attrib.buffer = buffer;

					mesh.vertex_attributes[type] = v_attrib;
				}

				if (gltf_primitive.indices >= 0)
				{
					auto &gltf_accessor = model.accessors[gltf_primitive.indices];

					sg::VertexAttribute v_attrib;
					v_attrib.count  = static_cast<uint32_t>(gltf_accessor.count);
					v_attrib.offset = static_cast<uint32_t>(gltf_accessor.byteOffset);
					v_attrib.stride = static_cast<uint32_t>(gltf_accessor.ByteStride(model.bufferViews[gltf_accessor.bufferView]));
					v_attrib.format = get_attribute_format(gltf_accessor);

					auto &buffer    = buffers[gltf_accessor.bufferView];
					v_attrib.buffer = buffer;

					mesh.indices = v_attrib;
				}

				mesh.topology = to_primitive_topology(gltf_primitive.mode);

				if (gltf_primitive.material >= 0)
				{
					mesh.material = materials[gltf_primitive.material];
				}

				auto mesh_node = sg::Node::ptr(m_registry, fmt::format("{} - sub mesh {}", gltf_mesh.name, primitive_index));

				m_registry->emplace<sg::Mesh>(mesh_node->entity, mesh);

				meshes.emplace(key, mesh_node);
			}
		}
	}
	// Process Scene

	// Preallocate Nodes

	sg::NodePtr root_node_ptr{nullptr};

	std::vector<sg::NodePtr> nodes;
	nodes.reserve(model.nodes.size());

	for (auto &gltf_node : model.nodes)
	{
		nodes.emplace_back(sg::Node::ptr(m_registry, gltf_node.name));
	}

	// Select Root Node

	if (model.defaultScene >= 0)
	{
		root_node_ptr = nodes[model.defaultScene];

		for (size_t node_index : model.scenes[model.defaultScene].nodes)
		{
			if (model.defaultScene != node_index)
			{
				root_node_ptr->add_child(nodes[node_index]);
			}
		}
	}
	else
	{
		// No root node - all nodes should be treated as individuals - group them
		root_node_ptr = sg::Node::ptr(m_registry, fmt::format("Node Group: {}", model_name));

		for (auto &node : nodes)
		{
			root_node_ptr->add_child(node);
		}
	}

	// Process Nodes
	{
		ZoneScopedN("nodes");

		for (size_t node_index = 0; node_index < model.nodes.size(); node_index++)
		{
			auto &gltf_node  = model.nodes[node_index];
			auto &scene_node = nodes[node_index];

			// Process Node Transforms
			if (!gltf_node.translation.empty())
			{
				std::transform(gltf_node.translation.begin(), gltf_node.translation.end(), glm::value_ptr(scene_node->transform.position), TypeCast<double, float>{});
			}

			if (!gltf_node.rotation.empty())
			{
				std::transform(gltf_node.rotation.begin(), gltf_node.rotation.end(), glm::value_ptr(scene_node->transform.rotation), TypeCast<double, float>{});
			}

			if (!gltf_node.scale.empty())
			{
				std::transform(gltf_node.scale.begin(), gltf_node.scale.end(), glm::value_ptr(scene_node->transform.scale), TypeCast<double, float>{});
			}

			if (!gltf_node.matrix.empty())
			{
				glm::mat4 matrix;

				std::transform(gltf_node.matrix.begin(), gltf_node.matrix.end(), glm::value_ptr(matrix), TypeCast<double, float>{});

				// decompose matrix into transform components
				glm::vec3 skew;
				glm::vec4 perspective;
				glm::decompose(matrix, scene_node->transform.scale, scene_node->transform.rotation, scene_node->transform.position, skew, perspective);
				scene_node->transform.rotation = glm::conjugate(scene_node->transform.rotation);
			}

			if (gltf_node.mesh >= 0)
			{
				auto &gltf_mesh = model.meshes[gltf_node.mesh];

				for (size_t primitive_index = 0; primitive_index < gltf_mesh.primitives.size(); primitive_index++)
				{
					auto it = meshes.find({gltf_node.mesh, primitive_index});

					if (it != meshes.end())
					{
						scene_node->add_child(it->second);
					}
				}
			}

			for (size_t child_index : gltf_node.children)
			{
				scene_node->add_child(nodes[child_index]);
			}
		}
	}

	*o_root = root_node_ptr;

	return nullptr;
}
}        // namespace models
}        // namespace components
