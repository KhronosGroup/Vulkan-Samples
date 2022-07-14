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

#include "components/platform/platform.hpp"

extern "C"
{
	// Sample Main
	typedef int (*PFN_SampleMain)(const components::PlatformContext *context);
}

struct Sample
{
	PFN_SampleMain sample_main{nullptr};
};

/**
 * @brief load 
 * 
 * @param library_name 
 * @param o_sample 
 * @return true 
 * @return false 
 */
bool load_sample(const std::string &library_name, Sample *o_sample);
