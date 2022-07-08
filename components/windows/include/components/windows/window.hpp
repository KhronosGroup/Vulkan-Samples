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

#pragma once

#include <stdint.h>

#include <components/events/event_bus.hpp>

namespace components
{
namespace windows
{
struct Extent
{
	uint32_t width;
	uint32_t height;
};

struct Position
{
	uint32_t x;
	uint32_t y;
};

struct ContentRectChangedEvent
{
	Extent extent;
};

struct PositionChangedEvent
{
	Position position;
};

class Window : public events::EventObserver
{
  public:
	Window(const std::string &title) :
	    m_title{title}
	{}

	virtual ~Window() = default;

	virtual void   set_extent(const Extent &extent) = 0;
	virtual Extent extent() const                   = 0;

	virtual void     set_position(const Position &position) = 0;
	virtual Position position() const                       = 0;

	virtual float dpi_factor() const = 0;

	inline void set_title(const std::string &title)
	{
		m_title = title;
	}

	inline const std::string &title() const
	{
		return m_title;
	}

  private:
	std::string m_title;
};
}        // namespace windows
}        // namespace components