/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_pipeline.h>

namespace vkb
{
namespace common
{
struct HPPLoadStoreInfo;
}

namespace rendering
{
struct HPPAttachment;
}

namespace core
{
class HPPPipelineLayout;
class HPPRenderPass;
class HPPShaderModule;
struct HPPSubpassInfo;
}        // namespace core

/**
 * @brief facade class around vkb::ResourceRecord, providing a vulkan.hpp-based interface
 *
 * See vkb::ResourceRecord for documentation
 */
class HPPResourceRecord
{
  public:
	void set_data(const std::vector<uint8_t> &data);

	std::vector<uint8_t> get_data();

	const std::ostringstream &get_stream();

	size_t register_graphics_pipeline(vk::PipelineCache pipeline_cache, vkb::rendering::HPPPipelineState &pipeline_state);
	size_t register_pipeline_layout(const std::vector<vkb::core::HPPShaderModule *> &shader_modules);
	size_t register_render_pass(const std::vector<vkb::rendering::HPPAttachment> &attachments, const std::vector<vkb::common::HPPLoadStoreInfo> &load_store_infos, const std::vector<vkb::core::HPPSubpassInfo> &subpasses);
	size_t register_shader_module(vk::ShaderStageFlagBits stage, const vkb::ShaderSource &glsl_source, const std::string &entry_point, const vkb::ShaderVariant &shader_variant);
	void   set_graphics_pipeline(size_t index, const vkb::core::HPPGraphicsPipeline &graphics_pipeline);
	void   set_pipeline_layout(size_t index, const vkb::core::HPPPipelineLayout &pipeline_layout);
	void   set_render_pass(size_t index, const vkb::core::HPPRenderPass &render_pass);
	void   set_shader_module(size_t index, const vkb::core::HPPShaderModule &shader_module);

  private:
	std::ostringstream stream;

	std::vector<size_t> shader_module_indices;
	std::vector<size_t> pipeline_layout_indices;
	std::vector<size_t> render_pass_indices;
	std::vector<size_t> graphics_pipeline_indices;

	std::unordered_map<const core::HPPShaderModule *, size_t>     shader_module_to_index;
	std::unordered_map<const core::HPPPipelineLayout *, size_t>   pipeline_layout_to_index;
	std::unordered_map<const core::HPPRenderPass *, size_t>       render_pass_to_index;
	std::unordered_map<const core::HPPGraphicsPipeline *, size_t> graphics_pipeline_to_index;
};
}        // namespace vkb