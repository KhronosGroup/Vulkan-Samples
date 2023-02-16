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

#include <stdexcept>

#include <components/common/strings.hpp>
#include <components/platform/dl.hpp>
#include <components/platform/platform.hpp>
#include <components/platform/sample.hpp>
#include <components/vfs/filesystem.hpp>

// fallbacks incase the library path is not set externally
#ifndef SOURCE_PATH
#	define SOURCE_PATH ""
#endif

#ifndef SAMPLE_LIB_PATH
#	define SAMPLE_LIB_PATH "/"
#endif

using namespace components;

CUSTOM_MAIN(context)
{
	// resolving dynamic library paths should improve over time - for now, we need to make sure we can atleast load the dummy library

	auto &fs = vfs::_default(context);

	auto search_path = strings::replace_all(SAMPLE_LIB_PATH, SOURCE_PATH, "");

	auto files = fs.enumerate_files_recursive(search_path, dl::os_library_name("vkb__dummy_sample"));
	if (files.size() != 1)
	{
		throw std::runtime_error{"failed to find dummy dynamic library"};
	}

	Sample sample;

	// this is a "hack" to make the sample library resolve relatively to the test directory
	if (!load_sample("./" + files[0], &sample))
	{
		throw std::runtime_error{"failed to load sample"};
	}

	if (!(*sample.sample_main)(context))
	{
		throw std::runtime_error{"failed to call sample_main()"};
	}

	return EXIT_SUCCESS;
}