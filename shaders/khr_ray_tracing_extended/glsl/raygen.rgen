/* Copyright (c) 2021 Holochip Corporation
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

#version 460
#extension GL_EXT_ray_tracing : enable
#define RENDER_DEFAULT 0
#define RENDER_BARYCENTRIC 1
#define RENDER_INSTANCE_ID 2
#define RENDER_DISTANCE 3
#define RENDER_GLOBAL_XYZ 4
#define RENDER_SHADOW_MAP 5
#define RENDER_AO 6

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0, rgba8) uniform image2D image;
layout(binding = 2, set = 0) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
} cam;

struct Payload
{
  vec4 color;
  vec4 intersection; // {x, y, z, intersectionType}
  vec4 normal; // {nx, ny, nz, distance}
};


layout(location = 0) rayPayloadEXT Payload hitValue;
layout (constant_id = 0) const uint render_mode = 0;
layout (constant_id = 1) const uint maxRays = 12;

void main() 
{
	const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
	const vec2 inUV = pixelCenter/vec2(gl_LaunchSizeEXT.xy);
	vec2 d = inUV * 2.0 - 1.0;

	vec4 origin = cam.viewInverse * vec4(0,0,0,1);
	vec4 target = cam.projInverse * vec4(d.x, d.y, 1, 1) ;
	vec4 direction = cam.viewInverse*vec4(normalize(target.xyz), 0) ;

	float tmin = 0.001;
	float tmax = 10000.0;

	uint max_rays = maxRays;
	if (render_mode != RENDER_DEFAULT)
	{
		max_rays = 1;
	}

	uint object_type = 100;
	vec4 color = vec4(0, 0, 0, 0);
	// 0 = normal, 1 = shadow, 2 = AO
	uint current_mode = 0;
	float expectedDistance = -1;
	for (uint i = 0; i < max_rays && current_mode < 100 && color.a < 0.95 && (color.r < 0.99 || color.b < 0.99 || color.g < 0.99); ++i)
	{
		traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin.xyz, tmin, direction.xyz, tmax, 0);
		object_type = uint(hitValue.intersection.w);
		const vec3 object_intersection_pt = hitValue.intersection.xyz;
		const vec3 object_normal = hitValue.normal.xyz;
		if (render_mode != RENDER_DEFAULT)
		{
			color = hitValue.color;
			break;
		}
		if (object_type == 0)
		{
			vec4 newColor = hitValue.color;
			
			//shadow
			{
				const float shadow_mult = 2;
				const float shadow_scale = 0.25;
				vec3 lightPt = vec3(0, -20, 0);
				vec3 currentDirection = lightPt - hitValue.intersection.xyz;
				expectedDistance = sqrt(dot(currentDirection, currentDirection));
				currentDirection = normalize(currentDirection);
				traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, object_intersection_pt, tmin, currentDirection, tmax, 0);
				float r = expectedDistance;
				float actDistance = hitValue.normal.w;
				float scale = actDistance < expectedDistance ? shadow_scale : 1;
				scale = min(scale * shadow_mult, 1);
				newColor.xyz *= scale;
				current_mode = 101;
				if (render_mode == RENDER_SHADOW_MAP)
				{
					color = vec4(scale, scale, scale, 1);
					break;
				}
			}
			
			// ambient occlusion
			{
				const float ao_mult = 1;
				uint max_ao_each = 2;
				uint max_ao = max_ao_each * max_ao_each;
				const float max_dist = 2;
				float accumulated_ao = 0.f;
				vec3 u = abs(dot(object_normal, vec3(0, 0, 1))) > 0.9 ? cross(object_normal, vec3(1, 0, 0)) : cross(object_normal, vec3(0, 0, 1));
				vec3 v = cross(object_normal, u);
				float accumulated_factor = 0;
				for (uint j = 0; j < max_ao_each; ++j)
				{
					float phi = 0.5*(-3.14159 + 2 * 3.14159 * (float(j + 1) / float(max_ao_each + 2)));
					for (uint k = 0; k < max_ao_each; ++k){
						float theta =  0.5*(-3.14159 + 2 * 3.14159 * (float(k + 1) / float(max_ao_each + 2)));
						float x = cos(phi) * sin(theta);
						float y = sin(phi) * sin(theta);
						float z = cos(theta);
						vec3 direction = x * u + y * v + z * object_normal;
						traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, object_intersection_pt, tmin, direction, tmax, 0);
						float ao = min(hitValue.normal.w, max_dist);
						float factor = 0.2 + 0.8 * z * z;
						accumulated_factor += factor;
						accumulated_ao += ao * factor;
					}
				}
				accumulated_ao /= (max_dist * accumulated_factor);
				accumulated_ao *= accumulated_ao;
				accumulated_ao = max(min((accumulated_ao) * ao_mult, 1), 0);
				if (render_mode == RENDER_AO)
				{
					color = vec4(accumulated_ao, accumulated_ao, accumulated_ao, 1);
					break;
				}
				
				newColor.xyz *= accumulated_ao;

				const float r = max(0, 1 - color.a);
				color += r * vec4(newColor.rgb, 1);
			}
			
		} else if (object_type == 1)
		{
			origin = vec4(hitValue.intersection.xyz, 0);
			const float IOR = hitValue.color.x;
			const float max_IOR = 1.01;
			float eta = 1 / IOR;
			float c = abs(dot(object_normal, direction.xyz));
			float t = (IOR - 1) / (max_IOR - 1);
			direction = normalize((1 - t) * direction + t * (eta * direction + (eta * c - (1 - eta*eta*(1 - c*c)))));
		} else if (object_type == 2)
		{
			vec4 newColor = hitValue.color;
			float r = 1 - color.a;
			color.rgb += r * newColor.rgb * newColor.a;
			color.a += 0.1 * r * newColor.a;
			origin = vec4(hitValue.intersection.xyz, 0);
		}
	}

	imageStore(image, ivec2(gl_LaunchIDEXT.xy), color);
}
