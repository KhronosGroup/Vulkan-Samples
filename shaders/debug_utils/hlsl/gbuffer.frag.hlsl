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

Texture2D textureEnvMap : register(t1);
SamplerState samplerEnvMap : register(s1);

struct VSOutput
{
float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

struct FSOutput
{
	float4 Color0 : SV_TARGET0;
	float4 Color1 : SV_TARGET1;
};

struct PushConstants
{
	float4 offset;
	float4 color;
	int object_type;
};
[[vk::push_constant]] PushConstants push_constants;

FSOutput main(VSOutput input)
{
	FSOutput output = (FSOutput)0;
	float4 color;

	switch (push_constants.object_type) {
		case 0: // Skysphere
			{
				color = textureEnvMap.Sample(samplerEnvMap, float2(input.UV.x, 1.0 - input.UV.y));
			}
			break;

		case 1: // Phong shading
			{
				float3 ambient = push_constants.color.rgb * 0.5;
				float3 N = normalize(input.Normal);
				float3 L = normalize(input.LightVec);
				float3 V = normalize(input.ViewVec);
				float3 R = reflect(-L, N);
				float3 diffuse = max(dot(N, L), 0.0).xxx * push_constants.color.rgb;
				float3 specular = (pow(max(dot(R, V), 0.0), 8.0)).xxx;
				color = float4(ambient + diffuse + specular, 1.0);
			}
			break;
	}

	// Base color into attachment 0
	output.Color0 = float4(color.rgb, 1.0);

	// Bright parts for bloom into attachment 1
	float l = dot(output.Color0.rgb, float3(0.2126, 0.7152, 0.0722));
	float threshold = 0.75;
	output.Color1.rgb = (l > threshold) ? output.Color0.rgb : float3(0.0, 0.0, 0.0);
	output.Color1.a = 1.0;
	return output;
}