/* Copyright (c) 2026, NVIDIA CORPORATION
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

struct PSInput
{
    float4 position : SV_POSITION;
};

// Push constants for window resolution, camera, and time
struct PushConstants
{
    float2 resolution;
    float3 position;
    float3 color;
};
[[vk::push_constant]] PushConstants pushConstants;

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = (input.position.xy - 0.5 * pushConstants.resolution) / pushConstants.resolution.y;

    // Circle parameters
    float2 pos = pushConstants.position.xy;
    float radius = 0.2;

    // 2D SDF for circle
    float dist = length(uv - pos) - radius;

    if (dist > 0.0)
    {
        discard;
    }

    return float4(pushConstants.color, 1.0);
}
