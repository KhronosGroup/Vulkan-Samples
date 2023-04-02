#pragma once

#include <components/vfs/filesystem.hpp>
#include <components/vfs/std_filesystem.hpp>

#include <filesystem>

components::vfs::FileSystem *get_asset_fs()
{
	static components::vfs::RootFileSystem fs;
	static bool                            initialized = false;
	if (!initialized)
	{
		initialized = true;

		std::filesystem::path path   = __FILE__;
		auto                  folder = path.parent_path();
		fs.mount("/", std::make_shared<components::vfs::StdFSFileSystem>(folder.c_str()));
	}
	return &fs;
}

std::vector<uint8_t> load_asset(const std::string &path)
{
	return get_asset_fs()->read_file(path);
}

std::vector<uint8_t> BASE_FRAG = load_asset("/base.frag");

std::vector<uint8_t> HELLO_TRIANGLE_VERT = load_asset("/hello_triangle.vert");

std::vector<uint8_t> HELLO_TRIANGLE_FRAG = load_asset("/hello_triangle.frag");