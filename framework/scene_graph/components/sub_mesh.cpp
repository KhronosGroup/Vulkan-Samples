/* Copyright (c) 2018-2025, Arm Limited and Contributors
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

#include "sub_mesh.h"

#include "material.h"
#include "rendering/subpass.h"

namespace vkb
{
namespace sg
{
SubMesh::SubMesh(const std::string &name) :
    Component{name}
{}

std::type_index SubMesh::get_type()
{
	return typeid(SubMesh);
}

void SubMesh::set_attribute(const std::string &attribute_name, const VertexAttribute &attribute)
{
	vertex_attributes[attribute_name] = attribute;
}

bool SubMesh::get_attribute(const std::string &attribute_name, VertexAttribute &attribute) const
{
	auto attrib_it = vertex_attributes.find(attribute_name);

	if (attrib_it == vertex_attributes.end())
	{
		return false;
	}

	attribute = attrib_it->second;

	return true;
}

void SubMesh::set_material(const Material &new_material)
{
	material = &new_material;
}

const Material *SubMesh::get_material() const
{
	return material;
}

const ShaderVariant &SubMesh::get_shader_variant() const
{
	return shader_variant;
}

ShaderVariant &SubMesh::get_mut_shader_variant()
{
	return shader_variant;
}
}        // namespace sg
}        // namespace vkb
