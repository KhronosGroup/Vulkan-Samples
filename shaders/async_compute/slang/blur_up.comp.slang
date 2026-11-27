/* Copyright (c) 2021-2026, Arm Limited and Contributors
 * Copyright (c) 2026, Sascha Willems
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

// Upscale pass. Simple and naive :)

#include "blur_common.h"

static const float SCALE = 0.875;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 globalInvocationID : SV_DispatchThreadID)
{
    if (all(globalInvocationID.xy < registers.resolution))
    {
        float2 uv = (float2(globalInvocationID.xy) + 0.5) * registers.inv_resolution;
        float3 rgb = bloom_blur(uv, SCALE);
        out_tex[int2(globalInvocationID.xy)] = float4(rgb, 1.0);
    }
}
