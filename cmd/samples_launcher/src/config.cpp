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

#include "config.hpp"

#include "config_marshalers.hpp"

using namespace components;

namespace config
{
components::StackErrorPtr load_config_from_json(const std::vector<uint8_t> &data, Config *config)
{
	Config loaded_config;
	if (auto err = encoding::unmarshal_json(data, &loaded_config.samples))
	{
		err->push("failed to load config from json", "config.cpp", __LINE__);
		return err;
	}

	*config = loaded_config;
	return nullptr;
}
}        // namespace config