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
};

Sampler2D samplerColor0;
Sampler2D samplerColor1;

[[SpecializationConstant]] const int dir = 0;

[shader("fragment")]
float4 main(VSOutput input)
{
    // From the OpenGL Super bible
    const float weights[] = {	0.0024499299678342,
                                0.0043538453346397,
                                0.0073599963704157,
                                0.0118349786570722,
                                0.0181026699707781,
                                0.0263392293891488,
                                0.0364543006660986,
                                0.0479932050577658,
                                0.0601029809166942,
                                0.0715974486241365,
                                0.0811305381519717,
                                0.0874493212267511,
                                0.0896631113333857,
                                0.0874493212267511,
                                0.0811305381519717,
                                0.0715974486241365,
                                0.0601029809166942,
                                0.0479932050577658,
                                0.0364543006660986,
                                0.0263392293891488,
                                0.0181026699707781,
                                0.0118349786570722,
                                0.0073599963704157,
                                0.0043538453346397,
                                0.0024499299678342};


    const float blurScale = 0.003;
    const float blurStrength = 1.0;

    float ar = 1.0;
    // Aspect ratio for vertical blur pass
    if (dir == 1)
    {
        float2 ts;
        samplerColor1.GetDimensions(ts.x, ts.y);
        ar = ts.y / ts.x;
    }

    float2 P = input.UV.yx - float2(0, (weights.getCount() >> 1) * ar * blurScale);

    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < weights.getCount(); i++)
    {
        float2 dv = float2(0.0, i * blurScale) * ar;
        color += samplerColor1.Sample(P + dv) * weights[i] * blurStrength;
    }

    return color;
}