#pragma once

#include <components/scene_graph/graph.hpp>
#include <components/vfs/filesystem.hpp>

namespace components
{
namespace assets
{
class ModelLoader
{
  public:
	ModelLoader()          = default;
	virtual ~ModelLoader() = default;

	// Load a model from a file into a given scene graph node
	virtual void load_from_file(vfs::FileSystem &fs, const std::string &path, sg::NodePtr scene) const = 0;
};
}        // namespace assets
}        // namespace components