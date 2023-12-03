#version 450
/* Copyright (c) 2019-2023, Sascha Willems
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
	vec4 pos;
	vec4 vel;
};

// Binding 0 : Position storage buffer
layout(std140, binding = 0) buffer Pos 
{
   Particle particles[ ];
};

layout (binding = 1) uniform UBO 
{
	float deltaT;
	int particleCount;
} ubo;

layout (constant_id = 1) const int SHARED_DATA_SIZE = 1024;
layout (constant_id = 2) const float GRAVITY = 0.002;
layout (constant_id = 3) const float POWER = 0.75;
layout (constant_id = 4) const float SOFTEN = 0.05;

layout (local_size_x_id = 0) in;

// Share data between computer shader invocations to speed up caluclations
shared vec4 sharedData[SHARED_DATA_SIZE];

#define TIME_FACTOR 0.05

void main() 
{
	// Current SSBO index
	uint index = gl_GlobalInvocationID.x;
	if (index >= ubo.particleCount) 
		return;	

	vec4 position = particles[index].pos;
	vec4 velocity = particles[index].vel;
	vec4 acceleration = vec4(0.0);

	for (int i = 0; i < ubo.particleCount; i += SHARED_DATA_SIZE)
	{
		if (i + gl_LocalInvocationID.x < ubo.particleCount)
		{
			sharedData[gl_LocalInvocationID.x] = particles[i + gl_LocalInvocationID.x].pos;
		}
		else
		{
			sharedData[gl_LocalInvocationID.x] = vec4(0.0);
		}

		barrier();

		for (int j = 0; j < gl_WorkGroupSize.x; j++)
		{
			vec4 other = sharedData[j];
			vec3 len = other.xyz - position.xyz;
			acceleration.xyz += GRAVITY * len * other.w / pow(dot(len, len) + SOFTEN, POWER);
		}

		barrier();
	}

	particles[index].vel.xyz += ubo.deltaT * TIME_FACTOR * acceleration.xyz;

	// Gradient texture position
	particles[index].vel.w += 0.1 * TIME_FACTOR * ubo.deltaT;
	if (particles[index].vel.w > 1.0)
		particles[index].vel.w -= 1.0;
}