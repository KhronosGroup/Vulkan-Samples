#pragma once

#include "components/models/model.hpp"

namespace components
{
namespace models
{
class GltfLoader final : public ModelLoader
{
  public:
	GltfLoader(sg::Registry &registry) :
	    m_registry{registry}
	{}

	virtual ~GltfLoader() = default;

	virtual StackErrorPtr load_from_file(const std::string &model_name, vfs::FileSystem &fs, const std::string &path, sg::NodePtr *o_root) const override;

  private:
	mutable sg::Registry m_registry;
};
}        // namespace models
}        // namespace components