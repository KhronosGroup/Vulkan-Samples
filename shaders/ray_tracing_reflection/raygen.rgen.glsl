/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2023 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#version 460
#extension GL_EXT_ray_tracing : enable

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 1, rgba8) uniform image2D image;
layout(set = 0, binding = 2) uniform CameraProperties
{
	mat4 viewInverse;
	mat4 projInverse;
}
cam;

struct hitPayload
{
	vec3 radiance;
	vec3 attenuation;
	int  done;
	vec3 rayOrigin;
	vec3 rayDir;
};

layout(location = 0) rayPayloadEXT hitPayload prd;

void main()
{
	const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
	const vec2 inUV        = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
	vec2       d           = inUV * 2.0 - 1.0;

	vec4 origin    = cam.viewInverse * vec4(0, 0, 0, 1);
	vec4 target    = cam.projInverse * vec4(d.x, d.y, 1, 1);
	vec4 direction = cam.viewInverse * vec4(normalize(target.xyz), 0);

	float tmin = 0.001;
	float tmax = 1e32;

	prd.rayOrigin   = origin.xyz;
	prd.rayDir      = direction.xyz;
	prd.radiance    = vec3(0.0);
	prd.attenuation = vec3(1.0);
	prd.done        = 0;

	vec3 hitValue = vec3(0);

	for (int depth = 0; depth < 64; depth++)
	{
		traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, prd.rayOrigin, tmin, prd.rayDir, tmax, 0);
		hitValue += prd.radiance;
		if (prd.done == 1 || length(prd.attenuation) < 0.1)
			break;
	}

	imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(hitValue, 0.0));
}
