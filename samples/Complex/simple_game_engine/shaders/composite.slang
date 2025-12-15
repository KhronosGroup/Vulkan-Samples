/* Copyright (c) 2025 Holochip Corporation
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

// Fullscreen composite pass: samples the off-screen opaque color and writes to swapchain

struct VSOut {
    float4 Position : SV_POSITION;
    float2 UV       : TEXCOORD0;
};

// Export entrypoint for vertex stage
[shader("vertex")] VSOut VSMain(uint vid : SV_VertexID)
{
    // Fullscreen triangle (no vertex buffer)
    float2 pos = float2( (vid == 2) ? 3.0 : -1.0,
                         (vid == 1) ? 3.0 : -1.0 );
    float2 uv  = float2( (vid == 2) ? 2.0 : 0.0,
                         (vid == 1) ? 2.0 : 0.0 );
    VSOut o;
    o.Position = float4(pos, 0.0, 1.0);
    o.UV = uv;
    return o;
}

// Set 0, binding 0: combined image sampler for the off-screen scene color
[[vk::binding(0, 0)]] Sampler2D sceneColor;

struct Push {
    float exposure;
    float gamma;
    int   outputIsSRGB; // 1 when the color attachment is SRGB; 0 otherwise
    float _pad;         // pad to 16 bytes for push constant layout
};
[[vk::push_constant]] Push pushConsts;

float3 tonemapReinhard(float3 x)
{
    return x / (1.0 + x);
}

float3 applyExposure(float3 x, float exposure)
{
    return 1.0 - exp(-x * max(exposure, 0.0001));
}

float3 linearToGamma(float3 x, float gamma)
{
    float inv = (gamma > 0.0) ? (1.0 / gamma) : (1.0 / 2.2);
    return pow(max(x, 0.0), inv);
}

// Export entrypoint for fragment stage
[shader("fragment")] float4 PSMain(VSOut i) : SV_TARGET
{
    float4 c = sceneColor.Sample(i.UV);
    float3 color = c.rgb;
    // Simple exposure; optional reinhard if desired later
    color = applyExposure(color, pushConsts.exposure);

    // If the attachment is NOT SRGB, encode gamma here. When it is SRGB,
    // the hardware will encode at store so we keep color in linear space.
    if (pushConsts.outputIsSRGB == 0) {
        color = linearToGamma(color, pushConsts.gamma);
    }
    return float4(color, 1.0);
}
