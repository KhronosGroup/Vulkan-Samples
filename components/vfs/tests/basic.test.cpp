#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

#include <components/vfs/filesystem.hpp>

using namespace components;

std::string get_ref(const int len)
{
	static const char dict[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::stringstream stream;
	for (int i = 0; i < len; ++i)
	{
		stream << dict[rand() % (sizeof(dict) - 1)];
	}
	return stream.str();
}

TEST_CASE("Create and delete a temporary file", "[vfs]")
{
	auto &fs = vfs::_default();

	std::string          file_path     = "/temp/" + get_ref(10) + ".txt";
	std::string          file_contents = "this is the file contents";
	std::vector<uint8_t> file_data{file_contents.begin(), file_contents.end()};

	REQUIRE(fs.write_file(file_path, file_data.data(), file_data.size()) == nullptr);

	std::shared_ptr<vfs::Blob> blob;
	REQUIRE(fs.read_file(file_path, &blob) == nullptr);

	REQUIRE(blob->ascii() == file_contents);

	// TODO(tomatkinsonarm): delete file?
}