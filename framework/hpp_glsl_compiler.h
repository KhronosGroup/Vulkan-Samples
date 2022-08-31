/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include "glsl_compiler.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
/**
 * @brief facade class around vkb::GLTFLoader, providing a vulkan.hpp-based interface
 *
 * See vkb::GLTFLoader for documentation
 */
class HPPGLSLCompiler : private vkb::GLSLCompiler
{
  public:
	inline bool compile_to_spirv(vk::ShaderStageFlagBits     stage,
	                             const std::vector<uint8_t> &glsl_source,
	                             const std::string &         entry_point,
	                             const ShaderVariant &       shader_variant,
	                             std::vector<std::uint32_t> &spirv,
	                             std::string &               info_log)
	{
		return vkb::GLSLCompiler::compile_to_spirv(static_cast<VkShaderStageFlagBits>(stage), glsl_source, entry_point, shader_variant, spirv, info_log);
	}
};
}        // namespace vkb
