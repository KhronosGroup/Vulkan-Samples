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
#include "entity.h"

// Most of the Entity class implementation is in the header file
// This file is mainly for any methods that might need additional implementation

void Entity::Initialize()
{
	for (auto &component : components)
	{
		component->Initialize();
	}
}

void Entity::Update(std::chrono::milliseconds deltaTime)
{
	if (!active)
		return;

	for (auto &component : components)
	{
		if (component->IsActive())
		{
			component->Update(deltaTime);
		}
	}
}

void Entity::Render()
{
	if (!active)
		return;

	for (auto &component : components)
	{
		if (component->IsActive())
		{
			component->Render();
		}
	}
}
