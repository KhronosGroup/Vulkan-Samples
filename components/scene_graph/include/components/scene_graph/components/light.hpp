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

#include <glm/glm.hpp>

namespace components
{
namespace sg
{
struct PointLight
{
	glm::vec3 color{1.0f, 1.0f, 1.0f};
	float     intensity{1.0f};
	float     range{0.0f};
};

struct DirectionalLight
{
	glm::vec3 direction{0.0f, 0.0f, -1.0f};
	glm::vec3 color{1.0f, 1.0f, 1.0f};
	float     intensity{1.0f};
};

struct SpotLight
{
	glm::vec3 direction{0.0f, 0.0f, -1.0f};
	glm::vec3 color{1.0f, 1.0f, 1.0f};
	float     intensity{1.0f};
	float     range{0.0f};
	float     inner_cone_angle{0.0f};
	float     outer_cone_angle{0.0f};
};
}        // namespace sg
}        // namespace components