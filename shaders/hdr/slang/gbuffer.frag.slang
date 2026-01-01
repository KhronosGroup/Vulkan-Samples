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
    float3 UVW;
    float3 Normal;
    float3 ViewVec;
    float3 LightVec;
};

struct FSOutput
{
    float4 Color0;
    float4 Color1;
};

struct UBOMatrices
{
    float4x4 projection;
    float4x4 modelview;
    float4x4 skyboxModelview;
    float4x4 inverseModelView;
    float modelscale;
};
ConstantBuffer<UBOMatrices> uboMatrices;

SamplerCube samplerEnvMap;

struct UBO {
    float exposure;
};
ConstantBuffer<UBO> ubo;

[[SpecializationConstant]] const int type = 0;

[shader("fragment")]
FSOutput main(VSOutput input)
{
    FSOutput output;
    float4 color;
    float3 wcNormal;

    switch (type) {
        case 0: // Skybox
        {
            float3 normal = normalize(input.UVW);
            color = samplerEnvMap.Sample(normal);
        }
        break;

        case 1: // Reflect
        {
            float3 wViewVec = mul((float3x3)uboMatrices.inverseModelView, normalize(input.ViewVec));
            float3 normal = normalize(input.Normal);
            float3 wNormal = mul((float3x3)uboMatrices.inverseModelView, normal);

            float NdotL = max(dot(normal, input.LightVec), 0.0);

            float3 eyeDir = normalize(input.ViewVec);
            float3 halfVec = normalize(input.LightVec + eyeDir);
            float NdotH = max(dot(normal, halfVec), 0.0);
            float NdotV = max(dot(normal, eyeDir), 0.0);
            float VdotH = max(dot(eyeDir, halfVec), 0.0);

            // Geometric attenuation
            float NH2 = 2.0 * NdotH;
            float g1 = (NH2 * NdotV) / VdotH;
            float g2 = (NH2 * NdotL) / VdotH;
            float geoAtt = min(1.0, min(g1, g2));

            const float F0 = 0.6;
            const float k = 0.2;

            // Fresnel (schlick approximation)
            float fresnel = pow(1.0 - VdotH, 5.0);
            fresnel *= (1.0 - F0);
            fresnel += F0;

            // Note: clamp to zero to mitigate any divide by zero
            float spec = max((fresnel * geoAtt) / (NdotV * NdotL * 3.14), 0.0);

            color = samplerEnvMap.Sample(reflect(-wViewVec, wNormal));

            color = float4(color.rgb * NdotL * (k + spec * (1.0 - k)), 1.0);
        }
        break;

        case 2: // Refract
        {
            float3 wViewVec = mul((float3x3)uboMatrices.inverseModelView, normalize(input.ViewVec));
            float3 wNormal = mul((float3x3)uboMatrices.inverseModelView, input.Normal);
            color = samplerEnvMap.Sample(refract(-wViewVec, wNormal, 1.0 / 1.6));
        }
        break;
    }

    // Color with manual exposure into attachment 0
    output.Color0.rgb = float3(1.0, 1.0, 1.0) - exp(-color.rgb * ubo.exposure);

    // Bright parts for bloom into attachment 1
    float l = dot(output.Color0.rgb, float3(0.2126, 0.7152, 0.0722));
    float threshold = 0.75;
    output.Color1.rgb = (l > threshold) ? output.Color0.rgb : float3(0.0, 0.0, 0.0);
    output.Color1.a = 1.0;
    return output;
}