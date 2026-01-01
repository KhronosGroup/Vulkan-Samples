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

struct Particle
{
    float4 pos;
    float4 vel;
};
RWStructuredBuffer<Particle> particles;

struct UBO
{
    float deltaT;
    int particleCount;
};
ConstantBuffer<UBO> ubo;

[[vk::constant_id(1)]] const int SHARED_DATA_SIZE = 1024;
[[vk::constant_id(2)]] const float GRAVITY = 0.002;
[[vk::constant_id(3)]] const float POWER = 0.75;
[[vk::constant_id(4)]] const float SOFTEN = 0.05;

// Share data between computer shader invocations to speed up caluclations
groupshared float4 sharedData[SHARED_DATA_SIZE];

#define TIME_FACTOR 0.05

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID, uint3 LocalInvocationID : SV_GroupThreadID)
{
    // Current SSBO index
    uint index = GlobalInvocationID.x;
    if (index >= ubo.particleCount)
        return;

    float4 position = particles[index].pos;
    float4 velocity = particles[index].vel;
    float4 acceleration = float4(0, 0, 0, 0);
    int3 workgroupSize = WorkgroupSize();

    for (int i = 0; i < ubo.particleCount; i += SHARED_DATA_SIZE)
    {
        if (i + LocalInvocationID.x < ubo.particleCount)
        {
            sharedData[LocalInvocationID.x] = particles[i + LocalInvocationID.x].pos;
        }
        else
        {
            sharedData[LocalInvocationID.x] = float4(0, 0, 0, 0);
        }

        GroupMemoryBarrierWithGroupSync();

        for (int j = 0; j < workgroupSize.x; j++)
        {
            float4 other = sharedData[j];
            float3 len = other.xyz - position.xyz;
            acceleration.xyz += GRAVITY * len * other.w / pow(dot(len, len) + SOFTEN, POWER);
        }

        GroupMemoryBarrierWithGroupSync();
    }

    particles[index].vel.xyz += ubo.deltaT * TIME_FACTOR * acceleration.xyz;

    // Gradient texture position
    particles[index].vel.w += 0.1 * TIME_FACTOR * ubo.deltaT;
    if (particles[index].vel.w > 1.0)
    {
        particles[index].vel.w -= 1.0;
    }
}