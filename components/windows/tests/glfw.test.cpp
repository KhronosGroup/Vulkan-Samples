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

#include <catch2/catch_test_macros.hpp>

#include <components/windows/glfw.hpp>

using namespace components::windows;
using namespace components::events;

TEST_CASE("glfw window title correct", "[windows]")
{
	GLFWWindow window{"This Is A Headless Window"};
	REQUIRE(window.title() == "This Is A Headless Window");
}

TEST_CASE("glfw window extent correct", "[windows]")
{
	const Extent expected_initial_extent{270, 130};
	GLFWWindow   window{"", expected_initial_extent};
	REQUIRE(window.title() == "");

	auto window_extent = window.extent();
	REQUIRE(window_extent.width == expected_initial_extent.width);
	REQUIRE(window_extent.height == expected_initial_extent.height);

	const Extent expected_extent{100, 120};
	window.set_extent(expected_extent);
	window_extent = window.extent();
	REQUIRE(window_extent.width == expected_extent.width);
	REQUIRE(window_extent.height == expected_extent.height);
}

TEST_CASE("glfw window position correct", "[windows]")
{
	const Position expected_position{270, 130};
	GLFWWindow     window{};

	auto window_position = window.position();

	REQUIRE(window_position.x == 0);
	REQUIRE(window_position.y == 0);

	window.set_position(expected_position);
	window_position = window.position();

	REQUIRE(window_position.x == expected_position.x);
	REQUIRE(window_position.y == expected_position.y);
}

TEST_CASE("glfw event bus", "[windows]")
{
	std::shared_ptr<GLFWWindow> window = std::make_shared<GLFWWindow>();

	const Extent   expected_extent{270, 130};
	const Position expected_position{270, 130};

	EventBus bus;

	bus
	    .attach(window)
	    .each<ContentRectChangedEvent>([&](const ContentRectChangedEvent &event) {
		    REQUIRE(event.extent.width == expected_extent.width);
		    REQUIRE(event.extent.height == expected_extent.height);
	    })
	    .each<PositionChangedEvent>([&](const PositionChangedEvent &event) {
		    REQUIRE(event.position.x == expected_position.x);
		    REQUIRE(event.position.y == expected_position.y);
	    });

	window->set_extent(expected_extent);
	window->set_position(expected_position);

	bus.process();
}