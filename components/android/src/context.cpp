/* Copyright (c) 2023, Thomas Atkinson
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

#include "android/context.hpp"

#include <jni.h>

extern "C"
{
	JNIEXPORT void JNICALL
	    Java_com_khronos_vulkan_1samples_SampleLauncherActivity_initFilePath(JNIEnv *env, jobject thiz, jstring external_dir, jstring temp_dir)
	{
		const char *external_dir_cstr                                   = env->GetStringUTFChars(external_dir, 0);
		vkb::AndroidPlatformContext::android_external_storage_directory = std::string(external_dir_cstr) + "/";
		env->ReleaseStringUTFChars(external_dir, external_dir_cstr);

		const char *temp_dir_cstr                           = env->GetStringUTFChars(temp_dir, 0);
		vkb::AndroidPlatformContext::android_temp_directory = std::string(temp_dir_cstr) + "/";
		env->ReleaseStringUTFChars(temp_dir, temp_dir_cstr);
	}

	JNIEXPORT void JNICALL
	    Java_com_khronos_vulkan_1samples_SampleLauncherActivity_sendArgumentsToPlatform(JNIEnv *env, jobject thiz, jobjectArray arg_strings)
	{
		std::vector<std::string> args;

		for (int i = 0; i < env->GetArrayLength(arg_strings); i++)
		{
			jstring arg_string = (jstring) (env->GetObjectArrayElement(arg_strings, i));

			const char *arg = env->GetStringUTFChars(arg_string, 0);

			args.push_back(std::string(arg));

			env->ReleaseStringUTFChars(arg_string, arg);
		}

		vkb::AndroidPlatformContext::android_arguments = args;
	}
}

namespace vkb
{
std::string              AndroidPlatformContext::android_external_storage_directory = {};
std::string              AndroidPlatformContext::android_temp_directory             = {};
std::vector<std::string> AndroidPlatformContext::android_arguments                  = {};

AndroidPlatformContext::AndroidPlatformContext(android_app *app) :
    PlatformContext{}, app{app}
{
	_external_storage_directory = android_external_storage_directory;
	_temp_directory             = android_temp_directory;
	_arguments                  = android_arguments;
}
}        // namespace vkb