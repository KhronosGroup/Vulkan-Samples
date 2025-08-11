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

[[vk::input_attachment_index(0)]] SubpassInput positionDepthAttachment;
[[vk::input_attachment_index(1)]] SubpassInput normalAttachment;
[[vk::input_attachment_index(2)]] SubpassInput albedoAttachment;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

struct Light
{
	float4 position;
	float3 color;
	float radius;
};
// Binding 0 : Position storage buffer
StructuredBuffer<Light> lights;

float4 main(VSOutput input) : SV_TARGET
{
	// Read G-Buffer values from previous sub pass
    float3 fragPos = positionDepthAttachment.SubpassLoad().rgb;
    float3 normal = normalAttachment.SubpassLoad().rgb;
    float4 albedo = albedoAttachment.SubpassLoad();

	#define ambient 0.005

    // Ambient part
    float3 fragcolor = albedo.rgb * ambient;

    uint lightsLength;
    uint lightsStride;
    lights.GetDimensions(lightsLength, lightsStride);
	
	for(int i = 0; i < lightsLength; ++i)
    {
        float3 L = lights[i].position.xyz - fragPos;
		float dist = length(L);

		float attenuation = lights[i].radius / (pow(dist, 8.0) + 1.0);

        float NdotL = max(0.0, dot(normalize(normal), normalize(L)));
        float3 diffuse = lights[i].color * albedo.rgb * NdotL * attenuation;

		fragcolor += diffuse;
	}

    return float4(fragcolor, 1.0);
}