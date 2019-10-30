/* Copyright (c) 2019, Arm Limited and Contributors
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
#include <vector>

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <glslang/Public/ShaderLang.h>
VKBP_ENABLE_WARNINGS()

#include "common/vk_common.h"
#include "core/shader_module.h"

namespace vkb
{
/// Helper class to generate SPIRV code from GLSL source
/// A very simple version of the glslValidator application
class GLSLCompiler
{
  public:
	/**
	 * @brief Compiles GLSL to SPIRV code
	 * @param stage The Vulkan shader stage flag
	 * @param glsl_source The GLSL source code to be compiled
	 * @param entry_point The entrypoint function name of the shader stage
	 * @param shader_variant The shader variant
	 * @param[out] spirv The generated SPIRV code
	 * @param[out] info_log Stores any log messages during the compilation process
	 */
	bool compile_to_spirv(VkShaderStageFlagBits       stage,
	                      const std::vector<uint8_t> &glsl_source,
	                      const std::string &         entry_point,
	                      const ShaderVariant &       shader_variant,
	                      std::vector<std::uint32_t> &spirv,
	                      std::string &               info_log);
};
}        // namespace vkb
