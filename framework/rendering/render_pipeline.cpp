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

#include "render_pipeline.h"

#include "scene_graph/components/camera.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/sampler.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/node.h"

namespace vkb
{
RenderPipeline::RenderPipeline(std::vector<std::unique_ptr<vkb::rendering::SubpassC>> &&subpasses_) :
    subpasses{std::move(subpasses_)}
{
	prepare();

	// Default clear value
	clear_value[0].color        = {0.0f, 0.0f, 0.0f, 1.0f};
	clear_value[1].depthStencil = {0.0f, ~0U};
}

void RenderPipeline::prepare()
{
	for (auto &subpass : subpasses)
	{
		subpass->prepare();
	}
}

void RenderPipeline::add_subpass(std::unique_ptr<vkb::rendering::SubpassC> &&subpass)
{
	subpass->prepare();
	subpasses.emplace_back(std::move(subpass));
}

std::vector<std::unique_ptr<vkb::rendering::SubpassC>> &RenderPipeline::get_subpasses()
{
	return subpasses;
}

const std::vector<LoadStoreInfo> &RenderPipeline::get_load_store() const
{
	return load_store;
}

void RenderPipeline::set_load_store(const std::vector<LoadStoreInfo> &ls)
{
	load_store = ls;
}

const std::vector<VkClearValue> &RenderPipeline::get_clear_value() const
{
	return clear_value;
}

void RenderPipeline::set_clear_value(const std::vector<VkClearValue> &cv)
{
	clear_value = cv;
}

void RenderPipeline::draw(CommandBuffer &command_buffer, RenderTarget &render_target, VkSubpassContents contents)
{
	assert(!subpasses.empty() && "Render pipeline should contain at least one sub-pass");

	// Pad clear values if they're less than render target attachments
	while (clear_value.size() < render_target.get_attachments().size())
	{
		clear_value.push_back({0.0f, 0.0f, 0.0f, 1.0f});
	}

	for (size_t i = 0; i < subpasses.size(); ++i)
	{
		active_subpass_index = i;

		auto &subpass = subpasses[i];

		subpass->update_render_target_attachments(render_target);

		if (i == 0)
		{
			command_buffer.begin_render_pass(render_target, load_store, clear_value, subpasses, contents);
		}
		else
		{
			command_buffer.next_subpass();
		}

		if (subpass->get_debug_name().empty())
		{
			subpass->set_debug_name(fmt::format("RP subpass #{}", i));
		}
		ScopedDebugLabel subpass_debug_label{command_buffer, subpass->get_debug_name().c_str()};

		subpass->draw(command_buffer);
	}

	active_subpass_index = 0;
}

std::unique_ptr<vkb::rendering::SubpassC> &RenderPipeline::get_active_subpass()
{
	return subpasses[active_subpass_index];
}
}        // namespace vkb
