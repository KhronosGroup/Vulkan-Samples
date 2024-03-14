/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include <stats/stats.h>

#include <rendering/hpp_render_context.h>

namespace vkb
{
namespace stats
{
/**
 * @brief facade class around vkb::Stats, providing a vulkan.hpp-based interface
 *
 * See vkb::Stats for documentation
 */
class HPPStats : private vkb::Stats
{
  public:
	using vkb::Stats::get_data;
	using vkb::Stats::get_graph_data;
	using vkb::Stats::get_requested_stats;
	using vkb::Stats::is_available;
	using vkb::Stats::request_stats;
	using vkb::Stats::resize;
	using vkb::Stats::update;

	explicit HPPStats(vkb::rendering::HPPRenderContext &render_context, size_t buffer_size = 16) :
	    vkb::Stats(reinterpret_cast<vkb::RenderContext &>(render_context), buffer_size)
	{}

	void begin_sampling(vkb::core::CommandBuffer<vkb::BindingType::Cpp> &cb)
	{
		vkb::Stats::begin_sampling(reinterpret_cast<vkb::core::CommandBuffer<vkb::BindingType::C> &>(cb));
	}

	void end_sampling(vkb::core::CommandBuffer<vkb::BindingType::Cpp> &cb)
	{
		vkb::Stats::end_sampling(reinterpret_cast<vkb::core::CommandBuffer<vkb::BindingType::C> &>(cb));
	}
};

}        // namespace stats
}        // namespace vkb
