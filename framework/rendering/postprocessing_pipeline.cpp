/* Copyright (c) 2020-2021, Arm Limited and Contributors
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

#include "postprocessing_pipeline.h"

#include "common/utils.h"

namespace vkb
{
PostProcessingPipeline::PostProcessingPipeline(RenderContext &render_context, ShaderSource triangle_vs) :
    render_context{&render_context},
    triangle_vs{std::move(triangle_vs)}
{}

void PostProcessingPipeline::draw(CommandBuffer &command_buffer, RenderTarget &default_render_target)
{
	for (current_pass_index = 0; current_pass_index < passes.size(); current_pass_index++)
	{
		auto &pass = *passes[current_pass_index];

		if (pass.debug_name.empty())
		{
			pass.debug_name = fmt::format("PPP pass #{}", current_pass_index);
		}
		ScopedDebugLabel marker{command_buffer, pass.debug_name.c_str()};

		if (!pass.prepared)
		{
			ScopedDebugLabel marker{command_buffer, "Prepare"};

			pass.prepare(command_buffer, default_render_target);
			pass.prepared = true;
		}

		if (pass.pre_draw)
		{
			ScopedDebugLabel marker{command_buffer, "Pre-draw"};

			pass.pre_draw();
		}

		pass.draw(command_buffer, default_render_target);

		if (pass.post_draw)
		{
			ScopedDebugLabel marker{command_buffer, "Post-draw"};

			pass.post_draw();
		}
	}

	current_pass_index = 0;
}

}        // namespace vkb
