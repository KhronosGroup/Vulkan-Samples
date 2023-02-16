#pragma once

#include "components/assets/model_loader.hpp"

namespace components
{
namespace gltf
{
class GLTFModelLoader : public assets::ModelLoader
{
  public:
	void load_from_file(vfs::FileSystem &fs, const std::string &path, sg::NodePtr scene) const override;
};
}        // namespace gltf
}        // namespace components