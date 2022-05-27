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
namespace detail
{
bool is_astc(const VkFormat format);
}

class AstcCodec final : public ImageCodec
{
  public:
	AstcCodec()          = default;
	virtual ~AstcCodec() = default;

	virtual StackErrorPtr encode(const Image &image, const std::vector<VkFormat> &format_preference, ImagePtr *o_image) const override;
	virtual StackErrorPtr decode(const Image &image, const std::vector<VkFormat> &format_preference, ImagePtr *o_image) const override;
};
}        // namespace images
}        // namespace components
