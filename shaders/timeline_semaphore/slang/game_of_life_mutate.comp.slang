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

[[vk::image_format("rgba8")]] RWTexture2D<float4> ImageOutput;
[[vk::binding(0, 1)]] Sampler2D ImageInput;
 
//  Slang/HLSL do not have GLSL's (not)equal functions
bool3 notEqual(float3 a, float3 b) {
    return bool3(a.x != b.x, a.y != b.y, a.z != b.z);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 globalInvocationID: SV_DispatchThreadID, uniform float counter)
{
    float4 v = ImageInput.Load(int3(globalInvocationID.xy, 0));

    // Increase intensity over time until the cell dies.
    if (any(notEqual(v.rgb, float3(0.0))))
        v.w = max(v.w, counter);
    else
        v.w = 0.0;

    ImageOutput[globalInvocationID.xy] = v; 
}
