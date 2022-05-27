#pragma once

#include <components/common/stack_error.hpp>
#include <components/scene_graph/graph.hpp>
#include <components/vfs/filesystem.hpp>

namespace components
{
namespace models
{
class ModelLoader
{
  public:
	ModelLoader()          = default;
	virtual ~ModelLoader() = default;

	virtual StackErrorPtr load_from_file(const std::string &model_name, vfs::FileSystem &fs, const std::string &path, sg::NodePtr *o_root) const = 0;
};
}        // namespace models
}        // namespace components