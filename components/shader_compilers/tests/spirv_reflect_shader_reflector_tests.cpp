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

#include <components/common/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <catch2/catch_test_macros.hpp>
#include <volk.h>
VKBP_ENABLE_WARNINGS()

#include <components/glslang/shader_compiler.hpp>
#include <components/spirv_reflect/shader_reflector.hpp>

#include "assets/assets.h"

using namespace components::spirv_reflect;
using namespace components::glslang;
using namespace components::vulkan;

// TEST_CASE("reflect hello triangle vert spirv", "[glslang]")
// {
// 	GlslangShaderCompiler compiler;
// 	std::vector<uint32_t> spirv = compiler.compile_spirv({VK_SHADER_STAGE_VERTEX_BIT}, HELLO_TRIANGLE_VERT);

// 	REQUIRE(spirv.size() != 0);

// 	SpirvReflectShaderReflector reflector;
// 	ShaderResources             resources = reflector.reflect_spirv(spirv);
// }

// TEST_CASE("reflect hello triangle frag spirv", "[glslang]")
// {
// 	GlslangShaderCompiler compiler;
// 	std::vector<uint32_t> spirv = compiler.compile_spirv({VK_SHADER_STAGE_FRAGMENT_BIT}, HELLO_TRIANGLE_FRAG);

// 	SpirvReflectShaderReflector reflector;
// 	ShaderResources             resources = reflector.reflect_spirv(spirv);
// }

TEST_CASE("reflect input attachment frag spirv", "[glslang]")
{
	GlslangShaderCompiler compiler;

	CompilerConfig config{};
	config.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	config.defines["MAX_LIGHT_COUNT"] = "5";

	std::vector<uint32_t> spirv = compiler.compile_spirv(config, BASE_FRAG);

	SpirvReflectShaderReflector reflector;
	ShaderResources             resources = reflector.reflect_spirv(spirv);
}