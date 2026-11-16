/* Copyright (c) 2026, Holochip Corporation
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

// Precompiled header shared by the framework, the samples, the sample registry (apps) and the plugins.
//
// Only add stable headers that are expensive to parse. Every change to a header listed here (or included by one)
// rebuilds everything, as it already would without a PCH.
//
// Translation units that define a *_IMPLEMENTATION macro (VMA_IMPLEMENTATION, TINYGLTF_IMPLEMENTATION,
// STB_*_IMPLEMENTATION) before including the library header must not use this PCH, because it already includes those
// headers. They are listed with SKIP_PRECOMPILE_HEADERS in framework/CMakeLists.txt.

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <vulkan/vulkan.hpp>

#include "common/glm_common.h"

// Sample base classes: these pull in most of the framework
#include "api_vulkan_sample.h"
#include "common/vk_common.h"
#include "hpp_api_vulkan_sample.h"
#include "vulkan_sample.h"
