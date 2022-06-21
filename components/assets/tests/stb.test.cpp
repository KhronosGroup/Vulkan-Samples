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

#include <string>
#include <vector>

#include <components/images/stb.hpp>
#include <components/vfs/filesystem.hpp>

using namespace components;

const char *TEST_IMAGE = "/tests";

TEST_CASE("load png", "[assets]")
{
	auto &fs = vfs::_default();

	CHECK(fs.file_exists("/tests/assets/BoxTextured/CesiumLogoFlat.png"));
}