/* Copyright (c) 2019-2023, Sascha Willems
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

#include "heightmap.h"

#include <cstring>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include "common/glm_common.h"
VKBP_ENABLE_WARNINGS()

#include "platform/filesystem.h"

#if defined(__ANDROID__)
#include "platform/android/asset_manager.h"
#endif

namespace vkb
{
HeightMap::HeightMap(const std::string &file_name, const uint32_t patchsize)
{
	std::string file_path = fs::path::get(fs::path::Assets, file_name);

	ktxTexture *ktx_texture;
	ktxResult   ktx_result;
	ktx_result = vkb::fs::read_ktx_file(file_path, &ktx_texture);
	assert(ktx_result == KTX_SUCCESS);

	ktx_size_t   ktx_size  = ktxTexture_GetImageSize(ktx_texture, 0);
	ktx_uint8_t *ktx_image = ktxTexture_GetData(ktx_texture);

	dim  = ktx_texture->baseWidth;
	data = new uint16_t[dim * dim];

	memcpy(data, ktx_image, ktx_size);

	this->scale = dim / patchsize;

	ktxTexture_Destroy(ktx_texture);
}

HeightMap::~HeightMap()
{
	delete[] data;
}

float HeightMap::get_height(const uint32_t x, const uint32_t y)
{
	glm::ivec2 rpos = glm::ivec2(x, y) * glm::ivec2(scale);
	rpos.x          = std::max(0, std::min(rpos.x, static_cast<int>(dim) - 1));
	rpos.y          = std::max(0, std::min(rpos.y, static_cast<int>(dim) - 1));
	rpos /= glm::ivec2(scale);
	return *(data + (rpos.x + rpos.y * dim) * scale) / 65535.0f;
}
}        // namespace vkb
