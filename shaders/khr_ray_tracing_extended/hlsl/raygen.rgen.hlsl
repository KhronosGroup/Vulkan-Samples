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

#define RENDER_DEFAULT 0
#define RENDER_BARYCENTRIC 1
#define RENDER_INSTANCE_ID 2
#define RENDER_DISTANCE 3
#define RENDER_GLOBAL_XYZ 4
#define RENDER_SHADOW_MAP 5
#define RENDER_AO 6

RaytracingAccelerationStructure rs : register(t0);
RWTexture2D<float4> image : register(u1);

struct CameraProperties
{
    float4x4 viewInverse;
    float4x4 projInverse;
};
[[vk::binding(2, 0)]]
ConstantBuffer<CameraProperties> cam : register(b2);

struct Payload
{
[[vk::location(0)]] float4 color;
[[vk::location(1)]] float4 intersection; // {x, y, z, intersectionType}
[[vk::location(2)]] float4 normal; // {nx, ny, nz, distance}
};

[[vk::constant_id(0)]] const int render_mode = 0;
[[vk::constant_id(1)]] const int maxRays = 12;

[shader("raygeneration")]
void main()
{
    uint3 LaunchID = DispatchRaysIndex();
    uint3 LaunchSize = DispatchRaysDimensions();

	const float2 pixelCenter = float2(LaunchID.xy) + float2(0.5, 0.5);
	const float2 inUV = pixelCenter/float2(LaunchSize.xy);
	float2 d = inUV * 2.0 - 1.0;

	float4 origin = mul(cam.viewInverse, float4(0,0,0,1));
	float4 target = mul(cam.projInverse, float4(d.x, d.y, 1, 1));
	float4 direction = mul(cam.viewInverse, float4(normalize(target.xyz), 0));

	float tmin = 0.001;
	float tmax = 10000.0;

	uint max_rays = maxRays;
	if (render_mode != RENDER_DEFAULT)
	{
		max_rays = 1;
	}

	uint object_type = 100;
	float4 color = float4(0, 0, 0, 0);
	// 0 = normal, 1 = shadow, 2 = AO
	uint current_mode = 0;
	float expectedDistance = -1;

    RayDesc rayDesc;
    rayDesc.TMin = tmin;
    rayDesc.TMax = tmax;

    Payload hitValue;

	for (uint i = 0; i < max_rays && current_mode < 100 && color.a < 0.95 && (color.r < 0.99 || color.b < 0.99 || color.g < 0.99); ++i)
	{
        rayDesc.Origin = origin.xyz;
        rayDesc.Direction = direction.xyz;
    	TraceRay(rs, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, hitValue);
		object_type = uint(hitValue.intersection.w);
		const float3 object_intersection_pt = hitValue.intersection.xyz;
		const float3 object_normal = hitValue.normal.xyz;
		if (render_mode != RENDER_DEFAULT)
		{
			color = hitValue.color;
			break;
		}
		if (object_type == 0)
		{
			float4 newColor = hitValue.color;
			
			//shadow
			{
				const float shadow_mult = 2;
				const float shadow_scale = 0.25;
				float3 lightPt = float3(0, -20, 0);
				float3 currentDirection = lightPt - hitValue.intersection.xyz;
				expectedDistance = sqrt(dot(currentDirection, currentDirection));
				currentDirection = normalize(currentDirection);
				rayDesc.Origin = object_intersection_pt;
				rayDesc.Direction = currentDirection;
    			TraceRay(rs, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, hitValue);
				float r = expectedDistance;
				float actDistance = hitValue.normal.w;
				float scale = actDistance < expectedDistance ? shadow_scale : 1;
				scale = min(scale * shadow_mult, 1);
				newColor.xyz *= scale;
				current_mode = 101;
				if (render_mode == RENDER_SHADOW_MAP)
				{
					color = float4(scale, scale, scale, 1);
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
				float3 u = abs(dot(object_normal, float3(0, 0, 1))) > 0.9 ? cross(object_normal, float3(1, 0, 0)) : cross(object_normal, float3(0, 0, 1));
				float3 v = cross(object_normal, u);
				float accumulated_factor = 0;
				for (uint j = 0; j < max_ao_each; ++j)
				{
					float phi = 0.5*(-3.14159 + 2 * 3.14159 * (float(j + 1) / float(max_ao_each + 2)));
					for (uint k = 0; k < max_ao_each; ++k){
						float theta =  0.5*(-3.14159 + 2 * 3.14159 * (float(k + 1) / float(max_ao_each + 2)));
						float x = cos(phi) * sin(theta);
						float y = sin(phi) * sin(theta);
						float z = cos(theta);
						float3 direction = x * u + y * v + z * object_normal;
						rayDesc.Origin = object_intersection_pt;
						rayDesc.Direction = direction;
    					TraceRay(rs, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, hitValue);
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
					color = float4(accumulated_ao, accumulated_ao, accumulated_ao, 1);
					break;
				}
				
				newColor.xyz *= accumulated_ao;

				const float r = max(0, 1 - color.a);
				color += r * float4(newColor.rgb, 1);
			}
			
		} else if (object_type == 1)
		{
			origin = float4(hitValue.intersection.xyz, 0);
			const float IOR = hitValue.color.x;
			const float max_IOR = 1.01;
			float eta = 1 / IOR;
			float c = abs(dot(object_normal, direction.xyz));
			float t = (IOR - 1) / (max_IOR - 1);
			direction = normalize((1 - t) * direction + t * (eta * direction + (eta * c - (1 - eta*eta*(1 - c*c)))));
		} else if (object_type == 2)
		{
			float4 newColor = hitValue.color;
			float r = 1 - color.a;
			color.rgb += r * newColor.rgb * newColor.a;
			color.a += 0.1 * r * newColor.a;
			origin = float4(hitValue.intersection.xyz, 0);
		}
	}

	image[int2(LaunchID.xy)] = color;
}