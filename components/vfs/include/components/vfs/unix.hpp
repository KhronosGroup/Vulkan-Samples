#pragma once

#include "components/vfs/filesystem.hpp"

namespace vfs
{
/**
 * @brief Unix File System Impl
 * 
 */
class UnixFileSystem : public FileSystem
{
  public:
	UnixFileSystem(const std::string &base_path = "");
	virtual ~UnixFileSystem() = default;

	virtual bool           folder_exists(const std::string &file_path) override;
	virtual bool           file_exists(const std::string &file_path) override;
	virtual status::status read_file(const std::string &file_path, std::shared_ptr<Blob> *blob) override;
	virtual status::status read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob) override;
	virtual size_t         file_size(const std::string &file_path) override;
	virtual bool           write_file(const std::string &file_path, const void *data, size_t size) override;
	virtual status::status enumerate_files(const std::string &file_path, std::vector<std::string> *files) override;
	virtual status::status enumerate_folders(const std::string &file_path, std::vector<std::string> *folders) override;

  private:
	std::string m_base_path;
};

/**
 * @brief Unix File System Impl from the default Unix temp directory
 * 
 */
class UnixTempFileSystem final : public UnixFileSystem
{
  public:
	UnixTempFileSystem();
	virtual ~UnixTempFileSystem() = default;
};
}        // namespace vfs