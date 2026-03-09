/* Copyright (c) 2026, Sascha Willems
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

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
    int InstanceIndex;
};

[[vk::binding(0, 1)]] Texture2D textureImage[2];
[[vk::binding(0, 2)]] SamplerState textureSampler[2];

[shader("fragment")]
float4 main(VSOutput input, uniform int samplerIndex, uniform int frameIndex)
{
    return textureImage[input.InstanceIndex].Sample(textureSampler[samplerIndex], input.UV);
}