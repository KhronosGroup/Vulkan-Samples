/* Copyright (c) 2018-2021, Arm Limited and Contributors
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

#include "vulkan_samples.h"

#include "common/logging.h"
#include "platform/platform.h"

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#	include <jni.h>

extern "C"
{
	JNIEXPORT jobjectArray JNICALL
	    Java_com_khronos_vulkan_1samples_SampleLauncherActivity_getSamples(JNIEnv *env, jobject thiz)
	{
		jclass       c             = env->FindClass("com/khronos/vulkan_samples/model/Sample");
		jmethodID    constructor   = env->GetMethodID(c, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");
		jobjectArray j_sample_list = env->NewObjectArray(sample_list.size(), c, 0);

		for (int sample_index = 0; sample_index < sample_list.size(); sample_index++)
		{
			const SampleInfo &sample_info = sample_list[sample_index];

			jstring id       = env->NewStringUTF(sample_info.id.c_str());
			jstring category = env->NewStringUTF(sample_info.category.c_str());
			jstring author   = env->NewStringUTF(sample_info.author.c_str());
			jstring name     = env->NewStringUTF(sample_info.name.c_str());
			jstring desc     = env->NewStringUTF(sample_info.description.c_str());

			jobjectArray j_tag_list = env->NewObjectArray(sample_info.tags.size(), env->FindClass("java/lang/String"), env->NewStringUTF(""));
			for (int tag_index = 0; tag_index < sample_info.tags.size(); ++tag_index)
			{
				env->SetObjectArrayElement(j_tag_list, tag_index, env->NewStringUTF(sample_info.tags[tag_index].c_str()));
			}

			env->SetObjectArrayElement(j_sample_list, sample_index, env->NewObject(c, constructor, id, category, author, name, desc, j_tag_list));
		}

		return j_sample_list;
	}
}
#endif        // VK_USE_PLATFORM_ANDROID_KHR

namespace vkb
{
namespace
{
inline void print_info()
{
	std::string col_delim(30, '-');

	LOGI("Vulkan Samples")
	LOGI("")
	LOGI("\tA collection of samples to demonstrate the Vulkan best practice.")
	LOGI("")
	LOGI("Available samples:")
	LOGI("")
	LOGI("{:20s} | {:20s} | {:20s}", "Id", "Name", "Description")
	LOGI("{}---{}---{}", col_delim.c_str(), col_delim.c_str(), col_delim.c_str())

	for (auto &sample_info : sample_list)
	{
		LOGI("{:20s} | {:20s} | {}", sample_info.id.c_str(), sample_info.name.c_str(), sample_info.description.c_str())
	}

	LOGI("")
	LOGI("Project home: https://github.com/KhronosGroup/Vulkan-Samples")
	LOGI("")
}

inline std::vector<SampleInfo>::const_iterator get_sample_info(const std::string &sample_id)
{
	return std::find_if(sample_list.begin(), sample_list.end(),
	                    [&sample_id](const SampleInfo &sample) { return sample.id == sample_id; });
}

inline const CreateAppFunc &get_create_func(const std::string &id)
{
	// Try to find the sample entry point
	const auto sample_iter = sample_create_functions.find(id);

	if (sample_iter == std::end(sample_create_functions))
	{
		// If not found, try to find the test entry point
		const auto test_iter = test_create_functions.find(id);

		if (test_iter == std::end(test_create_functions))
		{
			throw std::runtime_error("Failed to find a create function for " + id);
		}

		return test_iter->second;
	}

	return sample_iter->second;
}
}        // namespace

VulkanSamples::VulkanSamples()
{
}

bool VulkanSamples::prepare(Platform &platform)
{
	this->platform = &platform;

	auto parser = platform.get_parser();

	if (parser->contains(&Platform::samples))
	{
		print_info();
		return false;
	}

	auto result = false;

	if (parser->contains(&Platform::batch))
	{
		auto category_arg = parser->as<std::string>(&Platform::batch_categories);
		auto tags         = parser->as<std::vector<std::string>>(&Platform::batch_tags);

		// If category is all, and either no tags were given, or one of the tags is 'any', then we use the entire list
		if (category_arg == "all" && (std::find(tags.begin(), tags.end(), "any") != tags.end() || tags.empty()))
		{
			batch_mode_sample_list = sample_list;
		}
		else
		{
			std::copy_if(sample_list.begin(), sample_list.end(), std::back_inserter(batch_mode_sample_list), [category_arg, tags](const SampleInfo &sample) {
				bool category_match = category_arg == "all" || sample.category == category_arg;
				bool tag_match      = tags.empty();
				for (auto &tag : tags)
				{
					if (std::find(sample.tags.begin(), sample.tags.end(), tag) != sample.tags.end())
					{
						tag_match = true;
						break;
					}
				}

				return category_match && tag_match;
			});
		}

		if (batch_mode_sample_list.empty())
		{
			LOGE("Couldn't find any samples by the given batch mode category and tags")
			return false;
		}

		this->batch_mode_sample_iter = batch_mode_sample_list.begin();

		result = prepare_active_app(
		    sample_create_functions.at(batch_mode_sample_list.begin()->id),
		    batch_mode_sample_list.begin()->name,
		    false,
		    true);
	}
	else if (parser->contains(&Platform::sample))
	{
		const auto &sample_arg = parser->as<std::string>(&Platform::sample);

		result = prepare_active_app(
		    get_create_func(sample_arg),
		    get_sample_info(sample_arg)->name,
		    false,
		    false);
	}
	else if (parser->contains(&Platform::app))
	{
		const auto &sample_arg = parser->as<std::string>(&Platform::app);

		result = prepare_active_app(
		    get_create_func(sample_arg),
		    get_sample_info(sample_arg)->name,
		    false,
		    false);
	}
	else if (parser->contains(&Platform::test))
	{
		const auto &test_arg = parser->as<std::string>(&Platform::test);

		result = prepare_active_app(
		    get_create_func(test_arg),
		    test_arg,
		    true,
		    false);
	}
	else
	{
		// The user didn't supply any arguments so print the usage
		print_info();
		LOGI("")
		for (auto &line : parser->help())
		{
			LOGI(line)
		}
		LOGI("")
		LOGE("No arguments given, exiting")
		return false;
	}

	if (!result)
	{
		LOGE("Failed to prepare application")
	}

	return result;
}

bool VulkanSamples::prepare_active_app(CreateAppFunc create_app_func, const std::string &name, bool test, bool batch)
{
	if (active_app)
	{
		active_app->finish();
	}

	active_app.reset();
	active_app = create_app_func();
	active_app->set_name(name);

	skipped_first_frame = false;

	if (!active_app)
	{
		LOGE("Failed to create a valid vulkan app.")
		return false;
	}

	if (!test)
	{
		auto *app = active_app.get();
		if (auto *active_app = dynamic_cast<vkb::VulkanSample *>(app))
		{
			active_app->get_configuration().reset();
		}
	}

	if (batch)
	{
		this->batch_mode = true;
	}
	else if (is_benchmark_mode())
	{
		active_app->set_benchmark_mode(true);
	}

	active_app->set_headless(is_headless());

	auto result = active_app->prepare(*platform);

	if (!result)
	{
		LOGE("Failed to prepare vulkan app.")
		return result;
	}

	return result;
}

void VulkanSamples::update(float delta_time)
{
	if (active_app)
	{
		active_app->step();
	}

	elapsed_time += skipped_first_frame ? delta_time : 0.0f;
	skipped_first_frame = true;

	if (batch_mode)
	{
		// When the runtime for the current configuration is reached, advance to the next config or next sample
		if (elapsed_time >= sample_run_time_per_configuration)
		{
			elapsed_time = 0.0f;

			// Only check and advance the config if the application is a vulkan sample
			if (auto *vulkan_app = dynamic_cast<vkb::VulkanSample *>(active_app.get()))
			{
				auto &configuration = vulkan_app->get_configuration();

				if (configuration.next())
				{
					configuration.set();
					return;
				}
			}

			// Wrap it around to the start
			++batch_mode_sample_iter;
			if (batch_mode_sample_iter == batch_mode_sample_list.end())
			{
				batch_mode_sample_iter = batch_mode_sample_list.begin();
			}

			// Prepare next application
			auto result = prepare_active_app(
			    sample_create_functions.at(batch_mode_sample_iter->id),
			    batch_mode_sample_iter->name,
			    false,
			    true);

			if (!result)
			{
				LOGE("Failed to prepare vulkan sample.")
				platform->close();
			}
		}
	}
}

void VulkanSamples::finish()
{
	if (active_app)
	{
		active_app->finish();
		active_app.reset();
	}
}

void VulkanSamples::resize(const uint32_t width, const uint32_t height)
{
	if (active_app)
	{
		active_app->resize(width, height);
	}
}

void VulkanSamples::input_event(const InputEvent &input_event)
{
	if (active_app && !batch_mode && !is_benchmark_mode())
	{
		active_app->input_event(input_event);
	}
	else
	{
		Application::input_event(input_event);
	}
}

}        // namespace vkb

std::unique_ptr<vkb::Application> create_vulkan_samples()
{
	return std::make_unique<vkb::VulkanSamples>();
}
