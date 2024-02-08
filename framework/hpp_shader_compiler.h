/* Copyright (c) 2022-2024, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2024, Mobica Limited
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

#include "shader_compiler.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
/**
 * @brief facade class around vkb::ShaderCompiler, providing a vulkan.hpp-based interface
 *
 * See vkb::ShaderCompiler for documentation
 */
class HPPShaderCompiler : private vkb::ShaderCompiler
{
  public:
	inline bool compile_to_spirv(vk::ShaderStageFlagBits     stage,
	                             const std::vector<uint8_t> &shader_source,
	                             const std::string          &entry_point,
	                             const ShaderVariant        &shader_variant,
	                             std::vector<std::uint32_t> &spirv,
	                             std::string                &info_log,
	                             vkb::ShaderSourceLanguage   src_language = vkb::ShaderSourceLanguage::VK_GLSL)
	{
		return vkb::ShaderCompiler::compile_to_spirv(static_cast<VkShaderStageFlagBits>(stage), shader_source, entry_point, shader_variant, spirv, info_log, src_language);
	}
};
}        // namespace vkb
