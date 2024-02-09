/* Copyright (c) 2023-2024, Thomas Atkinson
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
	// TODO: Arguments can be parsed from the bundle
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

namespace details
{
std::string get_external_storage_directory(android_app *app)
{
	return app->activity->externalDataPath;
}

std::string get_external_cache_directory(android_app *app)
{
	JNIEnv *env;
	app->activity->vm->AttachCurrentThread(&env, NULL);

	jclass    cls         = env->FindClass("android/app/NativeActivity");
	jmethodID getCacheDir = env->GetMethodID(cls, "getCacheDir", "()Ljava/io/File;");
	jobject   cache_dir   = env->CallObjectMethod(app->activity->javaGameActivity, getCacheDir);

	jclass    fcls        = env->FindClass("java/io/File");
	jmethodID getPath     = env->GetMethodID(fcls, "getPath", "()Ljava/lang/String;");
	jstring   path_string = (jstring) env->CallObjectMethod(cache_dir, getPath);

	const char *path_chars = env->GetStringUTFChars(path_string, NULL);
	std::string temp_folder(path_chars);

	env->ReleaseStringUTFChars(path_string, path_chars);
	app->activity->vm->DetachCurrentThread();
	return temp_folder;
}
}        // namespace details

namespace vkb
{
std::vector<std::string> AndroidPlatformContext::android_arguments = {};

AndroidPlatformContext::AndroidPlatformContext(android_app *app) :
    PlatformContext{}, app{app}
{
	_external_storage_directory = details::get_external_storage_directory(app);
	_temp_directory             = details::get_external_cache_directory(app);
	_arguments                  = android_arguments;
}
}        // namespace vkb