#pragma once

#include <string>

namespace vfs
{
namespace helpers
{
std::string directory_path(const std::string &path);

std::string strip_directory(const std::string &path);
}        // namespace helpers
}        // namespace vfs