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

#include <components/windows/headless.hpp>

namespace components
{
namespace windows
{
HeadlessWindow::HeadlessWindow(const std::string &title, const Extent &initial_extent) :
    Window{title},
    m_extent{initial_extent}
{
}

void HeadlessWindow::set_extent(const Extent &extent)
{
	m_extent         = extent;
	m_extent_changed = true;
}

Extent HeadlessWindow::extent() const
{
	return m_extent;
}

void HeadlessWindow::set_position(const Position &position)
{
	m_position         = position;
	m_position_changed = true;
}

Position HeadlessWindow::position() const
{
	return m_position;
}

float HeadlessWindow::dpi_factor() const
{
	return m_dpi_factor;
}

void HeadlessWindow::update()
{
	if (m_extent_changed)
	{
		m_content_rect_sender->push(ContentRectChangedEvent{m_extent});
		m_extent_changed = false;
	}

	if (m_position_changed)
	{
		m_position_sender->push(PositionChangedEvent{m_position});
		m_position_changed = false;
	}
}

void HeadlessWindow::attach(events::EventBus &bus)
{
	m_content_rect_sender = bus.request_sender<ContentRectChangedEvent>();
	m_position_sender     = bus.request_sender<PositionChangedEvent>();
}

}        // namespace windows
}        // namespace components