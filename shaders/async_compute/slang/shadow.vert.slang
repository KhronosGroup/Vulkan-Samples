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

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

struct GlobalUniform
{
    column_major float4x4 model;
    column_major float4x4 view_proj;
};

[[vk::binding(1, 0)]] ConstantBuffer<GlobalUniform> global_uniform;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    float4 o_pos = mul(global_uniform.model, float4(input.position, 1.0));
    output.position = mul(global_uniform.view_proj, o_pos);
    return output;
}
