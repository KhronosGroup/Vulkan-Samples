#!/bin/bash
# Copyright (c) 2019-2022, Arm Limited and Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


ROOT_DIR="$(cd "$(dirname "$0")"; pwd)/../.."

if [ "$#" -ne 1 ]; then
	BUILD_DIR=$ROOT_DIR/build/android_gradle
else
	BUILD_DIR="$1"
fi

cmake -DPROJECT_NAME="vulkan_samples" \
	  -DANDROID_API=30 \
	  -DARCH_ABI="arm64-v8a;armeabi-v7a" \
	  -DANDROID_MANIFEST=$ROOT_DIR/app/android/AndroidManifest.xml \
	  -DJAVA_DIRS=$ROOT_DIR/app/android/java \
	  -DRES_DIRS=$ROOT_DIR/app/android/res \
	  -DOUTPUT_DIR=$BUILD_DIR \
	  -DASSET_DIRS="" \
	  -DJNI_LIBS_DIRS="" \
	  -DNATIVE_SCRIPT=$ROOT_DIR/CMakeLists.txt \
	  -P $ROOT_DIR/bldsys/cmake/create_gradle_project.cmake
