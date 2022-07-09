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

#include <components/common/stack_error.hpp>

#include <sstream>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

/*
 * WARNING: serializing floats using nlohmann JSON does not work as expected. USE DOUBLES!
 */

namespace components
{
namespace encoding
{
/**
 * @brief Used to serialize a type into a different format. The format is represented in a binary array
 * 
 * @tparam Type The type to be serialized
 */
template <typename Type>
class JsonMarshaler
{
  public:
	JsonMarshaler()          = default;
	virtual ~JsonMarshaler() = default;

	StackErrorPtr marshal(const Type &type, std::vector<uint8_t> *data) const
	{
		try
		{
			// this is an awkward edge case mechanism. It is unlikely that this will be used in practice
			// used for testing
			json j;
			j[typeid(Type).name()] = type;
			std::string j_str      = j.dump();
			*data                  = std::vector<uint8_t>{j_str.begin(), j_str.end()};
		}
		catch (std::exception &e)
		{
			std::stringstream msg{};
			msg << "type not supported for serialization: " << e.what();
			return StackError::unique(msg.str(), "encoding/json.hpp", __LINE__);
		}

		return nullptr;
	}
};

template <typename Type>
StackErrorPtr marshal_json(const Type &type, std::vector<uint8_t> *data)
{
	return marshal<JsonMarshaler, Type>(type, data);
}

/**
 * @brief Used to deserialize a type from a given format. The format is represented in a binary array
 * 
 * @tparam Type The type to be deserialized
 */
template <typename Type>
class JsonUnMarshaler
{
  public:
	JsonUnMarshaler()          = default;
	virtual ~JsonUnMarshaler() = default;

	StackErrorPtr unmarshal(const std::vector<uint8_t> &data, Type *type) const
	{
		try
		{
			std::string json_string{data.begin(), data.end()};
			json        J = json::parse(json_string);
			*type         = J.get<Type>();
		}
		catch (std::exception &e)
		{
			std::stringstream msg{};
			msg << "type not supported for deserialization: " << e.what();
			return StackError::unique(msg.str(), "encoding/json.hpp", __LINE__);
		}

		return nullptr;
	}
};

template <typename Type>
StackErrorPtr unmarshal_json(const std::vector<uint8_t> &data, Type *type)
{
	return unmarshal<JsonUnMarshaler, Type>(data, type);
}
}        // namespace encoding
}        // namespace components