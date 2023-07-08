/* Copyright (c) 2023, Holochip Corporation
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

// Example of the data shared with its associated mesh shader:
// Note that all of these could be removed.  One can pass positionTransformation, subDimension, and cullRadius from a UBO.
// however, in the interests of demonstrating how to use shared data between task and mesh shaders.

// Number of task shader invocations per task shader workgroup
const uint numTaskInvocationsX = 2;
const uint numTaskInvocationsY = 2;

const uint numMeshInvocationsX = 2;
const uint numMeshInvocationsY = 2;

struct SharedData
{
	vec2 position;
	vec2 offsets[numTaskInvocationsX * numTaskInvocationsY];
	vec2 size;
};
