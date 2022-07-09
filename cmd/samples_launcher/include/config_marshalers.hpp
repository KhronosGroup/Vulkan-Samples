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

#include "config.hpp"

#include <components/encoding/json.hpp>

namespace components
{
namespace encoding
{
inline config::Sample from(const json &j)
{
	config::Sample sample;
	sample.name         = j["name"];
	sample.description  = j["description"];
	sample.library_name = j["library_name"];
	return sample;
}

inline json to(const config::Sample &sample)
{
	json j;
	j["name"]         = sample.name;
	j["description"]  = sample.description;
	j["library_name"] = sample.library_name;
	return j;
}

template <>
class JsonMarshaler<config::Sample>
{
  public:
	JsonMarshaler<config::Sample>()          = default;
	virtual ~JsonMarshaler<config::Sample>() = default;

	StackErrorPtr marshal(const config::Sample &sample, std::vector<uint8_t> *data) const
	{
		assert(data);

		HANDLE_JSON_ERROR_START()

		json        j     = to(sample);
		std::string j_str = j.dump();
		*data             = std::vector<uint8_t>{j_str.begin(), j_str.end()};

		HANDLE_JSON_ERROR_END()

		return nullptr;
	}
};

template <>
class JsonUnMarshaler<config::Sample>
{
  public:
	JsonUnMarshaler()          = default;
	virtual ~JsonUnMarshaler() = default;

	StackErrorPtr unmarshal(const std::vector<uint8_t> &data, config::Sample *type) const
	{
		assert(type);

		HANDLE_JSON_ERROR_START()

		std::string json_string{data.begin(), data.end()};
		json        j = json::parse(json_string);
		*type         = from(j);

		HANDLE_JSON_ERROR_END()

		return nullptr;
	}
};

template <>
class JsonMarshaler<std::vector<config::Sample>>
{
  public:
	JsonMarshaler<std::vector<config::Sample>>()          = default;
	virtual ~JsonMarshaler<std::vector<config::Sample>>() = default;

	StackErrorPtr marshal(const std::vector<config::Sample> &type, std::vector<uint8_t> *data) const
	{
		assert(data);

		HANDLE_JSON_ERROR_START()

		json j;

		for (auto &sample : type)
		{
			j["samples"].push_back(to(sample));
		}

		std::string j_str = j.dump();
		*data             = std::vector<uint8_t>{j_str.begin(), j_str.end()};

		HANDLE_JSON_ERROR_END()

		return nullptr;
	}
};

template <>
class JsonUnMarshaler<std::vector<config::Sample>>
{
  public:
	JsonUnMarshaler()          = default;
	virtual ~JsonUnMarshaler() = default;

	StackErrorPtr unmarshal(const std::vector<uint8_t> &data, std::vector<config::Sample> *type) const
	{
		assert(type);

		HANDLE_JSON_ERROR_START()

		std::string json_string{data.begin(), data.end()};
		json        J = json::parse(json_string);

		if (!J.contains("samples") || !J["samples"].is_array())
		{
			return StackError::unique("samples is not an array", "config_marshalers.hpp", __LINE__);
		}

		auto &samples_list_j = J["samples"];

		std::vector<config::Sample> samples_list;
		samples_list.reserve(samples_list_j.size());

		for (auto &sample_j : J["samples"])
		{
			samples_list.push_back(from(sample_j));
		}

		*type = samples_list;

		HANDLE_JSON_ERROR_END()

		return nullptr;
	}
};
}        // namespace encoding
}        // namespace components