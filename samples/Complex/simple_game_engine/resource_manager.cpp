/* Copyright (c) 2025 Holochip Corporation
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
#include "resource_manager.h"

#include <ranges>

// Most of the ResourceManager class implementation is in the header file
// This file is mainly for any methods that might need additional implementation
//
// This implementation corresponds to the Engine_Architecture chapter in the tutorial:
// @see en/Building_a_Simple_Engine/Engine_Architecture/04_resource_management.adoc

bool Resource::Load()
{
	loaded = true;
	return true;
}

void Resource::Unload()
{
	loaded = false;
}

void ResourceManager::UnloadAllResources()
{
	for (auto &kv : resources)
	{
		auto &val = kv.second;
		for (auto &innerKv : val)
		{
			auto &loadedResource = innerKv.second;
			loadedResource->Unload();
		}
		val.clear();
	}
	resources.clear();
}
