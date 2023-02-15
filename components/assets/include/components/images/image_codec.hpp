/* Copyright (c) 2022, Thomas Atkinson
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

#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

#include <components/assets/image.hpp>
#include <components/common/error.hpp>
#include <components/vfs/filesystem.hpp>

namespace components
{
namespace images
{
class ImageLoader
{
  public:
	ImageLoader()          = default;
	virtual ~ImageLoader() = default;

	inline void load_from_file(const std::string &name, vfs::FileSystem &fs, const std::string &path, assets::ImageAssetPtr *o_image) const
	{
		load_from_memory(name, fs.read_file(path), o_image);
	}

	virtual void load_from_memory(const std::string &name, const std::vector<uint8_t> &data, assets::ImageAssetPtr *o_image) const = 0;
};

class ImageWriter
{
  public:
	ImageWriter()          = default;
	virtual ~ImageWriter() = default;

	virtual void write_to_file(vfs::FileSystem &fs, const std::string &path, const assets::ImageAsset &image) const = 0;
};

class ImageEncoder
{
  public:
	ImageEncoder()          = default;
	virtual ~ImageEncoder() = default;

	virtual void encode(const assets::ImageAsset &image, assets::ImageAssetPtr *o_image) const = 0;
};

class ImageDecoder
{
  public:
	ImageDecoder()          = default;
	virtual ~ImageDecoder() = default;

	virtual void decode(const assets::ImageAsset &image, assets::ImageAssetPtr *o_image) const = 0;
};

class ImageCodec : public ImageEncoder, ImageDecoder
{
  public:
	ImageCodec()          = default;
	virtual ~ImageCodec() = default;
};
}        // namespace images
}        // namespace components