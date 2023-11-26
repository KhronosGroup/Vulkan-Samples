#include "filesystem/filesystem.hpp"

#include "core/util/error.hpp"

#include "std_filesystem.hpp"

namespace vkb
{
namespace fs
{
void FileSystem::write_file(const std::string &path, const std::string &data)
{
	write_file(path, std::vector<uint8_t>(data.begin(), data.end()));
}

std::string FileSystem::read_file(const std::string &path)
{
	auto bin = read_binary_file(path);
	return {bin.begin(), bin.end()};
}

FileSystemPtr get_filesystem()
{
	static auto fs = std::make_shared<StdFileSystem>();
	return fs;
}

std::string filename(const std::string &path)
{
	auto pos = path.find_last_of("/\\");
	if (pos == std::string::npos)
	{
		return path;
	}
	else
	{
		return path.substr(pos + 1);
	}
}
}        // namespace fs
}        // namespace vkb