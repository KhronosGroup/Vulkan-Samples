#include "components/vfs/android.hpp"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

namespace vfs
{
jobject global_asset_manager;

AAssetManager *get_aasset_manager(android_app *app)
{
	if (!app)
	{
		return nullptr;
	}

	return app->activity->assetManager;
}

AndroidAAssetManager::AndroidAAssetManager(android_app *app, const std::string &base_path) :
    FileSystem{}, m_base_path{base_path}, asset_manager{get_aasset_manager(app)}
{
}

std::string AndroidAAssetManager::get_path(const std::string &path)
{
	std::string real_path = m_base_path + path;

	if (real_path.empty())
	{
		return real_path;
	}

	// remove / prefix
	if (real_path[0] == '/')
	{
		real_path = real_path.substr(1, real_path.size());
	}

	return real_path;
}

bool AndroidAAssetManager::folder_exists(const std::string &file_path)
{
	if (!asset_manager)
	{
		return false;
	}

	std::string real_path = get_path(file_path);

	AAssetDir *dir = AAssetManager_openDir(asset_manager, real_path.c_str());

	if (!dir)
	{
		return false;
	}

	AAssetDir_close(dir);

	return true;
}

bool AndroidAAssetManager::file_exists(const std::string &file_path)
{
	if (!asset_manager)
	{
		return false;
	}

	std::string real_path = get_path(file_path);

	AAsset *asset = AAssetManager_open(asset_manager, real_path.c_str(), AASSET_MODE_STREAMING);

	if (!asset)
	{
		return false;
	}

	AAsset_close(asset);

	return true;
}

status::status AndroidAAssetManager::read_file(const std::string &file_path, std::shared_ptr<Blob> *blob)
{
	if (!asset_manager)
	{
		return status::FailedToRead;
	}

	std::string real_path = get_path(file_path);

	AAsset *asset = AAssetManager_open(asset_manager, real_path.c_str(), AASSET_MODE_STREAMING);

	if (!asset)
	{
		return status::FileNotFound;
	}

	size_t size = AAsset_getLength(asset);

	auto a_blob = std::make_shared<StdBlob>();
	a_blob->buffer.resize(size, 0);

	AAsset_read(asset, const_cast<void *>(reinterpret_cast<const void *>(a_blob->buffer.data())), size);
	AAsset_close(asset);

	*blob = a_blob;

	return status::Success;
}

status::status AndroidAAssetManager::read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob)
{
	if (!asset_manager)
	{
		return status::FailedToRead;
	}

	std::string real_path = get_path(file_path);

	AAsset *asset = AAssetManager_open(asset_manager, real_path.c_str(), AASSET_MODE_STREAMING);

	if (!asset)
	{
		return status::FileNotFound;
	}

	size_t size = AAsset_getLength(asset);

	if (offset + count > size)
	{
		return status::MemoryOutOfRange;
	}

	auto a_blob = std::make_shared<StdBlob>();
	a_blob->buffer.resize(size, 0);

	AAsset_seek(asset, offset, 0);
	AAsset_read(asset, const_cast<void *>(reinterpret_cast<const void *>(a_blob->buffer.data())), size);

	AAsset_close(asset);

	*blob = a_blob;

	return status::Success;
}

size_t AndroidAAssetManager::file_size(const std::string &file_path)
{
	if (!asset_manager)
	{
		return 0;
	}

	std::string real_path = get_path(file_path);

	AAsset *asset = AAssetManager_open(asset_manager, real_path.c_str(), AASSET_MODE_STREAMING);

	if (!asset)
	{
		return status::FileNotFound;
	}

	size_t size = AAsset_getLength(asset);

	AAsset_close(asset);

	return size;
}

bool AndroidAAssetManager::write_file(const std::string &file_path, const void *data, size_t size)
{
	return false;
}

status::status AndroidAAssetManager::enumerate_files(const std::string &file_path, std::vector<std::string> *files)
{
	return status::NotImplemented;
}

status::status AndroidAAssetManager::enumerate_folders(const std::string &file_path, std::vector<std::string> *folders)
{
	return status::NotImplemented;
}
}        // namespace vfs