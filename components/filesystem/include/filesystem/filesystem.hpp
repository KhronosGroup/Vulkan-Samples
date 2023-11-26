#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/platform/context.hpp"
#include "core/util/logging.hpp"

namespace vkb
{
namespace fs
{
struct FileStat
{
	bool   is_file;
	bool   is_directory;
	size_t size;
};

class FileSystem
{
  public:
	FileSystem()          = default;
	virtual ~FileSystem() = default;

	virtual FileStat             stat_file(const std::string &path)                                    = 0;
	virtual bool                 is_file(const std::string &path)                                      = 0;
	virtual bool                 is_directory(const std::string &path)                                 = 0;
	virtual bool                 exists(const std::string &path)                                       = 0;
	virtual bool                 create_directory(const std::string &path)                             = 0;
	virtual std::vector<uint8_t> read_chunk(const std::string &path, size_t start, size_t offset)      = 0;
	virtual void                 write_file(const std::string &path, const std::vector<uint8_t> &data) = 0;

	void write_file(const std::string &path, const std::string &data);

	// Read the entire file into a string
	std::string read_file(const std::string &path);

	// Read the entire file into a vector of bytes
	template <typename T = uint8_t>
	std::vector<T> read_binary_file(const std::string &path)
	{
		static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
		auto stat   = stat_file(path);
		auto binary = read_chunk(path, 0, stat.size);
		return std::vector<T>(binary.data(), binary.data() + binary.size() / sizeof(T));
	}
};

using FileSystemPtr = std::shared_ptr<FileSystem>;

void init(const PlatformContext &context);

FileSystemPtr get_filesystem();

std::string filename(const std::string &path);
}        // namespace fs
}        // namespace vkb