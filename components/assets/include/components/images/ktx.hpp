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

#include "components/images/image.hpp"

namespace components
{
namespace images
{
class KtxLoader final : public ImageLoader
{
  public:
	KtxLoader()          = default;
	virtual ~KtxLoader() = default;

	virtual StackErrorPtr load_from_file(const std::string &name, vfs::FileSystem &fs, const std::string &path, ImagePtr *o_image) const;
	virtual StackErrorPtr load_from_memory(const std::string &name, const std::vector<uint8_t> &data, ImagePtr *o_image) const;
};
}        // namespace images
}        // namespace components