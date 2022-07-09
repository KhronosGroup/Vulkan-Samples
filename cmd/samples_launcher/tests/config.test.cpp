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

#include <vector>

#include <config_marshalers.hpp>

using namespace components::encoding;

#define CHECK_ERROR(err)         \
	if (err != nullptr)          \
	{                            \
		INFO(err->what());       \
		REQUIRE(err == nullptr); \
	}

TEST_CASE("load config", "[sample_launcher]")
{
	std::string json_string = "{\"samples\":[{\"description\":\"this is a fake\",\"library_name\":\"lib1\",\"name\":\"fake_sample_one\"},{\"description\":\"this is a fake\",\"library_name\":\"lib2\",\"name\":\"fake_sample_two\"},{\"description\":\"this is a fake\",\"library_name\":\"lib3\",\"name\":\"fake_sample_three\"}]}";

	std::vector<config::Sample>
	    sample_list = {
	        config::Sample{
	            "fake_sample_one",
	            "this is a fake",
	            "lib1"},
	        config::Sample{
	            "fake_sample_two",
	            "this is a fake",
	            "lib2"},
	        config::Sample{
	            "fake_sample_three",
	            "this is a fake",
	            "lib3"},
	    };

	config::Config some_config;

	auto err = config::load_config_from_json({json_string.begin(), json_string.end()}, &some_config);
	CHECK_ERROR(err);

	for (size_t i = 0; i < sample_list.size(); i++)
	{
		REQUIRE(some_config.samples[i].name == sample_list[i].name);
		REQUIRE(some_config.samples[i].description == sample_list[i].description);
	}
}