/* Copyright (c) 2021-2026, NVIDIA CORPORATION. All rights reserved.
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

#include "rendering/render_pipeline.h"
#include "rendering/subpasses/forward_subpass.h"

namespace vkb
{
namespace rendering
{
/**
 * @brief facade class around vkb::RenderPipeline, providing a vulkan.hpp-based interface
 *
 * See vkb::RenderPipeline for documentation
 */
class HPPRenderPipeline : private vkb::RenderPipeline
{
  public:
	void add_subpass(std::unique_ptr<vkb::rendering::subpasses::ForwardSubpassCpp> &&subpass)
	{
		vkb::RenderPipeline::add_subpass(
		    std::unique_ptr<vkb::rendering::subpasses::ForwardSubpassC>(reinterpret_cast<vkb::rendering::subpasses::ForwardSubpassC *>(subpass.release())));
	}

	void draw(vkb::core::CommandBufferCpp &command_buffer, vkb::rendering::HPPRenderTarget &render_target,
	          vk::SubpassContents contents = vk::SubpassContents::eInline)
	{
		vkb::RenderPipeline::draw(reinterpret_cast<vkb::core::CommandBufferC &>(command_buffer), reinterpret_cast<vkb::RenderTarget &>(render_target),
		                          static_cast<VkSubpassContents>(contents));
	}
};
}        // namespace rendering
}        // namespace vkb
