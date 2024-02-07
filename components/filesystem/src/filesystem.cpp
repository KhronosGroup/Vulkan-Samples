#include "filesystem/filesystem.hpp"

#include "core/platform/context.hpp"
#include "core/util/error.hpp"

#include "std_filesystem.hpp"

namespace vkb
{
namespace filesystem
{
static FileSystemPtr fs = nullptr;

void init(const PlatformContext &context)
{
	fs = std::make_shared<StdFileSystem>(
	    context.external_storage_directory(),
	    context.temp_directory());
}

FileSystemPtr get()
{
	assert(fs && "Filesystem not initialized");
	return fs;
}

void FileSystem::write_file(const std::string &path, const std::string &data)
{
	write_file(path, std::vector<uint8_t>(data.begin(), data.end()));
}

std::string FileSystem::read_file(const std::string &path)
{
	auto bin = read_binary_file(path);
	return {bin.begin(), bin.end()};
}

namespace helpers
{
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
}        // namespace helpers
}        // namespace filesystem
}        // namespace vkb