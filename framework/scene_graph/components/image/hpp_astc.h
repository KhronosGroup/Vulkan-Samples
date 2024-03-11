/* Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include "scene_graph/components/hpp_image.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
class HPPAstc : public vkb::scene_graph::components::HPPImage
{
  public:
	/**
	 * @brief Decodes an ASTC image
	 * @param image Image to decode
	 */
	HPPAstc(const HPPImage &image);

	/**
	 * @brief Decodes ASTC data with an ASTC header
	 * @param name Name of the component
	 * @param data ASTC data with header
	 */
	HPPAstc(const std::string &name, const std::vector<uint8_t> &data);

	virtual ~HPPAstc() = default;

  private:
	/**
	 * @brief Decodes ASTC data
	 * @param blockdim Dimensions of the block
	 * @param extent Extent of the image
	 * @param data Pointer to ASTC image data
	 */
	void decode(std::array<uint8_t, 3> const &blockdim, vk::Extent3D const &extent, const uint8_t *data);

	/**
	 * @brief Initializes ASTC library
	 */
	void init();
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
