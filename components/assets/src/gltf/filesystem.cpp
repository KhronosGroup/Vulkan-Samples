#include "tinygltf.h"

#include <components/common/strings.hpp>

namespace components
{
namespace gltf
{
bool FileExistsFunction(const std::string &abs_filename, void *data)
{
	auto *user_data = static_cast<UserData *>(data);
	if (!user_data)
	{
		return false;
	}

	return user_data->fs->file_exists(abs_filename);
}

std::string ExpandFilePathFunction(const std::string &file_path, void *data)
{
	auto *user_data = static_cast<UserData *>(data);
	if (!user_data)
	{
		return "";
	}

	auto path = strings::replace_all(file_path, "./", "");

	if (path != file_path)
	{
		// path was relative - map to absolute path
		return vfs::helpers::join({user_data->root_directory, path});
	}

	// use default value
	return path;
}

bool ReadWholeFileFunction(std::vector<unsigned char> *out_data, std::string *err, const std::string &file_path, void *data)
{
	auto *user_data = static_cast<UserData *>(data);
	if (!user_data)
	{
		return false;
	}

	try
	{
		*out_data = user_data->fs->read_file(file_path);
		return true;
	}
	catch (const std::exception &e)
	{
		*err = e.what();
		return false;
	}
}
}        // namespace gltf
}        // namespace components