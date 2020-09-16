/* Copyright (c) 2020, Arm Limited and Contributors
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

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "platform/application.h"

/*
 *
 *	Vulkan Samples currently has two types of apps, Samples and Tests. These apps work from the same interface AppInfo.
 *	Samples and Tests are categorised into namespaces samples and tests respectively. Each name space has a method to retrieve
 *  the AppInfo interface version of the app (tests::get_apps, samples::get_apps). 
 *
 */

namespace apps
{
using CreateFunc = std::function<std::unique_ptr<vkb::Application>()>;

/*
 *  Apps - Used by Vulkan Samples to load a vkb::Applicaiton with the creation function (CreateFunc create).
 */

class AppInfo
{
  public:
	AppInfo(const std::string &id, const CreateFunc &create) :
	    id(id), create(create)
	{}

	std::string id;
	CreateFunc  create;
};

/*
 *  Samples - These are individual applications which show different usages and optimizations of the Vulkan API
 */

class SampleInfo : public AppInfo
{
  public:
	SampleInfo(const std::string &id, const CreateFunc &create, const std::string &category, const std::string &author, const std::string &name, const std::string &description, const std::vector<std::string> &tags = {}) :
	    AppInfo(id, create), category(category), author(author), name(name), description(description), tags(tags)
	{}

	std::string              category;
	std::string              author;
	std::string              name;
	std::string              description;
	std::vector<std::string> tags;
};

/*
 *  Tests - Used to test Vulkan Samples functionality
 */

class TestInfo : public AppInfo
{
  public:
	TestInfo(const std::string &id, const CreateFunc &create) :
	    AppInfo(id, create)
	{}
};

/**
 * @brief Get a specific app
 * 
 * @param id ID of a specific app
 * @return const std::vector<AppInfo *> 
 */
AppInfo *get_app(const std::string &id);

/**
 * @brief Get all apps
 * 
 * @return const std::vector<AppInfo *> A list of all apps
 */
std::vector<AppInfo *> get_apps();

/**
 * @brief Get all samples
 * 
 * @param categories If not empty the lists will include samples that match on of the categories requested
 * @param tags If not empty the lists will include samples that match on of the tags requested
 * @return std::vector<AppInfo *> A list of samples
 */
std::vector<SampleInfo *> get_samples(const std::vector<std::string> &categories = {}, const std::vector<std::string> &tags = {});

/**
 * @brief Get all tests
 * 
 * @return std::vector<AppInfo *> A list of Tests
 */
std::vector<TestInfo *> get_tests();
}        // namespace apps