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

#define TIME_FACTOR 0.05

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    int index = int(GlobalInvocationID.x);
    float4 position = particles[index].pos;
    float4 velocity = particles[index].vel;
    position += ubo.deltaT * TIME_FACTOR * velocity;
    particles[index].pos = position;
}