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

#include "transform.h"

namespace vkb
{
namespace scene_graph
{
namespace components
{
/**
 * @brief facade class around vkb::sg::Transform, providing a vulkan.hpp-based interface
 *
 * See vkb::sg::Transform for documentation
 */
class HPPTransform : private vkb::sg::Transform
{
  public:
	using ComponentType = vkb::sg::Transform;

	using vkb::sg::Transform::get_translation;
	using vkb::sg::Transform::set_matrix;
	using vkb::sg::Transform::set_rotation;
	using vkb::sg::Transform::set_scale;
	using vkb::sg::Transform::set_translation;
};
}        // namespace components
}        // namespace scene_graph
}        // namespace vkb
