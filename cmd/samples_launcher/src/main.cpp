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

#include <components/platform/dl.hpp>
#include <components/platform/platform.hpp>
#include <components/platform/sample.hpp>
#include <components/vfs/filesystem.hpp>
#include <components/vfs/helpers.hpp>

#include "config.hpp"

#include <iostream>

using namespace components;

struct AvailableSample
{
	config::Sample sample;
	std::string    library_path;
};

CUSTOM_MAIN(context)
{
	auto &fs = vfs::_default(context);

	std::cout << "loading sample configs\n";

	std::vector<std::string> files;
	if (auto err = fs.enumerate_files_recursive("/", "samples.json", &files))
	{
		std::cout << err->what() << std::endl;
		return EXIT_FAILURE;
	}

	if (files.size() == 0)
	{
		std::cout << "no samples configs found" << std::endl;
		return EXIT_SUCCESS;
	}

	for (auto &file : files)
	{
		std::cout << "candidate: " << file << "\n";
	}

	std::shared_ptr<vfs::Blob> blob;

	config::Config samples_config;

	// Attempt to load a sample config
	for (auto &file : files)
	{
		if (auto err = fs.read_file(file, &blob))
		{
			std::cout << "failed to read file\n";
			continue;
		}

		if (auto err = config::load_config_from_json(blob->binary(), &samples_config))
		{
			err->push("failed to read config", "samples_launcher/main.cpp", __LINE__);
			std::cout << err->what() << "\n";
			continue;
		}

		std::cout << "selected: " << file << "\n";

		break;
	}

	// Resolve available samples

	// consider that all samples are appended with an os shared library postfix
	std::vector<std::string> sample_files;
	if (auto err = fs.enumerate_files_recursive("/", dl::os_library_postfix(), &sample_files))
	{
		std::cout << err->what();
		return EXIT_FAILURE;
	}

	std::vector<AvailableSample> available_samples;
	available_samples.reserve(samples_config.samples.size());

	for (auto &sample : samples_config.samples)
	{
		auto library_name = dl::os_library_name(sample.library_name);

		std::cout << "looking for: " << library_name;

		std::vector<std::string> candidates;

		for (auto &file : sample_files)
		{
			if (vfs::helpers::get_file_name(file) == library_name)
			{
				// add . for cwd
				candidates.push_back("." + file);
			}
		}

		if (candidates.size() == 0)
		{
			std::cout << " - not found\n";
			continue;
		}

		std::cout << "\n";

		for (auto &candidate : candidates)
		{
			std::cout << "candidate: " << candidate << "\n";
		}

		std::cout << "selecting: " << candidates[0] << "\n";

		available_samples.push_back(AvailableSample{sample, candidates[0]});
	}

	std::cout << "\n\nAvailable Samples: (" << available_samples.size() << "/" << samples_config.samples.size() << ")\n";
	for (auto &info : available_samples)
	{
		std::cout << "\n";
		std::cout << "id:             " << info.sample.id << "\n";
		std::cout << "name:           " << info.sample.name << "\n";
		std::cout << "description:    " << info.sample.description << "\n";
		std::cout << "compile target: " << info.sample.library_name << "\n";
		std::cout << "\n";
	}

	auto args = context->arguments();

	if (args.size() == 0)
	{
		std::cout << "no sample select\n";
		return EXIT_SUCCESS;
	}

	for (auto &info : available_samples)
	{
		if (info.sample.id == args[0])
		{
			Sample sample;
			if (load_sample(info.library_path, &sample))
			{
				(*sample.sample_main)(context);

				break;
			}
			else
			{
				std::cout << "failed to load sample\n";
			}
		}
	}

	return EXIT_SUCCESS;
}