/* Copyright (c) 2019-2021, Arm Limited and Contributors
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
#include <vector>

#include <volk.h>

#include "components/shaders/shader_resource.hpp"

namespace components
{
namespace shaders
{
class ShaderStage
{
  public:
	virtual VkShaderStageFlagBits stage() const     = 0;
	virtual ShaderResources       resources() const = 0;
	virtual size_t                hash() const      = 0;
};

using ShaderStagePtr = std::unique_ptr<ShaderStage>;

class Shader
{
  public:
	Shader(const std::vector<ShaderStagePtr> &stages);

	~Shader();

	const std::vector<ShaderStagePtr> &stages() const
	{
		return _stages;
	}

	ShaderResources resources(VkShaderStageFlagBits stage) const
	{
		for (auto &s : _stages)
		{
			if (s->stage() == stage)
			{
				return s->resources();
			}
		}

		return {};
	}

	size_t hash() const
	{
		return _hash;
	}

  private:
	std::string                 _name;
	std::vector<ShaderStagePtr> _stages;
	size_t                      _hash;
};

}        // namespace vulkan
}        // namespace components