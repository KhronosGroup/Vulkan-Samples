#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <catch2/catch_test_macros.hpp>
VKBP_ENABLE_WARNINGS()

#include <components/gltf/model_loader.hpp>
#include <components/scene_graph/components/mesh.hpp>
#include <components/scene_graph/graph.hpp>

using namespace components;

namespace
{
const char *TEST_MODEL = "/tests/assets/BoxTextured/BoxTextured.gltf";
}        // namespace

TEST_CASE("load BoxTextured", "[assets][gltf]")
{
	auto &fs = vfs::_default();

	gltf::GLTFModelLoader loader;

	auto        scene_registry = sg::create_registry();
	sg::NodePtr graph          = sg::Node::create(scene_registry, "scene graph");

	loader.load_from_file(fs, TEST_MODEL, graph);

	sg::NodePtr model_root = nullptr;
	{
		// model root assertions
		REQUIRE(graph->children().size() == 1);
		model_root = graph->children()[0];

		// shouldn't contain a mesh component
		REQUIRE(!model_root->has_components<sg::Mesh>());
		REQUIRE(model_root->has_components<sg::SceneNode, sg::Transform>());
	}

	sg::NodePtr mesh_node = nullptr;
	{
		// mesh node assertions
		REQUIRE(model_root->children().size() == 1);
		mesh_node = model_root->children()[0];

		// should contain a mesh component
		REQUIRE(mesh_node->has_components<sg::SceneNode, sg::Transform>());
	}

	sg::NodePtr mesh_primitive_node = nullptr;
	{
		// mesh node assertions
		REQUIRE(mesh_node->children().size() == 1);
		mesh_primitive_node = mesh_node->children()[0];

		// should contain a mesh component
		REQUIRE(mesh_primitive_node->has_components<sg::SceneNode, sg::Transform, sg::Mesh>());
	}
}
