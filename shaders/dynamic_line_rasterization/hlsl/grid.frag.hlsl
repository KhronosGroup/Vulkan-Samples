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
[[vk::location(0)]] float3 nearPoint : VECTOR0;
[[vk::location(1)]] float3 farPoint : VECTOR1;
[[vk::location(2)]] float4x4 view : MATRIX0;
[[vk::location(6)]] float4x4 projection : MATRIX1; 
};

float4 grid(float3 pos) {
    float2 coord = pos.xz;
    float2 derivative = fwidth(coord);
    float2 grid = abs(frac(coord - 0.5) - 0.5) / derivative;
    float _line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    float4 color = float4(0.5, 0.5, 0.5, 1.0 - min(_line, 1.0));

    if(abs(pos.x) < minimumx)
        color.y = 1;
    if(abs(pos.z) < minimumz)
        color.x = 1;
    return color;
}

float fadeFactor(float3 pos, VSOutput input) {
    float z = mul(input.projection, mul(input.view, float4(pos.xyz, 1.0))).z;
    // Empirical values are used to determine when to cut off the grid before moire patterns become visible.
    return z * 6 - 0.5;
}

float4 main(VSOutput input) : SV_TARGET0
{
    float t = -input.nearPoint.y / (input.farPoint.y - input.nearPoint.y);
    float3 pos = input.nearPoint + t * (input.farPoint - input.nearPoint);

    // Display only the lower plane
    if(t < 1.0) {
        float4 gridColor = grid(pos);
        return float4(gridColor.xyz, gridColor.w * fadeFactor(pos, input));
    }
    else {
        return float4(0.0, 0.0, 0.0, 0.0);
    }
}
