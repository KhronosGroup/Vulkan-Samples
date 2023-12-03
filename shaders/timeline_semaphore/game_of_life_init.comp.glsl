/* Copyright (c) 2021-2023, Arm Limited and Contributors
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
#version 450
layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0, rgba8) writeonly uniform image2D Image;

void main()
{
    ivec2 index = ivec2(gl_GlobalInvocationID.xy);
    bool is_alive;

    // Create an arbitrary pattern which happens to create a desirable result.

    ivec2 mask = mix(ivec2(3), ivec2(7), notEqual(index & 16, ivec2(0)));

    ivec2 wrapped_index = index & mask;
    if (all(equal(wrapped_index, ivec2(1, 0))) ||
        all(equal(wrapped_index, ivec2(2, 1))) ||
        all(equal(wrapped_index, ivec2(0, 2))) ||
        all(equal(wrapped_index, ivec2(1, 2))) ||
        all(equal(wrapped_index, ivec2(2, 2))))
    {
        is_alive = true;
    }
    else
    {
        is_alive = false;
    }

    imageStore(Image, index, is_alive ? vec4(1.0, 1.0, 1.0, 0.0) : vec4(0.0));
}
