#include <catch2/catch_test_macros.hpp>

#include <set>
#include <sstream>
#include <string>

#include <components/vfs/filesystem.hpp>
#include <components/vfs/helpers.hpp>

using namespace components;

#define CATCH_ERROR()            \
	catch (std::exception & err) \
	{                            \
		INFO(err.what());        \
		REQUIRE(false);          \
	}

TEST_CASE("Create and delete a temporary file", "[vfs]")
{
	auto &fs = vfs::_default();

	std::string          file_path     = "/temp/vkb_basic_test_file.txt";
	std::string          file_contents = "this is the file contents";
	std::vector<uint8_t> file_data{file_contents.begin(), file_contents.end()};

	try
	{
		fs.write_file(file_path, file_data.data(), file_data.size());

		std::vector<uint8_t> blob = fs.read_file(file_path);

		REQUIRE(file_contents == std::string{blob.begin(), blob.end()});

		REQUIRE(fs.remove(file_path));
	}
	CATCH_ERROR()
}

TEST_CASE("Search for folders", "[vfs]")
{
	auto &fs = vfs::_default();

	try
	{
		std::vector<std::string> folders = fs.enumerate_folders("/bldsys");

		std::set<std::string> individual_folders{folders.begin(), folders.end()};

		REQUIRE(individual_folders.find("/bldsys/cmake") != individual_folders.end());
		REQUIRE(individual_folders.find("/bldsys/scripts") != individual_folders.end());

		REQUIRE(folders.size() == 2);
	}
	CATCH_ERROR()
}

TEST_CASE("Search for folders recursive", "[vfs]")
{
	auto &fs = vfs::_default();

	try
	{
		std::vector<std::string> folders = fs.enumerate_folders_recursive("/bldsys");

		std::set<std::string> individual_folders{folders.begin(), folders.end()};

		REQUIRE(individual_folders.find("/bldsys/cmake") != individual_folders.end());
		REQUIRE(individual_folders.find("/bldsys/cmake/module") != individual_folders.end());
		REQUIRE(individual_folders.find("/bldsys/cmake/template") != individual_folders.end());
		REQUIRE(individual_folders.find("/bldsys/cmake/template/sample") != individual_folders.end());
		REQUIRE(individual_folders.find("/bldsys/scripts") != individual_folders.end());

		REQUIRE(folders.size() == 5);
	}
	CATCH_ERROR()
}

TEST_CASE("Search for files", "[vfs]")
{
	auto &fs = vfs::_default();

	try
	{
		std::vector<std::string> files = fs.enumerate_files("/bldsys/cmake/module");

		std::set<std::string> individual_files{files.begin(), files.end()};

		REQUIRE(individual_files.find("/bldsys/cmake/module/FindAdb.cmake") != individual_files.end());
		REQUIRE(individual_files.find("/bldsys/cmake/module/FindGradle.cmake") != individual_files.end());

		REQUIRE(files.size() == 2);
	}
	CATCH_ERROR()
}

TEST_CASE("Search for files recursive", "[vfs]")
{
	auto &fs = vfs::_default();

	std::string file_extension{"cmake"};

	try
	{
		std::vector<std::string> files = fs.enumerate_files_recursive("/bldsys", file_extension);

		for (auto const &file_path : files)
		{
			REQUIRE(vfs::helpers::get_file_extension(file_path) == file_extension);
		}

		std::set<std::string> individual_files{files.begin(), files.end()};
		REQUIRE(individual_files.find("/bldsys/cmake/check_atomic.cmake") != individual_files.end());

		REQUIRE(files.size() == 8);
	}
	CATCH_ERROR()
}