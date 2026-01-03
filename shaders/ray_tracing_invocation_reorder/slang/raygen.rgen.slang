/* Copyright (c) 2025-2026, Holochip Inc.
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

struct CameraProperties
{
    float4x4 viewInverse;
    float4x4 projInverse;
    int enableSER;
    int useCoherenceHint;
    float time;
};

struct Payload
{
    float4 color;
    float4 intersection; // {x, y, z, intersectionType}
    float4 normal; // {nx, ny, nz, distance}
};

RaytracingAccelerationStructure topLevelAS;
RWTexture2D<float4> image;
ConstantBuffer<CameraProperties> cam;

[[vk::constant_id(0)]] const uint render_mode = RENDER_DEFAULT;
[[vk::constant_id(1)]] const uint maxRays = 12;

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

    uint max_rays = maxRays;
    if (render_mode != RENDER_DEFAULT)
    {
        max_rays = 1;
    }

    uint object_type = 100;
    float4 color = float4(0, 0, 0, 0);
    uint current_mode = 0;
    float expectedDistance = -1;

    Payload payload;
    payload.color = float4(0, 0, 0, 0);
    payload.intersection = float4(0, 0, 0, 0);
    payload.normal = float4(0, 0, 0, 0);

    // Loop to trace through multiple objects (flame particles, refractive surfaces)
    // Similar to ray_tracing_extended - continues until alpha is saturated or max rays reached
    for (uint rayIndex = 0; rayIndex < max_rays && current_mode < 100 && color.a < 0.95; ++rayIndex)
    {
        // Primary ray - when SER is enabled, reorder with hit objects for the first trace
        if (cam.enableSER != 0 && rayIndex == 0)
        {
            RayDesc ray;
            ray.Origin = origin.xyz;
            ray.Direction = direction.xyz;
            ray.TMin = tmin;
            ray.TMax = tmax;

            HitObject hitObj = HitObject::TraceRay(
                topLevelAS,
                RAY_FLAG_NONE,
                0xff,
                0,
                0,
                0,
                ray,
                payload);

            if (cam.useCoherenceHint != 0)
            {
                uint hint = 0;
                if (hitObj.IsHit())
                {
                    hint = hitObj.GetInstanceIndex();
                }
                ReorderThread(hitObj, hint, 8);
            }
            else
            {
                ReorderThread(hitObj);
            }

            HitObject::Invoke(topLevelAS, hitObj, payload);
            object_type = uint(payload.intersection.w);
        }
        else
        {
            RayDesc ray;
            ray.Origin = origin.xyz;
            ray.Direction = direction.xyz;
            ray.TMin = tmin;
            ray.TMax = tmax;

            TraceRay(topLevelAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);
            object_type = uint(payload.intersection.w);
        }

        // Secondary rays (shadow and AO) - apply SER for better coherence
        float3 object_intersection_pt = payload.intersection.xyz;
        float3 object_normal = payload.normal.xyz;
        
        if (render_mode != RENDER_DEFAULT)
        {
            color = payload.color;
            break;
        }
        else if (object_type == 0)
        {
            float4 newColor = payload.color;
            // shadow
            {
                const float shadow_mult = 2;
                const float shadow_scale = 0.25;
                float3 lightPt = float3(0, -20, 0);
                float3 currentDirection = lightPt - payload.intersection.xyz;
                float expectedDistance = sqrt(dot(currentDirection, currentDirection));
                currentDirection = normalize(currentDirection);
                
                RayDesc shadowRay;
                shadowRay.Origin = object_intersection_pt;
                shadowRay.Direction = currentDirection;
                shadowRay.TMin = tmin;
                shadowRay.TMax = tmax;

                TraceRay(topLevelAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, shadowRay, payload);
                
                float actDistance = payload.normal.w;
                float scale = actDistance < expectedDistance ? shadow_scale : 1.0;
                scale = min(scale * shadow_mult, 1.0);
                newColor.xyz *= scale;
                current_mode = 101;
                if (render_mode == RENDER_SHADOW_MAP)
                {
                    image[launchID.xy] = float4(scale, scale, scale, 1);
                    return;
                }
            }
            // ambient occlusion - standard TraceRay (SER overhead in loops is too high)
            {
                const float ao_mult = 1;
                uint max_ao_each = 2;  // 2x2=4 AO rays
                const float max_dist = 2;
                float accumulated_ao = 0.0;
                float3 u = abs(dot(object_normal, float3(0, 0, 1))) > 0.9 ? cross(object_normal, float3(1, 0, 0)) : cross(object_normal, float3(0, 0, 1));
                float3 v = cross(object_normal, u);
                float accumulated_factor = 0;
                for (uint j = 0; j < max_ao_each; ++j)
                {
                    float phi = 0.5 * (-3.14159 + 2 * 3.14159 * (float(j + 1) / float(max_ao_each + 2)));
                    for (uint k = 0; k < max_ao_each; ++k)
                    {
                        float theta = 0.5 * (-3.14159 + 2 * 3.14159 * (float(k + 1) / float(max_ao_each + 2)));
                        float x = cos(phi) * sin(theta);
                        float y = sin(phi) * sin(theta);
                        float z = cos(theta);
                        float3 dir2 = x * u + y * v + z * object_normal;
                        
                        RayDesc aoRay;
                        aoRay.Origin = object_intersection_pt;
                        aoRay.Direction = dir2;
                        aoRay.TMin = tmin;
                        aoRay.TMax = tmax;
                        
                        // AO rays use standard TraceRay - SER overhead in loops is too high
                        TraceRay(topLevelAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, aoRay, payload);
                        
                        float ao = min(payload.normal.w, max_dist);
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
                    image[launchID.xy] = float4(accumulated_ao, accumulated_ao, accumulated_ao, 1);
                    return;
                }
                
                newColor.xyz *= accumulated_ao;
                const float r = max(0, 1 - color.a);
                color += r * float4(newColor.rgb, 1);
            }
        }
        else if (object_type == 1)
        {
            // Refractive / reflective path - continue ray through the surface
            origin = float4(payload.intersection.xyz, 0);
            float IOR = payload.color.x;
            float max_IOR = 1.01;
            float eta = 1 / IOR;
            float c = abs(dot(object_normal, direction.xyz));
            float t = (IOR - 1) / (max_IOR - 1);
            direction = float4(normalize((1 - t) * direction.xyz + t * (eta * direction.xyz + (eta * c - sqrt(1 - eta*eta*(1 - c*c))) * object_normal)), 0);
        }
        else if (object_type == 2)
        {
            // Flame particle with alpha blending - continue ray through particle
            float4 newColor = payload.color;
            float r = 1 - color.a;
            color.rgb += r * newColor.rgb * newColor.a;
            color.a += 0.1 * r * newColor.a;
            origin = float4(payload.intersection.xyz, 0);
        }
        else
        {
            // Unknown object type or miss - stop tracing
            break;
        }
    }

    image[launchID.xy] = color;
}
