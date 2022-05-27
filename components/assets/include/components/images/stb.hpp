/* Copyright (c) 2019, Arm Limited and Contributors
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

#include <string>
#include <vector>

#include "components/images/image_codec.hpp"

namespace components
{
namespace images
{
class StbLoader final : public ImageLoader
{
  public:
	StbLoader()          = default;
	virtual ~StbLoader() = default;

	virtual void load_from_memory(const std::string &name, const std::vector<uint8_t> &data, assets::ImageAssetPtr *o_image) const override;
};

class StbWriter final : public ImageWriter
{
  public:
	enum class Target
	{
		PNG,
		BMP,
		TGA,
		JPG
	};

	StbWriter(Target target) :
	    m_target{target}
	{}

	virtual ~StbWriter() = default;

	virtual void write_to_file(vfs::FileSystem &fs, const std::string &path, const assets::ImageAsset &image) const override;

  private:
	Target m_target;
};

}        // namespace images
}        // namespace components
