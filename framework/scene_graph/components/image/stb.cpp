/* Copyright (c) 2019-2024, Arm Limited and Contributors
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

#include "scene_graph/components/image/stb.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PSD
#include <stb_image.h>

namespace vkb
{
namespace sg
{
Stb::Stb(const std::string &name, const std::vector<uint8_t> &data, ContentType content_type) :
    Image{name}
{
	int width;
	int height;
	int comp;
	int req_comp = 4;

	auto data_buffer = reinterpret_cast<const stbi_uc *>(data.data());
	auto data_size   = static_cast<int>(data.size());

	auto raw_data = stbi_load_from_memory(data_buffer, data_size, &width, &height, &comp, req_comp);

	if (!raw_data)
	{
		throw std::runtime_error{"Failed to load " + name + ": " + stbi_failure_reason()};
	}

	set_data(raw_data, width * height * req_comp);
	stbi_image_free(raw_data);

	set_format(content_type == Color ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
	set_width(to_u32(width));
	set_height(to_u32(height));
	set_depth(1u);
}

}        // namespace sg
}        // namespace vkb
