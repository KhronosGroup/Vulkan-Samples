/* Copyright (c) 2026, Holochip Inc.
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

// Fragment shader for displaying the computed gradient visualization

// Combined image sampler at binding 0 (texture and sampler share the same binding)
[[vk::binding(0, 0)]] Texture2D computedImage;
[[vk::binding(0, 0)]] SamplerState computedImageSampler;

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("fragment")]
float4 main(PSInput input) : SV_Target
{
    // Sample the computed gradient visualization image
    return computedImage.Sample(computedImageSampler, input.uv);
}
