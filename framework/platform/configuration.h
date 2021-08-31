/* Copyright (c) 2019, Arm Limited and Contributors
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

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace vkb
{
class Setting
{
  public:
	Setting() = default;

	Setting(Setting &&other) = default;

	virtual ~Setting() = default;

	virtual void set() = 0;

	virtual std::type_index get_type() = 0;
};

class BoolSetting : public Setting
{
  public:
	BoolSetting(bool &handle, bool value);

	void set() override;

	std::type_index get_type() override;

  private:
	bool &handle;

	bool value;
};

class IntSetting : public Setting
{
  public:
	IntSetting(int &handle, int value);

	void set() override;

	std::type_index get_type() override;

  private:
	int &handle;

	int value;
};

class EmptySetting : public Setting
{
  public:
	EmptySetting() = default;

	void set() override;

	std::type_index get_type() override;
};

using ConfigMap = std::map<uint32_t, std::unordered_map<std::type_index, std::vector<Setting *>>>;

/**
 * @brief A class that contains configuration data for a sample.
 */
class Configuration
{
  public:
	/**
	 * @brief Constructor
	 */
	Configuration() = default;

	/**
	 * @brief Configures the settings in the current config
	 */
	void set();

	/**
	 * @brief Increments the configuration count
	 * @returns True if the current configuration iterator was incremented
	 */
	bool next();

	/**
	 * @brief Resets the configuration to beginning
	 */
	void reset();

	/**
	 * @brief Inserts a setting into the current configuration
	 * @param config_index The configuration to insert the setting into
	 * @param setting A setting to be inserted into the configuration
	 */
	void insert_setting(uint32_t config_index, std::unique_ptr<Setting> setting);

	/**
	 * @brief Inserts a setting into the current configuration
	 * @param config_index The configuration to insert the setting into
	 * @param args A parameter pack containing the parameters to initialize a setting object
	 */
	template <class T, class... A>
	void insert(uint32_t config_index, A &&... args)
	{
		static_assert(std::is_base_of<Setting, T>::value,
		              "T is not a type of setting.");

		insert_setting(config_index, std::make_unique<T>(args...));
	}

  protected:
	ConfigMap configs;

	std::vector<std::unique_ptr<Setting>> settings;

	ConfigMap::iterator current_configuration;
};
}        // namespace vkb
