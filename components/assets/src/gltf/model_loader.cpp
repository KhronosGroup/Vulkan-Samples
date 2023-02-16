#include "components/gltf/model_loader.hpp"

#define TINYGLTF_IMPLEMENTATION
#include "tinygltf.h"

#include "lookups.h"
#include "transform.h"

namespace components
{
namespace gltf
{
void GLTFModelLoader::load_from_file(vfs::FileSystem &fs, const std::string &path, sg::NodePtr scene) const
{
	auto        data = fs.read_file(path);
	std::string gltf_data(data.begin(), data.end());

	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;

	UserData user_data{
	    &model,
	    &fs,
	    vfs::helpers::get_directory(path),
	};

	loader.SetFsCallbacks(tinygltf::FsCallbacks{
	    FileExistsFunction,
	    ExpandFilePathFunction,
	    ReadWholeFileFunction,
	    nullptr,
	    &user_data,        // pass the proxy file system
	});

	bool ret = loader.LoadASCIIFromString(&model, &err, &warn, gltf_data.c_str(), static_cast<unsigned int>(gltf_data.size()), vfs::helpers::get_directory(path));

	if (!warn.empty())
	{
		ERRORF("Warn: {}\n", warn);
	}

	if (!err.empty())
	{
		ERRORF("Err: {}\n", err);
	}

	if (!ret)
	{
		ERRORF("Failed to parse glTF\n");
	}

	// process buffers
	std::vector<sg::Buffer> buffers;
	for (auto &buffer : model.buffers)
	{
		buffers.emplace_back(sg::make_buffer(std::move(buffer.data)));
	}

	// process images
	std::vector<assets::ImageAssetPtr> images;

	// process samplers
	std::vector<sg::Sampler> samplers;

	for (auto &gltf_sampler : model.samplers)
	{
		sg::Sampler sampler;
		sampler.address_mode_u = find_wrap_mode(gltf_sampler.wrapS);
		sampler.address_mode_v = find_wrap_mode(gltf_sampler.wrapT);
		sampler.mag_filter     = find_mag_filter(gltf_sampler.magFilter);
		sampler.min_filter     = find_min_filter(gltf_sampler.minFilter);
		samplers.push_back(sampler);
	}

	std::vector<sg::Texture> textures;

	// process materials
	std::vector<sg::Material> materials;

	const auto load_texture = [&](tinygltf::Parameter &gltf_texture_params) -> sg::Texture {
		return {};
	};

	for (auto &gltf_material : model.materials)
	{
		sg::Material material;

		for (auto &[texture_type, gltf_texture_params] : gltf_material.values)
		{
			if (gltf_texture_params.TextureIndex() < 0)
			{
				continue;
			}

			material.textures[get_texture_type(texture_type)] = load_texture(gltf_texture_params);
		}

		for (auto &[texture_type, gltf_texture_params] : gltf_material.additionalValues)
		{
			if (gltf_texture_params.TextureIndex() < 0)
			{
				continue;
			}

			material.textures[get_texture_type(texture_type)] = load_texture(gltf_texture_params);
		}

		materials.push_back(material);
	}

	// process meshes

	// each mesh is represented by 1 or more primitives we have a scene node per primitive
	std::vector<std::vector<sg::NodePtr>> meshes;

	for (auto &gltf_mesh : model.meshes)
	{
		std::vector<sg::NodePtr> nodes;

		for (auto &gltf_primitive : gltf_mesh.primitives)
		{
			sg::Mesh mesh;

			// process indices
			if (gltf_primitive.indices >= 0)
			{
				auto &gltf_accessor    = model.accessors[gltf_primitive.indices];
				auto &gltf_buffer_view = model.bufferViews[gltf_accessor.bufferView];

				sg::VertexAttribute attribute;
				attribute.buffer = buffers[gltf_buffer_view.buffer];
				attribute.format = get_attribute_format(gltf_accessor);
				attribute.stride = gltf_accessor.ByteStride(gltf_buffer_view);
				attribute.offset = static_cast<uint32_t>(gltf_accessor.byteOffset + gltf_buffer_view.byteOffset);
				attribute.count  = static_cast<uint32_t>(gltf_accessor.count);

				mesh.indices = attribute;
			}

			// process vertex attributes
			for (auto attribute : gltf_primitive.attributes)
			{
				sg::AttributeType type = get_attribute_type(attribute.first);

				auto &gltf_accessor    = model.accessors[attribute.second];
				auto &gltf_buffer_view = model.bufferViews[gltf_accessor.bufferView];

				sg::VertexAttribute attribute;
				attribute.buffer = buffers[gltf_buffer_view.buffer];
				attribute.format = get_attribute_format(gltf_accessor);
				attribute.stride = gltf_accessor.ByteStride(gltf_buffer_view);
				attribute.offset = static_cast<uint32_t>(gltf_accessor.byteOffset + gltf_buffer_view.byteOffset);
				attribute.count  = static_cast<uint32_t>(gltf_accessor.count);

				mesh.vertex_attributes[type] = attribute;
			}

			mesh.topology = get_topology(gltf_primitive.mode);

			if (gltf_primitive.material >= 0)
			{
				mesh.material = materials[gltf_primitive.material];
			}

			sg::NodePtr primitive_node = sg::Node::create(scene->registry(), gltf_mesh.name + "_primitive");
			primitive_node->set_component<sg::Mesh>(mesh);
			nodes.push_back(primitive_node);
		}

		meshes.push_back(nodes);
	}

	// 4. process nodes
	std::unordered_map<int, sg::NodePtr> nodes;

	// process an individually node
	// does not relate children
	const auto process_node = [&](int index) {
		auto &gltf_node = model.nodes[index];

		sg::NodePtr node_root;

		if (gltf_node.mesh >= 0)
		{
			auto &primitives = meshes[gltf_node.mesh];

			if (primitives.size() == 0)
			{
				// the primitive is the root
				node_root = primitives[0];
			}
			else
			{
				// create a proxy root
				node_root = sg::Node::create(scene->registry(), gltf_node.name);
				for (auto &primitive : primitives)
				{
					// add the primitive as a child
					node_root->add_child(primitive);
				}
			}
		}

		// could not get the node from the mesh so create an empty node
		if (!node_root)
		{
			node_root = sg::Node::create(scene->registry(), gltf_node.name);
		}

		// parse the transform
		node_root->set_component<sg::Transform>(parse_node_transform(gltf_node));

		if (gltf_node.camera >= 0)
		{
			NOT_IMPLEMENTED();
		}

		if (gltf_node.skin >= 0)
		{
			NOT_IMPLEMENTED();
		}

		nodes[index] = node_root;
	};

	// process nodes and relate children
	for (int node_index = 0; node_index < static_cast<int>(model.nodes.size()); node_index++)
	{
		if (nodes.find(node_index) == nodes.end())
		{
			process_node(node_index);
		}

		assert(nodes.find(node_index) != nodes.end() && "process_node must add a valid node to the map");
		// avoid multiple lookups in the loop by storing the node locally
		sg::NodePtr node = nodes[node_index];

		// process children
		auto &gltf_node = model.nodes[node_index];
		for (auto &child : gltf_node.children)
		{
			if (nodes.find(child) == nodes.end())
			{
				process_node(child);
			}

			assert(nodes.find(child) != nodes.end() && "process_node must add a valid node to the map");
			node->add_child(nodes[child]);
		}
	}

	// process the scene
	if (model.defaultScene >= 0)
	{
		// try use the default scene
		auto &gltf_scene = model.scenes[model.defaultScene];

		for (auto &node : gltf_scene.nodes)
		{
			scene->add_child(nodes[node]);
		}
	}
	else
	{
		// else return the first node
		if (nodes.size() > 0)
		{
			scene->add_child(nodes[0]);
		}
	}
}
}        // namespace gltf
}        // namespace components
