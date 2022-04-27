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

#include "components/vfs/helpers.hpp"

namespace vfs
{
namespace helpers
{
std::string directory_path(const std::string &path)
{
	return path.substr(0, path.rfind("/"));
}

std::string strip_directory(const std::string &path)
{
	return path.substr(path.rfind("/"), path.size());
}
}        // namespace helpers
}        // namespace vfs