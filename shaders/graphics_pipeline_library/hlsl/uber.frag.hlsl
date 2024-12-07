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

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float3 Normal : NORMAL0;
    [[vk::location(1)]] float3 Color : COLOR0;
    [[vk::location(2)]] float3 ViewVec : VECTOR0;
    [[vk::location(3)]] float3 LightVec : VECTOR1;
    [[vk::location(4)]] nointerpolation float3 FlatNormal : NORMAL1;
};

[[vk::constant_id(0)]] const int LIGHTING_MODEL = 0;

float4 main(VSOutput input) : SV_TARGET0
{
	switch (LIGHTING_MODEL) {
		case 0: // Phong			
		{
			float3 ambient = input.Color * (0.25).xxx;
			float3 N = normalize(input.Normal);
			float3 L = normalize(input.LightVec);
			float3 V = normalize(input.ViewVec);
			float3 R = reflect(-L, N);
			float3 diffuse = max(dot(N, L), (0.0).xxx) * input.Color;
			float3 specular = pow(max(dot(R, V), (0.0).xxx), 32.0) * (0.75).xxx;
			return float4(ambient + diffuse * 1.75 + specular, 1.0);		
		}
		case 1: // Toon
		{

			float3 N = normalize(input.Normal);
			float3 L = normalize(input.LightVec);
			float intensity = dot(N,L);
			float3 color;
			if (intensity > 0.98)
				color = input.Color * 1.5;
			else if  (intensity > 0.9)
				color = input.Color * 1.0;
			else if (intensity > 0.5)
				color = input.Color * 0.6;
			else if (intensity > 0.25)
				color = input.Color * 0.4;
			else
				color = input.Color * 0.2;
			// Desaturate a bit
			color = float3(lerp(color, float3(dot(float3(0.2126,0.7152,0.0722), color).xxx), 0.25));
			return float4(color, 1.0);
		}
	}
	return float4(input.Color, 1.0);
}