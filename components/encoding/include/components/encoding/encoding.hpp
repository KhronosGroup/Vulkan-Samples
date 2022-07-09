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

#include <components/common/stack_error.hpp>

#include <vector>

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
class Marshaler
{
  public:
	Marshaler()          = default;
	virtual ~Marshaler() = default;

	StackErrorPtr marshal(const Type &type, std::vector<uint8_t> *data) const = 0;
};

/**
 * @brief Helper to remove boilerplate needed to serialize values
 * 
 * @tparam MarshalerType concrete marshaler class
 * @tparam Type data type to serialize
 * @param type input type value
 * @param data output binary array
 * @return StackErrorPtr error if not null
 */
template <template <typename> class MarshalerType, typename Type>
StackErrorPtr marshal(const Type &type, std::vector<uint8_t> *data)
{
	static MarshalerType<Type> marshaler;
	return marshaler.marshal(type, data);
}

/**
 * @brief Used to deserialize a type from a given format. The format is represented in a binary array
 * 
 * @tparam Type The type to be deserialized
 */
template <typename Type>
class UnMarshaler
{
  public:
	UnMarshaler()          = default;
	virtual ~UnMarshaler() = default;

	StackErrorPtr unmarshal(const std::vector<uint8_t> &data, Type *type) const = 0;
};

/**
 * @brief Helper to remove boilerplate needed to deserialize values
 * 
 * @tparam MarshalerType concrete unmarshaler class
 * @tparam Type data type to deserialize
 * @param data input binary array
 * @param type output type value
 * @return StackErrorPtr error if not null
 */
template <template <typename> class MarshalerType, typename Type>
StackErrorPtr unmarshal(const std::vector<uint8_t> &data, Type *type)
{
	static MarshalerType<Type> unmarshaler;
	return unmarshaler.unmarshal(data, type);
}
}        // namespace encoding
}        // namespace components