#include <catch2/catch_test_macros.hpp>

#include <components/vfs/helpers.hpp>

using namespace components;

TEST_CASE("vfs::helpers::get_file_extension", "[vfs]")
{
	REQUIRE(vfs::helpers::get_file_extension("file.txt") == "txt");
	REQUIRE(vfs::helpers::get_file_extension(".file") == "file");
	REQUIRE(vfs::helpers::get_file_extension("file") == "");
}

TEST_CASE("vfs::helpers::get_directory", "[vfs]")
{
	REQUIRE(vfs::helpers::get_directory("/this/is/a/dir/file.txt") == "/this/is/a/dir");
	REQUIRE(vfs::helpers::get_directory("/this/is/a/di.r/file.txt") == "/this/is/a/di.r");        // . in a path is still valid
	REQUIRE(vfs::helpers::get_directory("/") == "");
}

TEST_CASE("vfs::helpers::get_directory_parts", "[vfs]")
{
	{
		std::vector<std::string> expected = {"/this", "/this/is", "/this/is/a", "/this/is/a/dir"};

		auto parts = vfs::helpers::get_directory_parts("/this/is/a/dir/file.txt");

		REQUIRE(parts.size() == expected.size());

		for (size_t i = 0; i < parts.size(); i++)
		{
			REQUIRE(parts[i] == expected[i]);
		}
	}

	{
		std::vector<std::string> expected{};
		auto                     parts = vfs::helpers::get_directory("/");
		REQUIRE(parts.size() == expected.size());
	}

	{
		std::vector<std::string> expected{};
		auto                     parts = vfs::helpers::get_directory("");
		REQUIRE(parts.size() == expected.size());
	}
}

TEST_CASE("vfs::helpers::tokenize_path", "[vfs]")
{
	{
		std::vector<std::string> expected = {"this", "is", "a", "dir"};

		auto parts = vfs::helpers::tokenize_path("/this/is/a/dir/file.txt");

		REQUIRE(parts.size() == expected.size());

		for (size_t i = 0; i < parts.size(); i++)
		{
			REQUIRE(parts[i] == expected[i]);
		}
	}

	{
		std::vector<std::string> expected{};
		auto                     parts = vfs::helpers::tokenize_path("/");
		REQUIRE(parts.size() == expected.size());
	}

	{
		std::vector<std::string> expected{};
		auto                     parts = vfs::helpers::tokenize_path("");
		REQUIRE(parts.size() == expected.size());
	}
}

TEST_CASE("vfs::helpers::get_file_name", "[vfs]")
{
	REQUIRE(vfs::helpers::get_file_name("/this/is/a/dir/file.txt") == "file.txt");
	REQUIRE(vfs::helpers::get_file_name("/this/is/a/di.r/file.txt") == "file.txt");
	REQUIRE(vfs::helpers::get_file_name("/") == "");
	REQUIRE(vfs::helpers::get_file_name("") == "");
}

TEST_CASE("vfs::helpers::sanitize", "[vfs]")
{
	REQUIRE(vfs::helpers::sanitize("/this/is/a/dir/file.txt") == "/this/is/a/dir/file.txt");
	REQUIRE(vfs::helpers::sanitize("/this/is/a/di.r/file.txt") == "/this/is/a/di.r/file.txt");
	REQUIRE(vfs::helpers::sanitize("/") == "/");
	REQUIRE(vfs::helpers::sanitize("") == "/");
	REQUIRE(vfs::helpers::sanitize("\\crazy_path\\///././/file.txt") == "/crazy_path/file.txt");
	REQUIRE(vfs::helpers::sanitize("C:\\\\windows\\\\path\\\\file.txt") == "C:/windows/path/file.txt");
	REQUIRE(vfs::helpers::sanitize("C:\\\\windows\\\\path\\\\") == "C:/windows/path");
}

TEST_CASE("vfs::helpers::join", "[vfs]")
{
	REQUIRE(vfs::helpers::join({}) == "/");
	REQUIRE(vfs::helpers::join({"this", "is", "a", "path"}) == "/this/is/a/path");
	REQUIRE(vfs::helpers::join({"this", "is", "a", "crazy\\/././\\", "path"}) == "/this/is/a/crazy/path");
}