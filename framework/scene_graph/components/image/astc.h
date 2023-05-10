/* Copyright (c) 2019-2023, Arm Limited and Contributors
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

#include "astcenc.h"
#include "common/vk_common.h"
#include "scene_graph/components/image.h"

namespace vkb
{
namespace sg
{
struct BlockDim
{
	uint8_t x;
	uint8_t y;
	uint8_t z;
};

class Astc : public Image
{
  public:
	/**
	 * @brief Decodes an ASTC image
	 * @param image Image to decode
	 */
	Astc(const Image &image);

	/**
	 * @brief Decodes ASTC data with an ASTC header
	 * @param name Name of the component
	 * @param data ASTC data with header
	 */
	Astc(const std::string &name, const std::vector<uint8_t> &data);

	virtual ~Astc() = default;

  private:
	/**
	 * @brief Decodes ASTC data
	 * @param blockdim Dimensions of the block
	 * @param extent Extent of the image
	 * @param data Pointer to ASTC image data
	 */
	void decode(BlockDim blockdim, VkExtent3D extent, const uint8_t *data);

	/**
	 * @brief Initializes ASTC library
	 */
	void init();

	astcenc_config config;
	astcenc_error  status;
};
}        // namespace sg
}        // namespace vkb
