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

#include "components/events/event_pipeline.hpp"

#include <cassert>

namespace components
{
namespace events
{
<<<<<<< HEAD:components/events/src/event_pipeline.cpp
EventPipeline &EventPipeline::add_once(std::unique_ptr<EventPipelineStage> &&stage)
=======
EventPipeline &EventPipeline::once(std::unique_ptr<EventPipelineStage> &&stage)
>>>>>>> 9591042... Pipeline -> EventPipeline:components/events/src/pipelines.cpp
{
	m_once_stages.emplace_back(std::move(stage));
	return *this;
}

<<<<<<< HEAD:components/events/src/event_pipeline.cpp
EventPipeline &EventPipeline::add_always(std::unique_ptr<EventPipelineStage> &&stage)
=======
EventPipeline &EventPipeline::then(std::unique_ptr<EventPipelineStage> &&stage)
>>>>>>> 9591042... Pipeline -> EventPipeline:components/events/src/pipelines.cpp
{
	m_stages.emplace_back(std::move(stage));
	return *this;
}

void EventPipeline::process()
{
	if (!running)
	{
		for (auto &stage : m_once_stages)
		{
			// TODO: Add Tracy ZoneScoped(stage->name());

			// flush after each stage to make sure pipeline stages are sequential
			stage->emit(*this);
			flush_callbacks();
		}

		running = true;
	}

	// query observer events
	EventBus::process();
	flush_callbacks();

	for (auto &stage : m_stages)
	{
		// TODO: Add Tracy ZoneScoped(stage->name());

		// flush after each stage to make sure pipeline stages are sequential
		stage->emit(*this);
		flush_callbacks();
	}
}
}        // namespace events
}        // namespace components