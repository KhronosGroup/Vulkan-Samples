/* Copyright (c) 2020-2025, Arm Limited and Contributors
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
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

#include "benchmark_mode.h"

#include "platform/platform.h"

namespace plugins
{
BenchmarkMode::BenchmarkMode() :
    BenchmarkModeTags("Benchmark Mode", "Log frame averages after running an app.", {vkb::Hook::OnUpdate, vkb::Hook::OnAppStart, vkb::Hook::OnAppClose}, {},
                      {{"benchmark", "Enable benchmark mode"}})
{}

bool BenchmarkMode::handle_option(std::deque<std::string> &arguments)
{
	assert(!arguments.empty() && (arguments[0].substr(0, 2) == "--"));
	std::string option = arguments[0].substr(2);
	if (option == "benchmark")
	{
		// Whilst in benchmark mode fix the fps so that separate runs are consistently simulated
		// This will effect the graph outputs of framerate
		platform->force_simulation_fps(60.0f);
		platform->force_render(true);

		arguments.pop_front();
		return true;
	}
	return false;
}

void BenchmarkMode::on_update(float delta_time)
{
	elapsed_time += delta_time;
	total_frames++;
}

void BenchmarkMode::on_app_start(const std::string &app_id)
{
	elapsed_time = 0;
	total_frames = 0;
	LOGI("Starting Benchmark for {}", app_id);
}

void BenchmarkMode::on_app_close(const std::string &app_id)
{
	LOGI("Benchmark for {} completed in {} seconds (ran {} frames, averaged {} fps)", app_id, elapsed_time, total_frames, total_frames / elapsed_time);
}
}        // namespace plugins
