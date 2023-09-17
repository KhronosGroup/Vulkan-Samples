#include "fs/filesystem.hpp"

namespace vkb
{
namespace fs
{
class StdFileSystem final : public FileSystem
{
  public:
	StdFileSystem()          = default;
	virtual ~StdFileSystem() = default;

	FileStat stat_file(const std::string &path) override;

	bool is_file(const std::string &path) override;

	bool is_directory(const std::string &path) override;

	bool exists(const std::string &path) override;

	bool create_directory(const std::string &path) override;

	std::vector<uint8_t> read_chunk(const std::string &path, size_t start, size_t offset) override;

	virtual void write_file(const std::string &path, const std::vector<uint8_t> &data) override;
};
}        // namespace fs
}        // namespace vkb