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
	JNIEnv *env;
	app->activity->vm->AttachCurrentThread(&env, nullptr);

	jclass    activity_class = env->GetObjectClass(app->activity->clazz);
	jmethodID method_id      = env->GetMethodID(activity_class, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
	jstring   arg            = env->NewStringUTF(nullptr);

	jobject file = env->CallObjectMethod(app->activity->clazz, method_id, arg);

	jclass    file_class = env->GetObjectClass(file);
	jmethodID get_path   = env->GetMethodID(file_class, "getAbsolutePath", "()Ljava/lang/String;");
	jstring   path       = (jstring) env->CallObjectMethod(file, get_path);

	const char *path_cstr = env->GetStringUTFChars(path, 0);
	std::string path_str  = std::string(path_cstr) + "/";
	env->ReleaseStringUTFChars(path, path_cstr);

	app->activity->vm->DetachCurrentThread();

	return path_str;
}

std::string get_external_cache_directory(android_app *app)
{
	JNIEnv *env;
	app->activity->vm->AttachCurrentThread(&env, nullptr);

	jclass    activity  = env->GetObjectClass(app->activity->clazz);
	jmethodID method_id = env->GetMethodID(activity, "getCacheDir", "()Ljava/io/File;");
	jobject   file      = env->CallObjectMethod(app->activity->clazz, method_id);

	jclass    file_class = env->GetObjectClass(file);
	jmethodID get_path   = env->GetMethodID(file_class, "getAbsolutePath", "()Ljava/lang/String;");
	jstring   path       = (jstring) env->CallObjectMethod(file, get_path);

	const char *path_cstr = env->GetStringUTFChars(path, 0);
	std::string path_str  = std::string(path_cstr) + "/";
	env->ReleaseStringUTFChars(path, path_cstr);

	app->activity->vm->DetachCurrentThread();

	return path_str;
}
}        // namespace details

namespace vkb
{
std::string              AndroidPlatformContext::android_external_storage_directory = {};
std::string              AndroidPlatformContext::android_temp_directory             = {};
std::vector<std::string> AndroidPlatformContext::android_arguments                  = {};

AndroidPlatformContext::AndroidPlatformContext(android_app *app) :
    PlatformContext{}, app{app}
{
	_external_storage_directory = details::get_external_storage_directory(app);
	_temp_directory             = details::get_external_cache_directory(app);
	_arguments                  = android_arguments;
}
}        // namespace vkb