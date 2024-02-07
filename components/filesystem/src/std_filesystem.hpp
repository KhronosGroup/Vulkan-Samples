#include "filesystem/filesystem.hpp"

#include <filesystem>

namespace vkb
{
namespace filesystem
{
class StdFileSystem final : public FileSystem
{
  public:
	StdFileSystem(std::string external_storage_directory = std::filesystem::current_path().string(), std::string temp_directory = std::filesystem::temp_directory_path().string()) :
	    _external_storage_directory(std::move(external_storage_directory)),
	    _temp_directory(std::move(temp_directory))
	{}

	virtual ~StdFileSystem() = default;

	FileStat stat_file(const std::string &path) override;

	bool is_file(const std::string &path) override;

	bool is_directory(const std::string &path) override;

	bool exists(const std::string &path) override;

	bool create_directory(const std::string &path) override;

	std::vector<uint8_t> read_chunk(const std::string &path, size_t start, size_t offset) override;

	void write_file(const std::string &path, const std::vector<uint8_t> &data) override;

	const std::string &external_storage_directory() const override;

	const std::string &temp_directory() const override;

  private:
	std::string _external_storage_directory;
	std::string _temp_directory;
};
}        // namespace filesystem
}        // namespace vkb