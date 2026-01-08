/* Copyright (c) 2024-2026, Holochip Inc.
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

#include "gaussian_splat.h"

namespace vkb
{
namespace sg
{

GaussianSplat::GaussianSplat(const std::string &name) :
    Component{name}
{
}

std::type_index GaussianSplat::get_type()
{
	return typeid(GaussianSplat);
}

size_t GaussianSplat::get_gpu_memory_size() const
{
	size_t total = 0;

	if (position_buffer)
	{
		total += position_buffer->get_size();
	}
	if (rotation_buffer)
	{
		total += rotation_buffer->get_size();
	}
	if (scale_buffer)
	{
		total += scale_buffer->get_size();
	}
	if (opacity_buffer)
	{
		total += opacity_buffer->get_size();
	}
	if (color_buffer)
	{
		total += color_buffer->get_size();
	}
	if (sh_buffer)
	{
		total += sh_buffer->get_size();
	}

	return total;
}

}        // namespace sg
}        // namespace vkb
