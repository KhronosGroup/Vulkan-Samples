/* Copyright (c) 2024, Sascha Willems
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

[[vk::binding(1, 0)]]
Texture2D textureEnvMap : register(t1);
[[vk::binding(1, 0)]]
SamplerState samplerEnvMap : register(s1);
[[vk::binding(2, 0)]]
Texture2D textureSphere : register(t2);
[[vk::binding(2, 0)]]
SamplerState samplerSphere : register(s2);

struct UBO
{
	float4x4 projection;
	float4x4 modelview;
	float4x4 skybox_modelview;
	int color_shading_rates;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo : register(b0);

struct PushConstants
{
	float4 offset;
	int object_type;
};
[[vk::push_constant]] PushConstants push_constants;

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 ViewVec : VECTOR0;
[[vk::location(3)]] float3 LightVec : VECTOR1;
};

float4 main(VSOutput input, uint shadingRate : SV_ShadingRate) : SV_TARGET
{
	float4 color;

	switch (push_constants.object_type) {
		case 0: // Skysphere			
			{
				color = textureEnvMap.Sample(samplerEnvMap, float2(input.UV.x, 1.0 - input.UV.y));
			}
			break;
		
		case 1: // Phong shading
			{
				float3 ambient = textureSphere.Sample(samplerSphere, input.UV).rgb;
				float3 N = normalize(input.Normal);
				float3 L = normalize(input.LightVec);
				float3 V = normalize(input.ViewVec);
				float3 R = reflect(-L, N);
				float3 diffuse = float3(max(dot(N, L), (0.0).xxx));
				float3 specular = float3(pow(max(dot(R, V), (0.0).xxx), 8.0));
				color = float4(ambient + diffuse + specular, 1.0);	
			}
			break;
	}

	if (ubo.color_shading_rates == 1) {
		// Visualize fragment shading rates

		const uint SHADING_RATE_1X1 = 0;
		const uint SHADING_RATE_1X2 = 0x1;
		const uint SHADING_RATE_2X1 = 0x4;
		const uint SHADING_RATE_2X2 = 0x5;
		const uint SHADING_RATE_2X4 = 0x6;
		const uint SHADING_RATE_4X2 = 0x9;
		const uint SHADING_RATE_4X4 = 0xa;

		int v = 1;
		int h = 1;
	
		if ((shadingRate == SHADING_RATE_1X2) || (shadingRate == SHADING_RATE_2X2) || (shadingRate == SHADING_RATE_4X2)) {
			v = 2;
		}
		if ((shadingRate == SHADING_RATE_2X4) || (shadingRate == SHADING_RATE_4X4)) {
			v = 4;
		}
		if ((shadingRate == SHADING_RATE_2X1) || (shadingRate == SHADING_RATE_2X2) || (shadingRate == SHADING_RATE_2X4)) {
			h = 2;
		}
		if ((shadingRate == SHADING_RATE_4X2) || (shadingRate == SHADING_RATE_4X4)) {
			h = 4;
		}			

		if (v == 1 && h == 1) {
			return float4(color.rrr * 1.0, 1.0);
		} else {
 			return float4(color.rrr * 1.0 - ((v+h) * 0.05), 1.0);
		}
	} else {
		return float4(color.rgb, 1.0);
	}

}