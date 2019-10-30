/* Copyright (c) 2018-2019, Arm Limited and Contributors
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

#include "buffer_pool.h"
#include "common/error.h"
#include "common/helpers.h"
#include "common/logging.h"
#include "common/utils.h"
#include "common/vk_common.h"
#include "debug_info.h"
#include "fence_pool.h"
#include "glsl_compiler.h"
#include "gui.h"
#include "resource_binding_state.h"
#include "resource_cache.h"
#include "resource_record.h"
#include "resource_replay.h"
#include "semaphore_pool.h"
#include "spirv_reflection.h"
#include "stats.h"
#include "timer.h"
#include "vulkan_sample.h"
