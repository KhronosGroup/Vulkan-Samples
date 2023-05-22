#!/bin/bash
# Copyright (c) 2019-2023, Arm Limited and Contributors
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

if [ "$#" -lt 1 ]; then
    SAMPLE_NAME=SampleTest
else
    SAMPLE_NAME="$1"
fi

if [ "$#" -lt 2 ]; then
    CATEGORY=api
else
    CATEGORY="$2"
fi

if [ "$#" -lt 3 ]; then
    TEMPLATE_NAME=sample
else
    TEMPLATE_NAME="$3"
fi

if [ "$#" -lt 4 ]; then
    BUILD_DIR=$ROOT_DIR/samples/$CATEGORY
else
    BUILD_DIR="$4"
fi

cmake -DSAMPLE_NAME=$SAMPLE_NAME \
      -DTEMPLATE_NAME=$TEMPLATE_NAME \
      -DOUTPUT_DIR=$BUILD_DIR \
      -P $ROOT_DIR/bldsys/cmake/create_sample_project.cmake
