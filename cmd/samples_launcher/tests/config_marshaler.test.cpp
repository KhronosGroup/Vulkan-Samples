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

TEST_CASE("un/marshal Sample config", "[sample_launcher]")
{
	config::Sample sample;
	sample.name         = "fake_sample";
	sample.description  = "this is a fake";
	sample.library_name = "lib1";

	std::vector<uint8_t> data;

	auto err = marshal_json(sample, &data);
	CHECK_ERROR(err);

	config::Sample new_sample;
	err = unmarshal_json(data, &new_sample);
	CHECK_ERROR(err);

	REQUIRE(new_sample.name == sample.name);
	REQUIRE(new_sample.description == sample.description);
	REQUIRE(new_sample.library_name == sample.library_name);
}

TEST_CASE("un/marshal Sample config list", "[sample_launcher]")
{
	std::vector<config::Sample> sample_list = {
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

	std::vector<uint8_t> data;

	auto err = marshal_json(sample_list, &data);
	CHECK_ERROR(err);

	std::string data_str{data.begin(), data.end()};
	INFO(data_str);

	std::vector<config::Sample> new_sample_list;
	err = unmarshal_json(data, &new_sample_list);
	CHECK_ERROR(err);

	REQUIRE(sample_list.size() == new_sample_list.size());

	for (size_t i = 0; i < sample_list.size(); i++)
	{
		REQUIRE(sample_list[i].name == new_sample_list[i].name);
		REQUIRE(sample_list[i].description == new_sample_list[i].description);
		REQUIRE(sample_list[i].library_name == new_sample_list[i].library_name);
	}
}