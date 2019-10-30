/* Copyright (c) 2019, Sascha Willems
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

namespace vkb
{
/**
 * @brief Class representing a heightmap loaded from a ktx texture
 */
class HeightMap
{
  public:
	/**
	 * @brief Loads in a ktx texture as a heightmap
	 * @param filename The ktx file to load
	 * @param patchsize The patch size
	 */
	HeightMap(const std::string &filename, const uint32_t patchsize);

	~HeightMap();

	/**
	 * @brief Retrieves a value from the heightmap at a specific coordinates
	 * @param x The x coordinate
	 * @param y The y coordinate
	 * @returns A float height value
	 */
	float get_height(const uint32_t x, const uint32_t y);

  private:
	uint16_t *data;

	uint32_t dim;

	uint32_t scale;
};
}        // namespace vkb
