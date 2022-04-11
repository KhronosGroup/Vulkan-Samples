#include "components/vfs/helpers.hpp"

namespace vfs
{
namespace helpers
{
std::string directory_path(const std::string &path)
{
	return path.substr(0, path.rfind("/"));
}

std::string strip_directory(const std::string &path)
{
	return path.substr(path.rfind("/"), path.size());
}
}        // namespace helpers
}        // namespace vfs