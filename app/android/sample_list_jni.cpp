/* Copyright (c) 2019-2026, Arm Limited and Contributors
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

// Lists the available samples to the Android launcher activity.
//
// This lives in the app, rather than in the framework, because it needs the generated sample registry (apps.h), and the
// framework must not depend on the registry.

#include <jni.h>

#include "apps.h"

extern "C"
{
	JNIEXPORT jobjectArray JNICALL
	    Java_com_khronos_vulkan_1samples_SampleLauncherActivity_getSamples(JNIEnv *env, jobject thiz)
	{
		auto sample_list = apps::get_samples();

		jclass       c             = env->FindClass("com/khronos/vulkan_samples/model/Sample");
		jmethodID    constructor   = env->GetMethodID(c, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");
		jobjectArray j_sample_list = env->NewObjectArray(sample_list.size(), c, 0);

		for (int sample_index = 0; sample_index < sample_list.size(); sample_index++)
		{
			const apps::SampleInfo *sample_info = reinterpret_cast<apps::SampleInfo *>(sample_list[sample_index]);

			jstring id       = env->NewStringUTF(sample_info->id.c_str());
			jstring category = env->NewStringUTF(sample_info->category.c_str());
			jstring author   = env->NewStringUTF(sample_info->author.c_str());
			jstring name     = env->NewStringUTF(sample_info->name.c_str());
			jstring desc     = env->NewStringUTF(sample_info->description.c_str());

			jobjectArray j_tag_list = env->NewObjectArray(sample_info->tags.size(), env->FindClass("java/lang/String"), env->NewStringUTF(""));
			for (int tag_index = 0; tag_index < sample_info->tags.size(); ++tag_index)
			{
				env->SetObjectArrayElement(j_tag_list, tag_index, env->NewStringUTF(sample_info->tags[tag_index].c_str()));
			}

			env->SetObjectArrayElement(j_sample_list, sample_index, env->NewObject(c, constructor, id, category, author, name, desc, j_tag_list));
		}

		return j_sample_list;
	}
}
