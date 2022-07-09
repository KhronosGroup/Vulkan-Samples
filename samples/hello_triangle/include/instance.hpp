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

#include "context.hpp"

/**
 * @brief Validates a list of required extensions, comparing it with the available ones.
 *
 * @param required A vector containing required extension names.
 * @param available A VkExtensionProperties object containing available extensions.
 * @return true if all required extensions are available
 * @return false otherwise
 */
bool validate_extensions(const std::vector<const char *> &required, const std::vector<VkExtensionProperties> &available);

/**
 * @brief Validates a list of required layers, comparing it with the available ones.
 *
 * @param required A vector containing required layer names.
 * @param available A VkLayerProperties object containing available layers.
 * @return true if all required extensions are available
 * @return false otherwise
 */
bool validate_layers(const std::vector<const char *> &required, const std::vector<VkLayerProperties> &available);

/**
 * @brief Initializes the Vulkan instance.
 *
 * @param context A newly created Vulkan context.
 * @param required_instance_extensions The required Vulkan instance extensions.
 * @param required_validation_layers The required Vulkan validation layers
 */
void init_instance(Context &context, const std::vector<const char *> &required_instance_extensions, const std::vector<const char *> &required_validation_layers);
