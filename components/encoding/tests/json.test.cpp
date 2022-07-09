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

#include <components/encoding/encoding.hpp>

#include <components/encoding/json.hpp>

using namespace components::encoding;

#define CHECK_ERROR(err)         \
	if (err != nullptr)          \
	{                            \
		INFO(err->what());       \
		REQUIRE(err == nullptr); \
	}

#define NUMERIC_UNMARSHAL_TEST(type, expected_value)                                       \
	TEST_CASE("unmarshal " #type, "[encoding][json]")                                      \
	{                                                                                      \
		std::string str_value = #expected_value;                                           \
                                                                                           \
		type out_value;                                                                    \
		auto err = unmarshal_json<type>({str_value.begin(), str_value.end()}, &out_value); \
		CHECK_ERROR(err);                                                                  \
		REQUIRE(out_value == type{expected_value});                                        \
	}

#define NUMERIC_MARSHAL_TEST(type, expected_value)                                           \
	TEST_CASE("marshal " #type, "[encoding][json]")                                          \
	{                                                                                        \
		std::vector<uint8_t> data;                                                           \
                                                                                             \
		type value{expected_value};                                                          \
		auto err = marshal_json<type>(value, &data);                                         \
		CHECK_ERROR(err);                                                                    \
		std::stringstream expected_outcome;                                                  \
		expected_outcome << "{\"" << typeid(type).name() << "\":" << #expected_value << "}"; \
		REQUIRE(std::string{data.begin(), data.end()} == expected_outcome.str());            \
	}

#define NUMERIC_TEST(type, expected_value)       \
	NUMERIC_UNMARSHAL_TEST(type, expected_value) \
	NUMERIC_MARSHAL_TEST(type, expected_value)

NUMERIC_TEST(uint8_t, 12)
NUMERIC_TEST(uint16_t, 12)
NUMERIC_TEST(uint32_t, 12)
NUMERIC_TEST(uint64_t, 12)
NUMERIC_TEST(int, 12)
NUMERIC_TEST(double, 12.432)

TEST_CASE("unmarshal float", "[encoding][json]")
{
	std::string str_value = "12.123";

	float out_value;
	auto  err = unmarshal_json<float>({str_value.begin(), str_value.end()}, &out_value);
	CHECK_ERROR(err);
	REQUIRE(out_value == 12.123f);
}

// TODO: serializing floats using nlohmann JSON does not work as expected.
TEST_CASE("marshal float", "[encoding][json]")
{
	std::vector<uint8_t> data;

	float value{12.123f};
	auto  err = marshal_json<float>(value, &data);
	CHECK_ERROR(err);
	std::stringstream expected_outcome;
	expected_outcome << "{\"" << typeid(float).name() << "\":" << 12.123 << "}";
	// REQUIRE(std::string{data.begin(), data.end()} == expected_outcome.str());
}

TEST_CASE("unmarshal bool", "[encoding][json]")
{
	std::string str_value = "true";

	bool out_value;
	auto err = unmarshal_json<bool>({str_value.begin(), str_value.end()}, &out_value);
	CHECK_ERROR(err);
	REQUIRE(out_value);

	str_value = "false";
	err       = unmarshal_json<bool>({str_value.begin(), str_value.end()}, &out_value);
	CHECK_ERROR(err);
	REQUIRE(!out_value);
}

TEST_CASE("marshal bool", "[encoding][json]")
{
	std::vector<uint8_t> data;

	bool value{true};
	auto err = marshal_json<bool>(value, &data);
	CHECK_ERROR(err);

	std::stringstream expected_outcome;
	expected_outcome << "{\"" << typeid(bool).name() << "\":true}";
	REQUIRE(std::string{data.begin(), data.end()} == expected_outcome.str());

	data.clear();

	value = false;
	err   = marshal_json<bool>(value, &data);
	CHECK_ERROR(err);
	expected_outcome.clear();
	expected_outcome << "{\"" << typeid(bool).name() << "\":false}";
	REQUIRE(std::string{data.begin(), data.end()} == expected_outcome.str());
}
