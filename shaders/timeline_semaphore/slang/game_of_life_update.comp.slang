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
void main(uint3 globalInvocationID: SV_DispatchThreadID)
{
    float2 textureSize;
    ImageInput.GetDimensions(textureSize.x, textureSize.y);

    int2 index = int2(globalInvocationID.xy);
    float2 uv = (float2(index) + 0.5) / textureSize;
    int neighbors = 0;

    float4 self = ImageInput.SampleLevel(uv, 0.0);
    bool is_alive = any(notEqual(self.rgb, float3(0.0)));
    float3 total = self.rgb;

#define CHECK_OFFSET(x, y) { \
    float3 tmp; \
    tmp = ImageInput.SampleLevel(uv, 0.0, int2(x, y)).rgb; \
    if (any(notEqual(tmp, float3(0.0)))) { \
        neighbors++; \
        total += tmp.rgb; \
    } \
}
    CHECK_OFFSET(-1, -1)
    CHECK_OFFSET( 0, -1)
    CHECK_OFFSET(+1, -1)
    CHECK_OFFSET(-1,  0)
    CHECK_OFFSET(+1,  0)
    CHECK_OFFSET(-1, +1)
    CHECK_OFFSET( 0, +1)
    CHECK_OFFSET(+1, +1)

    if (is_alive)
    {
        is_alive = neighbors == 2u || neighbors == 3u;
        if (is_alive)
        {
            total /= float(neighbors);
        }
        else
        {
            total = float3(0.0);
        }
    }
    else
    {
        is_alive = neighbors == 3u;
        if (is_alive)
        {
            float3 fresh_color = float3(uv.x, uv.y, 1.0 - uv.x - uv.y);
            total = fresh_color;
        }
        else
        {
            total = float3(0.0);
        }
    }

    ImageOutput[int2(globalInvocationID.xy)] = float4(total, 0.0);
}
