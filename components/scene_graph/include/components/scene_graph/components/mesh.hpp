/* Copyright (c) 2022, Arm Limited and Contributors
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
#include <optional>
#include <unordered_map>
#include <vector>

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <glm/glm.hpp>
#include <volk.h>
VKBP_ENABLE_WARNINGS()

#include <components/assets/image.hpp>

namespace components
{
namespace sg
{
using Buffer = std::shared_ptr<std::vector<uint8_t>>;

inline Buffer make_buffer(std::vector<uint8_t> &&data)
{
	return std::make_shared<std::vector<uint8_t>>(std::move(data));
}

enum class AttributeType
{
	Position,
	Normal,
	Tangent,
	TexCoord_0,
	TexCoord_1,
	Color_0,
	Joints_0,
	Weights_0,
	Unknown
};

struct VertexAttribute
{
	VkFormat format = VK_FORMAT_UNDEFINED;
	uint32_t stride = 0;
	uint32_t offset = 0;
	uint32_t count  = 0;

	Buffer buffer;
};

struct Sampler
{
	VkFilter             min_filter;
	VkFilter             mag_filter;
	VkSamplerAddressMode address_mode_u;
	VkSamplerAddressMode address_mode_v;
	VkSamplerMipmapMode  mipmap_mode{VK_SAMPLER_MIPMAP_MODE_NEAREST};
};

struct Texture
{
	AttributeType         texCoord;
	assets::ImageAssetPtr image;
	Sampler               sampler;

	inline AttributeType get_valid_tex_coord_target() const
	{
		assert(texCoord != AttributeType::TexCoord_0 || texCoord != AttributeType::TexCoord_1 && "invalid texture coordinate attribute");
		return texCoord;
	}
};

// How the alpha value of the main factor and texture should be interpreted
enum class AlphaMode
{
	Opaque,        // < Alpha value is ignored
	Mask,          // < Either full opaque or fully transparent
	Blend          // < Output is combined with the background
};

enum class TextureType
{
	BaseColor,
	Normal,
	Occlusion,
	Emissive,
	MetallicRoughness,
	Unknown
};

struct Material
{
	std::unordered_map<TextureType, Texture> textures;

	bool      double_sided{false};
	AlphaMode alpha_mode{AlphaMode::Opaque};
	float     alpha_cutoff{0.5f};
	glm::vec4 base_color_factor{0.0f, 0.0f, 0.0f, 0.0f};
	glm::vec3 emissive_factor{0.0f, 0.0f, 0.0f};
	float     metallic_factor{0.0f};
	float     roughness_factor{0.0f};
};

struct Mesh
{
	std::optional<VertexAttribute> indices;

	VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_MAX_ENUM};

	std::unordered_map<AttributeType, VertexAttribute> vertex_attributes;

	Material material;
};
}        // namespace sg
}        // namespace components