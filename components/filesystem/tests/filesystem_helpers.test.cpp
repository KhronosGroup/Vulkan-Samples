/* Copyright (c) 2024, Thomas Atkinson
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

#include <core/util/error.hpp>

VKBP_DISABLE_WARNINGS()
#include <catch2/catch_test_macros.hpp>
VKBP_ENABLE_WARNINGS()

#include <filesystem/filesystem.hpp>

TEST_CASE("Windows Filename", "[filesystem]")
{
	REQUIRE("file.txt" == vkb::filesystem::helpers::filename("C:\\path\\to\\file.txt"));
}