/* Copyright (c) 2022, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "components/vfs/android.hpp"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <components/common/stack_error.hpp>

namespace components
{
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

StackErrorPtr AndroidAAssetManager::read_file(const std::string &file_path, std::shared_ptr<Blob> *blob)
{
	if (!asset_manager)
	{
		return StackError::unique("AAsset Manager not initialized", "vfs/android_aasset_manager.cpp", __LINE__);
	}

	std::string real_path = get_path(file_path);

	AAsset *asset = AAssetManager_open(asset_manager, real_path.c_str(), AASSET_MODE_STREAMING);

	if (!asset)
	{
		return StackError::unique("failed to find file: " + file_path, "vfs/android_aasset_manager.cpp", __LINE__);
	}

	size_t size = AAsset_getLength(asset);

	auto a_blob = std::make_shared<StdBlob>();
	a_blob->buffer.resize(size, 0);

	AAsset_read(asset, const_cast<void *>(reinterpret_cast<const void *>(a_blob->buffer.data())), size);
	AAsset_close(asset);

	*blob = a_blob;

	return nullptr;
}

StackErrorPtr AndroidAAssetManager::read_chunk(const std::string &file_path, const size_t offset, const size_t count, std::shared_ptr<Blob> *blob)
{
	if (!asset_manager)
	{
		return StackError::unique("AAsset Manager not initialized", "vfs/android_aasset_manager.cpp", __LINE__);
	}

	std::string real_path = get_path(file_path);

	AAsset *asset = AAssetManager_open(asset_manager, real_path.c_str(), AASSET_MODE_STREAMING);

	if (!asset)
	{
		return StackError::unique("failed to find file: " + file_path, "vfs/android_aasset_manager.cpp", __LINE__);
	}

	size_t size = AAsset_getLength(asset);

	if (offset + count > size)
	{
		return StackError::unique("requested chunk out of range", "vfs/android_aasset_manager.cpp", __LINE__);
	}

	auto a_blob = std::make_shared<StdBlob>();
	a_blob->buffer.resize(size, 0);

	AAsset_seek(asset, offset, 0);
	AAsset_read(asset, const_cast<void *>(reinterpret_cast<const void *>(a_blob->buffer.data())), size);

	AAsset_close(asset);

	*blob = a_blob;

	return nullptr;
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

StackErrorPtr AndroidAAssetManager::write_file(const std::string &file_path, const void *data, size_t size)
{
	return StackError::unique("not implemented", "vfs/android_aasset_manager.cpp", __LINE__);
}

StackErrorPtr AndroidAAssetManager::enumerate_files(const std::string &file_path, std::vector<std::string> *files)
{
	return StackError::unique("not implemented", "vfs/android_aasset_manager.cpp", __LINE__);
}

StackErrorPtr AndroidAAssetManager::enumerate_folders(const std::string &file_path, std::vector<std::string> *folders)
{
	return StackError::unique("not implemented", "vfs/android_aasset_manager.cpp", __LINE__);
}

void AndroidAAssetManager::make_directory(const std::string &path)
{
}

}        // namespace vfs
}        // namespace components