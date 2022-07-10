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

#include <iostream>

#include <components/platform/dl.hpp>
#include <components/platform/platform.hpp>
#include <components/platform/sample.hpp>

using namespace components;

CUSTOM_MAIN(context)
{
	Sample sample;

	auto library_name = dl::os_library_name("vkb__platform__dummy_sample");
	std::cout << "loading: " << library_name << "\n";

	if (!load_sample(library_name, &sample))
	{
		std::cout << "failed to load sample\n";
		return EXIT_FAILURE;
	}

	if (!(*sample.sample_main)(context))
	{
		std::cout << "failed to call sample_main()\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}