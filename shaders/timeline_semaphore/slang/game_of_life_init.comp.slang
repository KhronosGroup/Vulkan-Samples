/* Copyright (c) 2021-2025, Arm Limited and Contributors
 * Copyright (c) 2025, Sascha Willems
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

[[vk::image_format("rgba8")]] RWTexture2D<float4> Image;

//  Slang/HLSL do not have GLSL's (not)equal functions
bool2 equal(int2 a, int2 b) {
    return bool2(a.x == b.x, a.y == b.y);
}

bool2 notEqual(int2 a, int2 b) {
    return bool2(a.x != b.x, a.y != b.y);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 globalInvocationID: SV_DispatchThreadID)
{
    int2 index = globalInvocationID.xy;
    bool is_alive;

    // Create an arbitrary pattern which happens to create a desirable result.

    int2 mask = lerp(int2(3), int2(7), notEqual(index & 16, int2(0)));

    int2 wrapped_index = index & mask;
    if (all(equal(wrapped_index, int2(1, 0))) ||
        all(equal(wrapped_index, int2(2, 1))) ||
        all(equal(wrapped_index, int2(0, 2))) ||
        all(equal(wrapped_index, int2(1, 2))) ||
        all(equal(wrapped_index, int2(2, 2))))
    {
        is_alive = true;
    }
    else
    {
        is_alive = false;
    }

    Image[int2(globalInvocationID.xy)] = is_alive ? float4(1.0, 1.0, 1.0, 0.0) : float4(0.0);
}
