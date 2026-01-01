/* Copyright (c) 2023-2025, Mobica Limited
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
    float3 Color;
};

[[vk::binding(1, 0)]] Sampler2D samplerColorMap;

[shader("fragment")]
float4 main(VSOutput input, float3 baryCoords: SV_Barycentrics, noperspective float3 baryCoordsAffine: SV_Barycentrics, uint vertexIndex: SV_VertexID, uniform int type)
{
    float3 vColor0 = GetAttributeAtVertex(input.Color, 0);
    float3 vColor1 = GetAttributeAtVertex(input.Color, 1);
    float3 vColor2 = GetAttributeAtVertex(input.Color, 2);

    float4 outColor = 0.0;
    switch (type) {
        case 0:
        {
            outColor.rgb = vColor0 * baryCoords.x +
                           vColor1 * baryCoords.y +
                           vColor2 * baryCoords.z;
            outColor.a = 1.0;
            break;
        }
        case 1:
        {
            outColor.rgb = baryCoords - baryCoordsAffine;
            const float exposure = 10.f;
            outColor = float4(float3(1.0) - exp(-outColor.rgb * exposure), 1.0);
            break;
        }
        case 2:
        {
            if (baryCoords.x < 0.01 || baryCoords.y < 0.01 || baryCoords.z < 0.01) 
                outColor = float4(0.0, 0.0, 0.0, 1.0);
            else
                outColor = float4(0.5, 0.5, 0.5, 1.0);
            break;
        }
        case 3:
        {
            if (baryCoords.x <= baryCoords.y && baryCoords.x <= baryCoords.z)
                outColor = float4(vColor0.rgb * baryCoords.x, 1.0);
            else if (baryCoords.y < baryCoords.x && baryCoords.y <= baryCoords.z)
                outColor = float4(vColor1.rgb * baryCoords.y, 1.0);
            else
                outColor = float4(vColor2.rgb * baryCoords.z, 1.0); 
            break;
        }
        case 4:
        {
            outColor = samplerColorMap.Sample(float2(sin(baryCoords.x) + cos(2 * baryCoords.z), sin(baryCoords.x) + cos(2 * baryCoords.y)));
        }
        }
        return outColor;
}
