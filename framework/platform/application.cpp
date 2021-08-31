/* Copyright (c) 2019-2021, Arm Limited and Contributors
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

#include "application.h"

#include "common/logging.h"
#include "platform/platform.h"

namespace vkb
{
Application::Application() :
    name{"Sample Name"}
{
}

bool Application::prepare(Platform & /*platform*/)
{
	auto &_debug_info = get_debug_info();
	_debug_info.insert<field::MinMax, float>("fps", fps);
	_debug_info.insert<field::MinMax, float>("frame_time", frame_time);

	timer.start();
	return true;
}

void Application::step()
{
	auto delta_time = static_cast<float>(timer.tick<Timer::Seconds>());

	// Fix the framerate to 60 FPS for benchmark mode and avoid
	// a huge spike in frame time while loading the app
	if (benchmark_mode || frame_count == 0)
	{
		delta_time = 0.01667f;
	}

	if (focus || benchmark_mode)
	{
		update(delta_time);
	}

	auto elapsed_time = static_cast<float>(timer.elapsed<Timer::Seconds>());

	frame_count++;

	if (elapsed_time > 0.5f)
	{
		fps        = static_cast<float>(frame_count - last_frame_count) / elapsed_time;
		frame_time = delta_time * 1000.0f;

		LOGI("FPS: {:.1f}", fps)

		last_frame_count = frame_count;
		timer.lap();
	}
}

void Application::finish()
{
	auto execution_time = timer.stop();
	LOGI("Closing App (Runtime: {:.1f})", execution_time)
}

void Application::resize(const uint32_t /*width*/, const uint32_t /*height*/)
{
}

void Application::input_event(const InputEvent &input_event)
{
	if (input_event.get_source() == EventSource::Keyboard)
	{
		const auto &key_event = dynamic_cast<const KeyInputEvent &>(input_event);

		if (key_event.get_code() == KeyCode::Back ||
		    key_event.get_code() == KeyCode::Escape)
		{
			input_event.get_platform().close();
		}
	}
}

const std::string &Application::get_name() const
{
	return name;
}

void Application::set_name(const std::string &name_)
{
	name = name_;
}

bool Application::is_benchmark_mode() const
{
	return benchmark_mode;
}

DebugInfo &Application::get_debug_info()
{
	return debug_info;
}

void Application::set_benchmark_mode(bool benchmark_mode_)
{
	benchmark_mode = benchmark_mode_;
}

bool Application::is_headless() const
{
	return headless;
}

void Application::set_headless(bool _headless)
{
	this->headless = _headless;
}

void Application::set_focus(bool flag)
{
	focus = flag;
}

bool Application::is_focused() const
{
	return focus;
}

void Application::parse_options(const std::vector<std::string> &args)
{
}

}        // namespace vkb
