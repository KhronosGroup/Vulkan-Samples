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
[[vk::location(0)]] float3 Color : COLOR0;
};

struct FSOutput
{
    float4 ColorR : SV_TARGET0;
    float4 ColorG : SV_TARGET1;
    float4 ColorB : SV_TARGET2;
};

// The full color is copied to individual attachments.
// Each attachment has a single component bit (R, G, B) enabled
// via the blend_attachment in ColorWriteEnable::prepare_pipelines.
FSOutput main(VSOutput Input)
{
    FSOutput output = (FSOutput)0;
    output.ColorR = float4(Input.Color, 1.0f);
    output.ColorG = float4(Input.Color, 1.0f);
    output.ColorB = float4(Input.Color, 1.0f);
    return output;
}
