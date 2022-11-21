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

#include "components/platform/sample.hpp"

#include "components/platform/dl.hpp"

using namespace components;

bool load_sample(const std::string &library_name, Sample *o_sample)
{
	void *lib_handle = dl::open_library(library_name.c_str());
	if (!lib_handle)
	{
		return false;
	}

	Sample sample;

	sample.sample_main = dl::load_function<PFN_SampleMain>(lib_handle, "sample_main");
	if (!sample.sample_main)
	{
		return false;
	}

	*o_sample = sample;

	return true;
}
