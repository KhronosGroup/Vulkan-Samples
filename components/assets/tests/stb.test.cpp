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

namespace
{
const char *TEST_IMAGE = "/tests/assets/BoxTextured/CesiumLogoFlat.png";
}

TEST_CASE("load png with stb", "[assets]")
{
	auto &fs = vfs::_default();

	auto contents = fs.read_file(TEST_IMAGE);

	std::shared_ptr<assets::ImageAsset> image{};

	images::StbLoader loader;
	loader.load_from_memory("image_name", contents, &image);

	CHECK(image->valid());
	CHECK(image->name == "image_name");
	CHECK(image->format == VK_FORMAT_R8G8B8A8_UNORM);
	CHECK(image->width() == 256);
	CHECK(image->height() == 256);
	CHECK(image->mips.size() == 1);
	CHECK(image->mips[0].offset == 0);
	CHECK(image->mips[0].level == 0);
	CHECK(image->mips[0].byte_length == 256 * 256 * 4);
}