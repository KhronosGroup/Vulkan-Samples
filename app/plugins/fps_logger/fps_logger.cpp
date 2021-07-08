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

#include "fps_logger.h"

namespace plugins
{
FpsLogger::FpsLogger() :
    FpsLoggerTags("FPS Logger",
                  "Enable FPS logging.",
                  {vkb::Hook::OnUpdate, vkb::Hook::OnAppStart}, {&fps_flag})
{
}

bool FpsLogger::is_active(const vkb::CommandParser &parser)
{
	return parser.contains(&fps_flag);
}

void FpsLogger::init(const vkb::CommandParser &parser)
{
}

void FpsLogger::on_update(float delta_time)
{
	if (!timer.is_running())
	{
		timer.start();
	}

	auto elapsed_time = static_cast<float>(timer.elapsed<vkb::Timer::Seconds>());

	frame_count++;

	if (elapsed_time > 0.5f)
	{
		auto fps = (frame_count - last_frame_count) / elapsed_time;

		LOGI("FPS: {:.1f}", fps);

		last_frame_count = frame_count;
		timer.lap();
	}
};
}        // namespace plugins