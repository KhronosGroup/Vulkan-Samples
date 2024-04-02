/* Copyright (c) 2023-2024, Mobica Limited
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

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);
 
struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
};

struct SettingsData
{
    bool color_highlight;
    int minLOD;
    int maxLOD;
};
[[vk::binding(2, 0)]]
ConstantBuffer<SettingsData> settings : register(b2);

static float3 color_blend_table[5] = 
{
	{1.00, 1.00, 1.00},
	{0.80, 0.60, 0.40},
	{0.60, 0.80, 0.60},
	{0.40, 0.60, 0.80},
	{0.20, 0.20, 0.20},
};

float4 main(VSOutput input) : SV_TARGET0
{
	float4 color = float4(0.0, 0.0, 0.0, 1.0);

	int lod = settings.minLOD;
    uint residencyCode;
	
    do
    {
        color = textureColor.SampleLevel(samplerColor, input.UV, lod, 0, residencyCode);
        lod += 1.0f;
    } while (!CheckAccessFullyMapped(residencyCode));
	
	if (settings.color_highlight)
	{
		lod -= 1;
		color.xyz = (color.xyz * color_blend_table[lod]);
	}

	return color;
}