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

#if defined(_WIN32) || defined(_WIN64)
// Windows.h defines IGNORE, so we must #undef it to avoid clashes with astc header
#	undef IGNORE
#endif
#include <astcenc.h>

#include "components/images/image.hpp"

namespace components
{
namespace images
{
namespace detail
{
bool is_astc(const VkFormat format);
}

enum class AstcSearchPreset
{
	ASTCENC_PRE_FASTEST,
	ASTCENC_PRE_FAST,
	ASTCENC_PRE_MEDIUM,
	ASTCENC_PRE_THOROUGH,
	ASTCENC_PRE_EXHAUSTIVE,
};

struct AstcConfig final
{
};

class AstcCodec final : public ImageCodec
{
  public:
	AstcCodec()          = default;
	virtual ~AstcCodec() = default;

	virtual StackErrorPtr encode(const Image &image, ImagePtr *o_image) const override;
	virtual StackErrorPtr decode(const Image &image, ImagePtr *o_image) const override;
};
}        // namespace images
}        // namespace components
