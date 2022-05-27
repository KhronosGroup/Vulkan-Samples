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

#include <components/images/ktx.hpp>
#include <components/vfs/filesystem.hpp>

using namespace components;

namespace
{
const char *TEST_IMAGE   = "/tests/assets/BoxTextured/CesiumLogoFlatPngMipMapped.ktx";
const char *TEST_IMAGE_2 = "/tests/assets/BoxTextured/CesiumLogoFlatPngMipMapped.ktx2";
}        // namespace

TEST_CASE("load png with ktx", "[assets]")
{
	auto &fs = vfs::_default();

	auto contents = fs.read_file(TEST_IMAGE);

	std::shared_ptr<assets::ImageAsset> image{};

	images::KtxLoader loader;
	loader.load_from_memory("image_name", contents, &image);

	CHECK(image->valid());
	CHECK(image->name == "image_name");
	CHECK(image->format == VK_FORMAT_R8G8B8A8_SRGB);
	CHECK(image->width() == 256);
	CHECK(image->height() == 256);
	CHECK(image->mips.size() == 9);

	uint32_t image_width     = 256;
	uint32_t previous_offset = 0;

	// byte offset increasing as KTX stores mips in memory highest to lowest
	for (size_t i = 0; i < image->mips.size(); i++)
	{
		CHECK(image->mips[i].offset == previous_offset);
		CHECK(image->mips[i].level == i);
		CHECK(image->mips[i].byte_length == image_width * image_width * 4);
		previous_offset += image_width * image_width * 4;
		image_width = image_width / 2;
	}
}

TEST_CASE("load png with ktx2", "[assets]")
{
	auto &fs = vfs::_default();

		auto contents = fs.read_file(TEST_IMAGE);

	std::shared_ptr<assets::ImageAsset> image{};

	images::KtxLoader loader;
	loader.load_from_memory("image_name", contents, &image);

	CHECK(image->valid());
	CHECK(image->name == "image_name");
	CHECK(image->format == VK_FORMAT_R8G8B8A8_SRGB);
	CHECK(image->width() == 256);
	CHECK(image->height() == 256);
	CHECK(image->mips.size() == 9);

	uint32_t image_width     = 256;
	uint32_t previous_offset = 0;

	for (size_t i = 0; i < image->mips.size(); i++)
	{
		uint32_t byte_length = image_width * image_width * 4;
		image_width          = image_width / 2;

		CHECK(image->mips[i].offset == previous_offset);
		previous_offset += image->mips[i].byte_length;

		CHECK(image->mips[i].level == i);
		CHECK(image->mips[i].byte_length == byte_length);
	}
}