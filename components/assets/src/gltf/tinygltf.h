#pragma once

#include <components/common/error.hpp>
#include <components/vfs/filesystem.hpp>
#include <components/vfs/helpers.hpp>

// required as windows.h defines min and max macros
#ifdef max
#	undef max
#endif

// required for tinygltf
#include <fstream>
#include <limits>

VKBP_DISABLE_WARNINGS()
#define TINYGLTF_NO_FS                     // disable filesystem support
#define TINYGLTF_NO_STB_IMAGE              // disable stb_image support
#define TINYGLTF_NO_STB_IMAGE_WRITE        // disable stb_image_write support
#define TINYGLTF_NO_EXTERNAL_IMAGE         // disable external image loading - we want to do this manually
#include <tiny_gltf.h>
VKBP_ENABLE_WARNINGS()

namespace components
{
namespace gltf
{

struct UserData
{
	tinygltf::Model *model;
	vfs::FileSystem *fs;
	std::string      root_directory;
};

// implemented in filesystem.cpp

bool FileExistsFunction(const std::string &abs_filename, void *data);

std::string ExpandFilePathFunction(const std::string &file_path, void *data);

bool ReadWholeFileFunction(std::vector<unsigned char> *out_data, std::string *err, const std::string &file_path, void *data);
}        // namespace gltf
}        // namespace components