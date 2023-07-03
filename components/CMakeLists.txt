# Copyright (c) 2023, Thomas Atkinson
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

add_subdirectory(core)

if(ANDROID)
    add_subdirectory(android)
elseif(WIN32)
    add_subdirectory(windows)
elseif(APPLE OR UNIX)
    add_subdirectory(unix)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()
