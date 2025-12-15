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
#pragma once

#include "model_loader.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

// Forward declarations
class Engine;
class ModelLoader;

/**
 * @brief Load a GLTF model synchronously on the main thread.
 * @param engine The engine to create entities in.
 * @param modelPath The path to the GLTF model file.
 * @param position The position to place the model.
 * @param rotation The rotation to apply to the model.
 * @param scale The scale to apply to the model.
 */
bool LoadGLTFModel(Engine *engine, const std::string &modelPath,
                   const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale);

/**
 * @brief Load a GLTF model with default transform values.
 * @param engine The engine to create entities in.
 * @param modelPath The path to the GLTF model file.
 */
void LoadGLTFModel(Engine *engine, const std::string &modelPath);
