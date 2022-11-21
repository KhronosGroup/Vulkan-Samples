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

#ifdef ANDROID
#	include <components/vfs/android.hpp>

namespace components
{
namespace vfs
{
RootFileSystem &_default(const PlatformContext *context)
{
	static vfs::RootFileSystem fs;

	static bool first_time = true;
	if (first_time)
	{
		auto *app = reinterpret_cast<android_app *>(context);

		fs.mount("/temp/", std::make_shared<vfs::AndroidTempFileSystem>(app));

#	ifdef VKB_BUNDLE_ASSETS
		fs.mount("/", std::make_shared<vfs::AndroidAAssetManager>(app));
		fs.mount("/scenes/", std::make_shared<vfs::AndroidAAssetManager>(app, "scenes"));
		fs.mount("/textures/", std::make_shared<vfs::AndroidAAssetManager>(app, "textures"));
		fs.mount("/fonts/", std::make_shared<vfs::AndroidAAssetManager>(app, "fonts"));
		fs.mount("/shaders/", std::make_shared<vfs::AndroidAAssetManager>(app, ""));
#	else
		fs.mount("/", std::make_shared<vfs::AndroidExternalFileSystem>(app));
		fs.mount("/scenes/", std::make_shared<vfs::AndroidExternalFileSystem>(app, "/assets/scenes"));
		fs.mount("/textures/", std::make_shared<vfs::AndroidExternalFileSystem>(app, "/assets/textures"));
		fs.mount("/fonts/", std::make_shared<vfs::AndroidExternalFileSystem>(app, "/assets/fonts"));
		fs.mount("/shaders/", std::make_shared<vfs::AndroidExternalFileSystem>(app, "/shaders"));
#	endif

		first_time = false;
	}

	return fs;
}
}        // namespace vfs
}        // namespace components

#else

#	include <components/vfs/std_filesystem.hpp>

namespace components
{
namespace vfs
{
RootFileSystem &_default(const PlatformContext * /* context */)
{
	static vfs::RootFileSystem fs;

	static bool first_time = true;
	if (first_time)
	{
		first_time = false;

		auto cwd = std::filesystem::current_path();
		fs.mount("/", std::make_shared<vfs::StdFSFileSystem>(cwd));
		fs.mount("/scenes/", std::make_shared<vfs::StdFSFileSystem>(cwd / "assets/scenes"));
		fs.mount("/textures/", std::make_shared<vfs::StdFSFileSystem>(cwd / "assets/textures"));
		fs.mount("/fonts/", std::make_shared<vfs::StdFSFileSystem>(cwd / "assets/fonts"));
		fs.mount("/temp/", std::make_shared<vfs::StdFSTempFileSystem>());
	}

	return fs;
}
}        // namespace vfs
}        // namespace components

#endif
