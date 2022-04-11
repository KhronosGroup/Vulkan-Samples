#pragma once

#include <android_native_app_glue.h>

#include "components/vfs/filesystem.hpp"
#include "components/vfs/unix.hpp"

namespace vfs
{
/**
 * @brief Detect the Android Temporary File dir through JNI
 * @param sub_path a subpath into the temporary file dir
 */
class AndroidTempFileSystem final : public UnixFileSystem
{
  public:
	AndroidTempFileSystem(android_app *app, const std::string &sub_path = "");
	virtual ~AndroidTempFileSystem() = default;
};

/**
 * @brief Detect the Android External File dir through JNI
 * @param sub_path a subpath into the external file dir
 */
class AndroidExternalFileSystem final : public UnixFileSystem
{
  public:
	AndroidExternalFileSystem(android_app *app, const std::string &sub_path = "");
	virtual ~AndroidExternalFileSystem() = default;
};

/**
 * @brief Load files from the android AAssetManager
 * @param sub_path a subpath into the AAsset archive
 */
class AndroidAAssetManager : public FileSystem
{
  public:
	AndroidAAssetManager(android_app *app, const std::string &sub_path = "");
	virtual ~AndroidAAssetManager() = default;

	virtual bool           folder_exists(const std::string &file_path) override;
	virtual bool           file_exists(const std::string &file_path) override;
	virtual status::status read_file(const std::string &file_path, std::shared_ptr<Blob> *blob) override;
	virtual status::status read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob) override;
	virtual size_t         file_size(const std::string &file_path) override;
	virtual bool           write_file(const std::string &file_path, const void *data, size_t size) override;
	virtual status::status enumerate_files(const std::string &file_path, std::vector<std::string> *files) override;
	virtual status::status enumerate_folders(const std::string &file_path, std::vector<std::string> *folders) override;

  private:
	std::string    m_base_path;
	AAssetManager *asset_manager;
};
}        // namespace vfs