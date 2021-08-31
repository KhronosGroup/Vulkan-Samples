/* Copyright (c) 2018-2019, Arm Limited and Contributors
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
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "common/vk_common.h"
#include "core/buffer.h"
#include "core/shader_module.h"
#include "scene_graph/component.h"

namespace vkb
{
namespace sg
{
class Material;

struct VertexAttribute
{
	VkFormat format = VK_FORMAT_UNDEFINED;

	std::uint32_t stride = 0;

	std::uint32_t offset = 0;
};

class SubMesh : public Component
{
  public:
	~SubMesh() override = default;

	std::type_index get_type() override;

	VkIndexType index_type{};

	std::uint32_t index_offset = 0;

	std::uint32_t vertices_count = 0;

	std::uint32_t vertex_indices = 0;

	std::unordered_map<std::string, core::Buffer> vertex_buffers;

	std::unique_ptr<core::Buffer> index_buffer;

	void set_attribute(const std::string &name, const VertexAttribute &attribute);

	bool get_attribute(const std::string &name, VertexAttribute &attribute) const;

	void set_material(const Material &material);

	const Material *get_material() const;

	const ShaderVariant &get_shader_variant() const;

	ShaderVariant &get_mut_shader_variant();

  private:
	std::unordered_map<std::string, VertexAttribute> vertex_attributes;

	const Material *material{nullptr};

	ShaderVariant shader_variant;

	void compute_shader_variant();
};
}        // namespace sg
}        // namespace vkb
