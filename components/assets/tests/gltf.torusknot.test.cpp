#include <catch2/catch.hpp>

#include <vector>

#include <components/models/gltf.hpp>
#include <components/scene_graph/components/mesh.hpp>
#include <components/vfs/filesystem.hpp>
#include <components/vfs/helpers.hpp>

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <entt/entt.hpp>

using namespace components;

TEST_CASE("test that loading a simple glTF is correct", "[common]")
{
	auto &fs = vfs::_default();

	auto registry = sg::registry();

	sg::NodePtr root{nullptr};

	models::GltfLoader loader{registry};

	auto err = loader.load_from_file("Torus Knot", fs, "/assets/scenes/torusknot.gltf", &root);
	if (err)
	{
		INFO(std::string{err->what()});
	}
	REQUIRE(err == nullptr);

	REQUIRE(root != nullptr);
	REQUIRE(root->name == "default");
	REQUIRE(root->children().size() == 1);        // < 1 mesh

	auto child = root->children()[0];

	REQUIRE(child->name == "default - sub mesh 0");

	const auto &mesh = registry->get<const sg::Mesh>(child->entity);
}