/* Copyright (c) 2025, Holochip Inc.
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

struct CameraProperties
{
	float4x4 viewInverse;
	float4x4 projInverse;
	int enableSER;
	int useCoherenceHint;
	float time;
};

RaytracingAccelerationStructure topLevelAS;
RWTexture2D<float4> image;
ConstantBuffer<CameraProperties> cam;

struct RayPayload
{
	float3 hitValue;
};

[shader("raygeneration")]
void main()
{
	uint3 launchID = DispatchRaysIndex();
	uint3 launchSize = DispatchRaysDimensions();
	
	float2 pixelCenter = float2(launchID.xy) + float2(0.5, 0.5);
	float2 inUV = pixelCenter / float2(launchSize.xy);
	float2 d = inUV * 2.0 - 1.0;

	float4 origin = mul(cam.viewInverse, float4(0, 0, 0, 1));
	float4 target = mul(cam.projInverse, float4(d.x, d.y, 1, 1));
	float4 direction = mul(cam.viewInverse, float4(normalize(target.xyz), 0));

	float tmin = 0.001;
	float tmax = 10000.0;

	RayPayload payload;
	payload.hitValue = float3(0.0, 0.0, 0.0);

	if (cam.enableSER != 0)
	{
		// Shader Execution Reordering path: use hit objects
		RayDesc ray;
		ray.Origin = origin.xyz;
		ray.Direction = direction.xyz;
		ray.TMin = tmin;
		ray.TMax = tmax;

		// Step 1: Trace ray and store hit information in hit object
		HitObject hitObj = HitObject::TraceRay(
			topLevelAS,
			RAY_FLAG_NONE,
			0xff,
			0,
			0,
			0,
			ray,
			payload);

		// Step 2: Reorder invocations to improve coherency
		if (cam.useCoherenceHint != 0)
		{
			// Use a simple coherence hint based on instance ID
			// In a real path tracer, you might use material ID or other hints
			uint hint = 0;
			if (hitObj.IsHit())
			{
				hint = hitObj.GetInstanceIndex();
			}
			// Reorder with hint (using 8 bits for instance ID)
			ReorderThread(hitObj, hint, 8);
		}
		else
		{
			// Reorder without explicit hint (sorts by shader ID only)
			ReorderThread(hitObj);
		}

		// Step 3: Execute the miss or closest-hit shader
		HitObject::Invoke(topLevelAS, hitObj, payload);
	}
	else
	{
		// Traditional path: single TraceRay call (for comparison)
		RayDesc ray;
		ray.Origin = origin.xyz;
		ray.Direction = direction.xyz;
		ray.TMin = tmin;
		ray.TMax = tmax;

		TraceRay(topLevelAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);
	}

	image[launchID.xy] = float4(payload.hitValue, 0.0);
}
