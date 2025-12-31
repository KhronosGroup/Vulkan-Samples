/* Copyright (c) 2025, Sascha Willems
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

[[vk::binding(1, 0)]] Sampler2D samplerHeight;
[[vk::binding(2, 0)]] Sampler2DArray samplerLayers;

struct DSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    float3 ViewVec;
    float3 LightVec;
    float3 EyePos;
    float3 WorldPos;
};

float3 sampleTerrainLayer(float2 inUV)
{
    // Define some layer ranges for sampling depending on terrain height
    float2 layers[6];
    layers[0] = float2(-10.0, 10.0);
    layers[1] = float2(5.0, 45.0);
    layers[2] = float2(45.0, 80.0);
    layers[3] = float2(75.0, 100.0);
    layers[4] = float2(95.0, 150.0);
    layers[5] = float2(140.0, 290.0);

    float3 color = float3(0.0, 0.0, 0.0);

    // Get height from displacement map
    float height = samplerHeight.SampleLevel(inUV, 0.0).r * 255.0;

    for (int i = 0; i < 6; i++)
    {
        float range = layers[i].y - layers[i].x;
        float weight = (range - abs(height - layers[i].y)) / range;
        weight = max(0.0, weight);
        color += weight * samplerLayers.Sample(float3(inUV * 16.0, i)).rgb;
    }

    return color;
}

float fog(float density, float4 FragCoord)
{
    const float LOG2 = -1.442695;
    float dist = FragCoord.z / FragCoord.w * 0.1;
    float d = density * dist;
    return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

[shader("fragment")]
float4 main(DSOutput input)
{
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 ambient = float3(0.5, 0.5, 0.5);
    float3 diffuse = max(dot(N, L), 0.0) * float3(1.0, 1.0, 1.0);

    float4 color = float4((ambient + diffuse) * sampleTerrainLayer(input.UV), 1.0);

    const float4 fogColor = float4(0.47, 0.5, 0.67, 0.0);
    return lerp(color, fogColor, fog(0.25, input.Pos));
}
