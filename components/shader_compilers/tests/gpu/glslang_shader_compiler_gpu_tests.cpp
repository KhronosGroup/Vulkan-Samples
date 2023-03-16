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
VKBP_ENABLE_WARNINGS()

#include <components/glslang/shader_compiler.hpp>
#include <components/vulkan/context/context_builder.hpp>

#include "assets/assets.h"

using namespace components::glslang;

namespace components
{

inline vulkan::ContextPtr create_context(vulkan::QueuePtr *transfer_queue)
{
	// failed to initialise meta loader
	REQUIRE((vulkan::init_meta_loader() == VK_SUCCESS));

	vulkan::ContextBuilder builder;

	builder
	    .configure_instance()
	    .application_info(vulkan::default_application_info())
	    .enable_validation_layers()
	    .enable_debug_logger()
	    .done();

	builder
	    .select_gpu()
	    .score_device(
	        vulkan::scores::combined_scoring({
	            vulkan::scores::device_preference({VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU}),
	        }))
	    .done();

	builder
	    .configure_device()
	    .done();

	builder.request_queue(VK_QUEUE_TRANSFER_BIT, {}, transfer_queue);

	return builder.build();
}

TEST_CASE("compile shader", "[glslang]")
{
	vulkan::QueuePtr transfer_queue;
	auto             context = create_context(&transfer_queue);

	GlslangShaderCompiler compiler;

	auto vert = compiler.compile_spirv({VK_SHADER_STAGE_VERTEX_BIT}, HELLO_TRIANGLE_VERT);
	auto frag = compiler.compile_spirv({VK_SHADER_STAGE_FRAGMENT_BIT}, HELLO_TRIANGLE_FRAG);

	REQUIRE(vert.size() > 0);
	REQUIRE(frag.size() > 0);

	VkShaderModuleCreateInfo vert_module_create_info = {};
	vert_module_create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vert_module_create_info.codeSize                 = vert.size() * sizeof(uint32_t);
	vert_module_create_info.pCode                    = vert.data();

	VkShaderModule vert_module{VK_NULL_HANDLE};
	REQUIRE(vkCreateShaderModule(context->device, &vert_module_create_info, nullptr, &vert_module) == VK_SUCCESS);

	VkShaderModuleCreateInfo frag_module_create_info = {};
	frag_module_create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	frag_module_create_info.codeSize                 = frag.size() * sizeof(uint32_t);
	frag_module_create_info.pCode                    = frag.data();

	VkShaderModule frag_module{VK_NULL_HANDLE};
	REQUIRE(vkCreateShaderModule(context->device, &frag_module_create_info, nullptr, &frag_module) == VK_SUCCESS);

	if (vert_module != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(context->device, vert_module, nullptr);
	}

	if (frag_module != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(context->device, frag_module, nullptr);
	}
}

}        // namespace components