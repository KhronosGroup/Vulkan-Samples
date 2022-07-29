#include <catch2/catch_test_macros.hpp>

#include <set>
#include <sstream>
#include <string>

#include <components/vfs/filesystem.hpp>

using namespace components;

#define CHECK_ERROR(err)         \
	if (err != nullptr)          \
	{                            \
		INFO(err->what());       \
		REQUIRE(err == nullptr); \
	}

TEST_CASE("Create and delete a temporary file", "[vfs]")
{
	auto &fs = vfs::_default();

	std::string          file_path     = "/temp/vkb_basic_test_file.txt";
	std::string          file_contents = "this is the file contents";
	std::vector<uint8_t> file_data{file_contents.begin(), file_contents.end()};

	auto err = fs.write_file(file_path, file_data.data(), file_data.size());
	CHECK_ERROR(err);

	std::shared_ptr<vfs::Blob> blob;
	err = fs.read_file(file_path, &blob);
	CHECK_ERROR(err);

	REQUIRE(blob->ascii() == file_contents);

	REQUIRE(fs.remove(file_path));
}

TEST_CASE("Search for folders", "[vfs]")
{
	auto &fs = vfs::_default();

	std::vector<std::string> folders;

	auto err = fs.enumerate_folders("/bldsys", &folders);
	CHECK_ERROR(err);

	std::set<std::string> individual_folders{folders.begin(), folders.end()};

	REQUIRE(individual_folders.find("/bldsys/cmake") != individual_folders.end());
	REQUIRE(individual_folders.find("/bldsys/scripts") != individual_folders.end());

	REQUIRE(folders.size() == 2);
}

TEST_CASE("Search for folders recursive", "[vfs]")
{
	auto &fs = vfs::_default();

	std::vector<std::string> folders;

	auto err = fs.enumerate_folders_recursive("/bldsys", &folders);
	CHECK_ERROR(err);

	std::set<std::string> individual_folders{folders.begin(), folders.end()};

	REQUIRE(individual_folders.find("/bldsys/cmake") != individual_folders.end());
	REQUIRE(individual_folders.find("/bldsys/cmake/module") != individual_folders.end());
	REQUIRE(individual_folders.find("/bldsys/cmake/template") != individual_folders.end());
	REQUIRE(individual_folders.find("/bldsys/cmake/template/sample") != individual_folders.end());
	REQUIRE(individual_folders.find("/bldsys/scripts") != individual_folders.end());

	REQUIRE(folders.size() == 5);
}

TEST_CASE("Search for files", "[vfs]")
{
	auto &fs = vfs::_default();

	std::vector<std::string> files;

	auto err = fs.enumerate_files("/bldsys/cmake/module", &files);
	CHECK_ERROR(err);

	std::set<std::string> individual_files{files.begin(), files.end()};

	REQUIRE(individual_files.find("/bldsys/cmake/module/FindAdb.cmake") != individual_files.end());
	REQUIRE(individual_files.find("/bldsys/cmake/module/FindGradle.cmake") != individual_files.end());

	REQUIRE(files.size() == 2);
}

TEST_CASE("Search for files recursive", "[vfs]")
{
	auto &fs = vfs::_default();

	std::vector<std::string> files;

	std::string file_extension{".cmake"};

	auto err = fs.enumerate_files_recursive("/bldsys", file_extension, &files);
	CHECK_ERROR(err);

	for (auto &file_path : files)
	{
		REQUIRE(file_path.substr(file_path.size() - file_extension.size()) == file_extension);
	}

	std::set<std::string> individual_files{files.begin(), files.end()};
	REQUIRE(individual_files.find("/bldsys/cmake/check_atomic.cmake") != individual_files.end());

	REQUIRE(files.size() == 8);
}