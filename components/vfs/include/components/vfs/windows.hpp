#pragma once

#include "components/vfs/filesystem.hpp"

namespace vfs
{
/**
 * @brief Windows File System Impl
 * 
 */
class WindowsFileSystem : public FileSystem
{
  public:
	WindowsFileSystem(const std::string &base_path = "");
	virtual ~WindowsFileSystem() = default;

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
 * @brief Windows File System Impl from the default Windows temp directory
 * 
 */
class WindowsTempFileSystem final : public WindowsFileSystem
{
  public:
	WindowsTempFileSystem();
	virtual ~WindowsTempFileSystem() = default;
};
}        // namespace vfs