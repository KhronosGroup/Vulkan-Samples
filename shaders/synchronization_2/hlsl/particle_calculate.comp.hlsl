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

// Copyright 2020 Google LLC
// Copyright 2023 Sascha Willems

struct Particle
{
    float4 pos;
    float4 vel;
};
[[vk::binding(0, 0)]]
RWStructuredBuffer<Particle> particles : register(u0);

struct UBO
{
    float deltaT;
    int particleCount;
};
[[vk::binding(1, 0)]]
ConstantBuffer<UBO> ubo : register(b1);

// We need to define an upper bound as HLSl doesn't support variable length arrays
#define MAX_SHARED_DATA_SIZE 1024
[[vk::constant_id(1)]] const int SHARED_DATA_SIZE = 512;
[[vk::constant_id(2)]] const float GRAVITY = 0.002;
[[vk::constant_id(3)]] const float POWER = 0.75;
[[vk::constant_id(4)]] const float SOFTEN = 0.05;

// Share data between computer shader invocations to speed up caluclations
groupshared float4 sharedData[MAX_SHARED_DATA_SIZE];

#define TIME_FACTOR 0.05

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

        for (int j = 0; j < 256; j++)
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