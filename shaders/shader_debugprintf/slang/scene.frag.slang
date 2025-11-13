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

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
    float3 Normal;
    float3 ViewVec;
    float3 LightVec;
};

[[vk::binding(1, 0)]] Sampler2D samplerEnvMap;

[shader("fragment")]
float4 main(VSOutput input, uniform float4 offset, uniform float4 color, uniform int object_type)
{
    float4 out_color;

    switch (object_type) {
        case 0: // Skysphere
        {
            out_color = samplerEnvMap.Sample(float2(input.UV.x, 1.0 - input.UV.y));
        }
        break;
        
        case 1: // Phong shaded objects
        {
            float3 ambient = color.rgb * (0.5).xxx;
            float3 N = normalize(input.Normal);
            float3 L = normalize(input.LightVec);
            float3 V = normalize(input.ViewVec);
            float3 R = reflect(-L, N);
            float3 diffuse = max(dot(N, L), 0.0) * color.rgb;
            float3 specular = (pow(max(dot(R, V), 0.0), 8.0)).xxx;
            out_color = float4(ambient + diffuse + specular, 1.0);	
        }
        break;
    }

    return float4(out_color.rgb, 1.0);
}