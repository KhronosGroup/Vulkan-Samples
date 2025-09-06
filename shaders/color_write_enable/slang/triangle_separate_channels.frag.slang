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
    float3 Color;
};

struct FSOutput
{
    float4 ColorR;
    float4 ColorG;
    float4 ColorB;
};

// The full color is copied to individual attachments.
// Each attachment has a single component bit (R, G, B) enabled
// via the blend_attachment in ColorWriteEnable::prepare_pipelines.
[shader("fragment")]
FSOutput main(VSOutput Input)
{
    FSOutput output;
    output.ColorR = float4(Input.Color, 1.0f);
    output.ColorG = float4(Input.Color, 1.0f);
    output.ColorB = float4(Input.Color, 1.0f);
    return output;
}
