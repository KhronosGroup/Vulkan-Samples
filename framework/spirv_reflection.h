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

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "common/error.h"

#include <spirv_glsl.hpp>

#include "common/vk_common.h"
#include "core/shader_module.h"

namespace vkb
{
/// Generate a list of shader resource based on SPIRV reflection code, and provided ShaderVariant
class SPIRVReflection
{
  public:
	/// @brief Reflects shader resources from SPIRV code
	/// @param stage The Vulkan shader stage flag
	/// @param spirv The SPIRV code of shader
	/// @param[out] resources The list of reflected shader resources
	/// @param variant ShaderVariant used for reflection to specify the size of the runtime arrays in Storage Buffers
	bool reflect_shader_resources(VkShaderStageFlagBits        stage,
	                              const std::vector<uint32_t> &spirv,
	                              std::vector<ShaderResource> &resources,
	                              const ShaderVariant         &variant);

  private:
	void parse_shader_resources(const spirv_cross::Compiler &compiler,
	                            VkShaderStageFlagBits        stage,
	                            std::vector<ShaderResource> &resources,
	                            const ShaderVariant         &variant);

	void parse_push_constants(const spirv_cross::Compiler &compiler,
	                          VkShaderStageFlagBits        stage,
	                          std::vector<ShaderResource> &resources,
	                          const ShaderVariant         &variant);

	void parse_specialization_constants(const spirv_cross::Compiler &compiler,
	                                    VkShaderStageFlagBits        stage,
	                                    std::vector<ShaderResource> &resources,
	                                    const ShaderVariant         &variant);
};
}        // namespace vkb
