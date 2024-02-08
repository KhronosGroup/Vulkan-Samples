/* Copyright (c) 2019-2024, Arm Limited and Contributors
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
class ShaderCompiler
{
  private:
	static glslang::EShTargetLanguage        env_target_language;
	static glslang::EShTargetLanguageVersion env_target_language_version;

  public:
	/**
	 * @brief Set the glslang target environment to translate to when generating code
	 * @param target_language The language to translate to
	 * @param target_language_version The version of the language to translate to
	 */
	static void set_target_environment(glslang::EShTargetLanguage        target_language,
	                                   glslang::EShTargetLanguageVersion target_language_version);

	/**
	 * @brief Reset the glslang target environment to the default values
	 */
	static void reset_target_environment();

	/**
	 * @brief Compiles GLSL/HLSL to SPIRV code
	 * @param stage The Vulkan shader stage flag
	 * @param shader_source The source code to be compiled
	 * @param entry_point The entrypoint function name of the shader stage
	 * @param shader_variant The shader variant
	 * @param[out] spirv The generated SPIRV code
	 * @param[out] info_log Stores any log messages during the compilation process
	 * @param src_language The language of source code
	 */
	bool compile_to_spirv(VkShaderStageFlagBits       stage,
	                      const std::vector<uint8_t> &shader_source,
	                      const std::string          &entry_point,
	                      const ShaderVariant        &shader_variant,
	                      std::vector<std::uint32_t> &spirv,
	                      std::string                &info_log,
	                      vkb::ShaderSourceLanguage   src_language = vkb::ShaderSourceLanguage::VK_GLSL);
};
}        // namespace vkb
