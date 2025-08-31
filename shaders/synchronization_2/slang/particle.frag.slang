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

Sampler2D samplerColorMap;
Sampler2D samplerGradientRamp;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float GradientPos;
    float2 CenterPos;
    float PSize : SV_PointSize;
    // remove
    float PointSize;
};

[shader("fragment")]
float4 main(VSOutput input)
{
    float3 color = samplerGradientRamp.Sample(float2(input.GradientPos, 0.0)).rgb;
    float2 PointCoord = (input.Pos.xy - input.CenterPos.xy) / input.PointSize + 0.5;
    return float4(samplerColorMap.Sample(PointCoord).rgb * color, 1);
}