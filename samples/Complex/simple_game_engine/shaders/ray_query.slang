/* Copyright (c) 2025 Holochip Corporation
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
// Ray query compute shader for ray-traced rendering
// Uses Slang's ray query extension as an alternative to rasterization

import common_types;
import pbr_utils;
import lighting_utils;
import tonemapping_utils;

// GPU data structures for proper geometry and material access
struct GeometryInfo {
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
    uint vertexCount;
    uint materialIndex;
    uint indexCount;   // number of indices in the index buffer
    uint _pad0;
    // Instance -> world normal transform (3 columns; xyz used, w unused)
    float4 normalMatrix0;
    float4 normalMatrix1;
    float4 normalMatrix2;
};

struct MaterialData {
    float3 albedo;
    float metallic;
    float3 emissive;
    float roughness;
    float ao;
    float ior;
    float emissiveStrength;
    float alpha;
    float transmissionFactor;
    float alphaCutoff;
    int alphaMode; // 0=OPAQUE, 1=MASK, 2=BLEND (matches glTF)
    uint isGlass;
    uint isLiquid;

    // Raster parity: texture-set flags (-1 = no texture; 0 = sample from texture table)
    int baseColorTextureSet;
    int physicalDescriptorTextureSet;
    int normalTextureSet;
    int occlusionTextureSet;
    int emissiveTextureSet;

    // Ray Query texture table indices (binding 6)
    int baseColorTexIndex;
    int normalTexIndex;
    int physicalTexIndex;
    int occlusionTexIndex;
    int emissiveTexIndex;

    // Specular-glossiness workflow support
    int useSpecGlossWorkflow;
    float glossinessFactor;
    float3 specularFactor;
    int hasEmissiveStrengthExt;
    uint _padMat[3];
};

// C++ Vertex structure layout (tightly packed, 48 bytes total):
// - position: vec3 at offset 0 (12 bytes)
// - normal: vec3 at offset 12 (12 bytes)  
// - texCoord: vec2 at offset 24 (8 bytes)
// - tangent: vec4 at offset 32 (16 bytes)
// We'll read normals directly as floats to avoid PhysicalStorageBuffer alignment issues

// Ray Query uses a dedicated uniform layout to avoid CPU↔shader drift.
// IMPORTANT: This must match `RayQueryUniformBufferObject` in `renderer.h`.
struct RayQueryUniforms {
    [[vk::offset(0)]]   float4x4 model;
    [[vk::offset(64)]]  float4x4 view;
    [[vk::offset(128)]] float4x4 proj;
    [[vk::offset(192)]] float4 camPos;

    [[vk::offset(208)]] float exposure;
    [[vk::offset(212)]] float gamma;
    [[vk::offset(216)]] float scaleIBLAmbient;
    [[vk::offset(220)]] int lightCount;
    [[vk::offset(224)]] int enableRayQueryReflections;
    [[vk::offset(228)]] int enableRayQueryTransparency;

    [[vk::offset(232)]] float2 screenDimensions;
    [[vk::offset(240)]] int geometryInfoCount;
    [[vk::offset(244)]] int materialCount;
    [[vk::offset(248)]] int _pad0;
};

// Compute shader descriptor bindings
[[vk::binding(0, 0)]] ConstantBuffer<RayQueryUniforms> ubo;
[[vk::binding(1, 0)]] RaytracingAccelerationStructure tlas;  // Top-level acceleration structure
[[vk::binding(2, 0)]] RWTexture2D<float4> outputImage;       // Output render target
[[vk::binding(3, 0)]] StructuredBuffer<LightData> lightBuffer;
[[vk::binding(4, 0)]] StructuredBuffer<GeometryInfo> geometryInfoBuffer;  // Geometry vertex/index addresses
[[vk::binding(5, 0)]] StructuredBuffer<MaterialData> materialBuffer;      // Material properties
// Fixed-size Ray Query texture table (combined image samplers)
// Must match Renderer::RQ_MAX_TEX in C++ (currently 2048)
static const uint RQ_MAX_TEX = 2048;
[[vk::binding(6, 0)]] Sampler2D baseColorTex[RQ_MAX_TEX];

// (No debug buffer in production layout)

// (No debug toggles in production)

// NOTE: Ray Query debugPrintf/printf diagnostics are intentionally removed.
// Keep Ray Query VVL-clean and log-noise-free by default.

float3 materialDebugColor(uint materialIndex) {
    float u = float(materialIndex + 1);
    float3 h = frac(float3(0.06711056, 0.00583715, 0.2065) * u);
    return frac(h * 2.6180339);
}

// Simple ray-scene intersection result
struct HitInfo {
    bool hit;
    float t;
    float3 worldPos;
    float3 normal;
    float3 baseColor;    // base color (factor * texture), not lit
    float3 color;
    float3 F0;
    float roughness;
    float metallic;
    float transmission;  // Added for glass/transparency support
    float ior;
    bool isGlass;        // Material is glass (should always have reflections/transparency)
    bool isLiquid;
    int alphaMode;       // 0=OPAQUE, 1=MASK, 2=BLEND
    float opacity;       // 0..1, derived from baseColor alpha (factor * texture)
    uint instanceId;     // Committed instance ID (for debug coloring)
    float2 uv;           // Interpolated UV at the hit (glTF V flip already applied)
    uint texIndex;       // Resolved baseColor texture index used for sampling (for stats/debug)
    uint materialIndex;  // Resolved/clamped material index used
};

static const float RQ_RAY_EPS = 0.001;

float3 skyColor(float3 dir) {
    float t = saturate(0.5 * (dir.y + 1.0));
    float3 a = float3(0.06, 0.06, 0.09);
    float3 b = float3(0.20, 0.25, 0.32);
    return lerp(a, b, t);
}

float3 shadeWithSecondaryRays(float3 rayOrigin, float3 rayDir, HitInfo hit) {
    float3 base = max(hit.color, 0.0);
    int maxBounces = clamp(ubo._pad0, 0, 10);
    if (maxBounces <= 0) {
        return base;
    }

    float3 N = hit.normal;
    float3 V = normalize(-rayDir);
    float NdotV = abs(dot(N, V));

    // Fresnel for reflection weighting
    float3 F = FresnelSchlick(NdotV, hit.F0);
    if (hit.transmission > 0.01 || hit.isGlass) {
        F = Fresnel_Dielectric(NdotV, hit.ior);
    }
    float Fr = saturate((F.r + F.g + F.b) * (1.0 / 3.0));

    // Transmission gate
    float opacity = clamp(hit.opacity, 0.0, 1.0);
    float blendTransmission = (hit.alphaMode == 2) ? (1.0 - opacity) : 0.0;
    float physicalTransmission = clamp(hit.transmission, 0.0, 1.0);
    float T = 0.0;
    if (ubo.enableRayQueryTransparency != 0 && !hit.isLiquid) {
        T = max(physicalTransmission, blendTransmission);

        // Engine glass materials should always show visible transmission even if the source asset
        // doesn’t author KHR_materials_transmission.
        if (hit.isGlass) {
            T = max(T, 0.85);
        }
    }

    // Reflection ray (chain up to maxBounces)
    float3 reflCol = float3(0.0, 0.0, 0.0);
    bool doRefl = (ubo.enableRayQueryReflections != 0) && (Fr > 1e-4);
    if (doRefl) {
        float3 ro = hit.worldPos;
        float3 rd = normalize(reflect(rayDir, N));
        // Bias along the secondary ray direction to avoid self-intersection.
        ro += rd * RQ_RAY_EPS;

        float3 last = skyColor(rd);
        for (int b = 0; b < maxBounces; ++b) {
            HitInfo rh = traceRay(ro, rd, RQ_RAY_EPS, 10000.0);
            last = rh.hit ? max(rh.color, 0.0) : skyColor(rd);
            if (!rh.hit) {
                break;
            }
            // Next bounce
            float3 Nr = normalize(rh.normal);
            rd = normalize(reflect(rd, Nr));
            ro = rh.worldPos + rd * RQ_RAY_EPS;
        }
        reflCol = last;
    }

    // Transmission ray:
    // - Physical transmission / glass: thin refraction ray
    // - Alpha BLEND with no physical transmission: straight-through ray (no refraction)
    float3 thruCol = float3(0.0, 0.0, 0.0);
    bool doThru = (T > 1e-4);
    if (doThru) {
        float3 thruDir = rayDir;

        if (physicalTransmission > 1e-4 || hit.isGlass) {
            // Thin refraction for glass/transmission
            float3 Nn = N;
            float eta = 1.0 / max(hit.ior, 1.0);
            // Determine if we're entering or exiting based on ray direction vs normal
            if (dot(rayDir, N) > 0.0) {
                Nn = -N;
                eta = max(hit.ior, 1.0);
            }
            float3 refrDir;
            if (refract(rayDir, Nn, eta, refrDir)) {
                thruDir = normalize(refrDir);
            } else {
                // Total internal reflection
                doThru = false;
            }
        }

        if (doThru) {
            HitInfo th = traceRay(hit.worldPos + thruDir * RQ_RAY_EPS, thruDir, RQ_RAY_EPS, 10000.0);
            thruCol = th.hit ? max(th.color, 0.0) : skyColor(thruDir);

            // Use baseColor as a transmission tint for glass-like materials.
            // (This is a simplified thin-glass model, not volumetric absorption.)
            if (hit.isGlass || physicalTransmission > 1e-4) {
                thruCol *= clamp(hit.baseColor, 0.0, 1.0);
            }
        }
    }

    if (T > 1e-4) {
        // Transmissive: mix transmitted view with Fresnel reflection.
        float3 trMix = lerp(thruCol, reflCol, Fr);

        // For alpha BLEND, composite as standard alpha blending (opacity = coverage).
        if (hit.alphaMode == 2 && physicalTransmission <= 1e-4 && !hit.isGlass) {
            return base * opacity + trMix * (1.0 - opacity);
        }

        // For glass/transmission, avoid keeping an opaque diffuse base.
        if (hit.isGlass || physicalTransmission > 1e-4) {
            return trMix;
        }

        // Fallback: interpolate when we only have a heuristic transmission signal.
        return lerp(base, trMix, T);
    }

    // Opaque: add a controlled reflection contribution (avoids double-counting too much)
    float reflWeight = Fr * (1.0 - clamp(hit.roughness, 0.0, 1.0));
    return lerp(base, reflCol, reflWeight);
}

float2 computeTriangleUV(uint instIndex, uint primitiveIndex, float2 bary) {
    float3 barycentrics = float3(1.0 - bary.x - bary.y, bary.x, bary.y);
    if (instIndex >= uint(max(0, ubo.geometryInfoCount))) return float2(0.0, 0.0);
    GeometryInfo geoInfo = geometryInfoBuffer[instIndex];
    if (geoInfo.vertexBufferAddress == 0 || geoInfo.indexBufferAddress == 0) return float2(0.0, 0.0);

    uint triCount = geoInfo.indexCount / 3u;
    if (primitiveIndex >= triCount) return float2(0.0, 0.0);

    uint* indexBuffer = (uint*)geoInfo.indexBufferAddress;
    float* vertexBuffer = (float*)geoInfo.vertexBufferAddress;
    uint idxBase = primitiveIndex * 3u;
    uint i0 = indexBuffer[idxBase + 0u];
    uint i1 = indexBuffer[idxBase + 1u];
    uint i2 = indexBuffer[idxBase + 2u];
    if (i0 >= geoInfo.vertexCount || i1 >= geoInfo.vertexCount || i2 >= geoInfo.vertexCount) return float2(0.0, 0.0);

    uint vertexStride = 12; // floats
    float2 uv0 = float2(vertexBuffer[i0 * vertexStride + 6], vertexBuffer[i0 * vertexStride + 7]);
    float2 uv1 = float2(vertexBuffer[i1 * vertexStride + 6], vertexBuffer[i1 * vertexStride + 7]);
    float2 uv2 = float2(vertexBuffer[i2 * vertexStride + 6], vertexBuffer[i2 * vertexStride + 7]);
    float2 uv = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;
    uv.y = 1.0 - uv.y; // flip V for glTF
    return uv;
}

float computeBaseColorAlpha(MaterialData material, uint instIndex, uint primitiveIndex, float2 bary) {
    float alpha = material.alpha;
    if (material.baseColorTextureSet >= 0) {
        float2 uv = computeTriangleUV(instIndex, primitiveIndex, bary);
        uint tiBase = (uint)min(max(material.baseColorTexIndex, 0), int(RQ_MAX_TEX - 1));
        float4 baseColor = baseColorTex[NonUniformResourceIndex(tiBase)].SampleLevel(uv, 0.0);
        baseColor *= float4(material.albedo, material.alpha);
        alpha = baseColor.a;
    }
    return alpha;
}

// Calculate refraction direction using Snell's law
// Returns true if refraction occurs, false if total internal reflection
bool refract(float3 I, float3 N, float eta, out float3 refracted) {
    float cosi = -dot(N, I);
    float cost2 = 1.0 - eta * eta * (1.0 - cosi * cosi);
    if (cost2 > 0.0) {
        refracted = eta * I + (eta * cosi - sqrt(cost2)) * N;
        return true;
    }
    refracted = float3(0, 0, 0); // Initialize even on failure
    return false; // Total internal reflection
}

// Perform ray query and return hit information with proper vertex normals and material properties
HitInfo traceRay(float3 origin, float3 direction, float tMin, float tMax) {
    HitInfo result;
    result.hit = false;
    result.t = tMax;
    result.uv = float2(0.0, 0.0);
    result.texIndex = 0u;
    result.materialIndex = 0u;
    result.baseColor = float3(1.0, 1.0, 1.0);
    result.color = float3(0.0, 0.0, 0.0);
    result.alphaMode = 0;
    result.opacity = 1.0;
    
    // Create ray query object
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = tMin;
    ray.TMax = tMax;
    
    // Initialize ray query.
    // We do NOT force opaque because we need to alpha-test MASK materials in-shader
    // (RayQuery inline traversal has no any-hit shader).
    RayQuery<RAY_FLAG_NONE> q;
    uint primaryMask = 0xFF;
    q.TraceRayInline(
        tlas,
        RAY_FLAG_NONE,
        primaryMask,
        ray
    );
    
    // Process ray query - loop until Proceed() returns false
    // For opaque geometry, this finds the closest hit automatically
    // Add safety limit to prevent infinite loops
    int maxIterations = 1000;
    int iteration = 0;
    while (q.Proceed() && iteration < maxIterations) {
        iteration++;

        // Alpha-mask handling: emulate any-hit by inspecting candidate triangle alpha
        // and committing only when the candidate passes alpha test.
        // Slang/HLSL ray query candidate type for triangle intersections.
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE) {
            uint instIndex = q.CandidateInstanceID();
            if (instIndex < uint(max(0, ubo.geometryInfoCount))) {
                GeometryInfo geoInfoC = geometryInfoBuffer[instIndex];
                uint materialIndexC = 0u;
                if (ubo.materialCount > 0) {
                    materialIndexC = min(geoInfoC.materialIndex, (uint)(ubo.materialCount - 1));
                }
                MaterialData matC = materialBuffer[materialIndexC];

                bool accept = true;
                if (matC.alphaMode == 1) {
                    float alpha = computeBaseColorAlpha(matC, instIndex, q.CandidatePrimitiveIndex(), q.CandidateTriangleBarycentrics());
                    accept = (alpha >= matC.alphaCutoff);
                }

                if (accept) {
                    q.CommitNonOpaqueTriangleHit();
                }
            }
        }
    }
    
    // (No debugPrintf in production)
    
    // Check if we hit anything
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        result.hit = true;
        result.t = q.CommittedRayT();
        result.worldPos = origin + direction * result.t;
        // Use CommittedInstanceID() which returns the instance custom index we set on CPU
        // (our per-instance sequential index that matches geometryInfoBuffer order).
        uint instIndex = q.CommittedInstanceID();
        result.instanceId = instIndex;
        
        // Get barycentric coordinates
        float2 bary = q.CommittedTriangleBarycentrics();
        float3 barycentrics = float3(1.0 - bary.x - bary.y, bary.x, bary.y);
        
        // PROPER GEOMETRY DATA FETCHING WITH SAFETY CHECKS
        // Per-instance geometry info index equals the instance custom index we assigned on CPU.
        uint blasIndex = instIndex;
        uint primitiveIndex = q.CommittedPrimitiveIndex();

        // Minimal debug disabled in production
        
        // Validate instance index is in bounds of geometry info buffer
        if (blasIndex >= uint(max(0, ubo.geometryInfoCount))) {
            // Invalid BLAS index, use default values
            result.normal = float3(0, 1, 0);
            result.baseColor = float3(0.8, 0.8, 0.8);
            result.color = float3(0.8, 0.8, 0.8);
            result.metallic = 0.0;
            result.roughness = 0.5;
            result.transmission = 0.0;
            result.isGlass = false;
            result.alphaMode = 0;
            result.opacity = 1.0;
            return result;
        }
        
        // Get geometry info for this BLAS (unique mesh)
        GeometryInfo geoInfo = geometryInfoBuffer[blasIndex];
        
        // Fetch material first so that even if geometry fetch fails we can still show a material color
        // Material property fetch with bounds clamp
        // Clamp material index to valid range to prevent out-of-bounds access
        uint materialIndex = 0u;
        if (ubo.materialCount > 0) {
            materialIndex = min(geoInfo.materialIndex, (uint) (ubo.materialCount - 1));
        }
        result.materialIndex = materialIndex;
        MaterialData material = materialBuffer[materialIndex];

        // Validate buffer addresses are non-zero (geometry may still be streaming)
        if (geoInfo.vertexBufferAddress == 0 || geoInfo.indexBufferAddress == 0) {
            // Geometry not ready: show a stable material-derived color so the frame isn't flat gray
            result.normal = float3(0, 1, 0);
            float3 albedoBase = float3(material.albedo);
            result.baseColor = albedoBase;
            float u0 = float(materialIndex + 1);
            float3 h0 = frac(float3(0.06711056, 0.00583715, 0.2065) * u0);
            float3 hashColor0 = frac(h0 * 2.6180339);
            result.color = saturate(0.7 * hashColor0 + 0.3 * albedoBase);
            result.metallic = material.metallic;
            result.roughness = material.roughness;
            result.transmission = material.transmissionFactor;
            result.isGlass = (material.isGlass != 0);
            result.alphaMode = material.alphaMode;
            result.opacity = clamp(material.alpha, 0.0, 1.0);
            return result;
        }
        
        // Cast device addresses to typed pointers for index buffer
        uint* indexBuffer = (uint*)geoInfo.indexBufferAddress;
        float* vertexBuffer = (float*)geoInfo.vertexBufferAddress;

        // Validate primitive index is within range of available triangles
        uint triCount = geoInfo.indexCount / 3u;
        if (primitiveIndex >= triCount) {
            // Out of bounds primitive; show material-derived color
            result.normal = float3(0, 1, 0);
            float3 albedoBase = float3(material.albedo);
            result.baseColor = albedoBase;
            float u1 = float(materialIndex + 1);
            float3 h1 = frac(float3(0.06711056, 0.00583715, 0.2065) * u1);
            float3 hashColor1 = frac(h1 * 2.6180339);
            result.color = saturate(0.7 * hashColor1 + 0.3 * albedoBase);
            result.metallic = material.metallic;
            result.roughness = material.roughness;
            result.transmission = material.transmissionFactor;
            result.isGlass = (material.isGlass != 0);
            result.alphaMode = material.alphaMode;
            result.opacity = clamp(material.alpha, 0.0, 1.0);
            return result;
        }

        // Fetch triangle indices
        uint idxBase = primitiveIndex * 3u;
        uint i0 = indexBuffer[idxBase + 0u];
        uint i1 = indexBuffer[idxBase + 1u];
        uint i2 = indexBuffer[idxBase + 2u];
        
        // CRITICAL: Validate vertex indices are within bounds to prevent GPU hang
        // Reading beyond vertex buffer bounds causes GPU fault/hang
        if (i0 >= geoInfo.vertexCount || i1 >= geoInfo.vertexCount || i2 >= geoInfo.vertexCount) {
            // Out of bounds vertex indices - still present material-derived color
            result.normal = float3(0, 1, 0);
            float3 albedoBase = float3(material.albedo);
            result.baseColor = albedoBase;
            float u2 = float(materialIndex + 1);
            float3 h2 = frac(float3(0.06711056, 0.00583715, 0.2065) * u2);
            float3 hashColor2 = frac(h2 * 2.6180339);
            result.color = saturate(0.7 * hashColor2 + 0.3 * albedoBase);
            result.metallic = material.metallic;
            result.roughness = material.roughness;
            result.transmission = material.transmissionFactor;
            result.isGlass = (material.isGlass != 0);
            result.alphaMode = material.alphaMode;
            result.opacity = clamp(material.alpha, 0.0, 1.0);
            return result;
        }
        
        // Vertex layout: pos(3) + normal(3) + texCoord(2) + tangent(4) = 12 floats per vertex
        uint vertexStride = 12;  // floats per vertex
        
        // Read object-space normals directly (offset 3 floats for position, then 3 floats for normal)
        float3 n0 = float3(vertexBuffer[i0 * vertexStride + 3],
                           vertexBuffer[i0 * vertexStride + 4],
                           vertexBuffer[i0 * vertexStride + 5]);
        float3 n1 = float3(vertexBuffer[i1 * vertexStride + 3],
                           vertexBuffer[i1 * vertexStride + 4],
                           vertexBuffer[i1 * vertexStride + 5]);
        float3 n2 = float3(vertexBuffer[i2 * vertexStride + 3],
                           vertexBuffer[i2 * vertexStride + 4],
                           vertexBuffer[i2 * vertexStride + 5]);
        
        // Interpolate normal using barycentric coordinates
        float3 interpolatedNormal = n0 * barycentrics.x +
                                     n1 * barycentrics.y +
                                     n2 * barycentrics.z;
        
        // Transform to world space using per-instance normal matrix
        float3x3 nrmMat = float3x3(geoInfo.normalMatrix0.xyz, geoInfo.normalMatrix1.xyz, geoInfo.normalMatrix2.xyz);
        float3 N = normalize(mul(nrmMat, interpolatedNormal));
        result.normal = N;

        // Read UVs and sample baseColor texture if available
        float2 uv0 = float2(vertexBuffer[i0 * vertexStride + 6], vertexBuffer[i0 * vertexStride + 7]);
        float2 uv1 = float2(vertexBuffer[i1 * vertexStride + 6], vertexBuffer[i1 * vertexStride + 7]);
        float2 uv2 = float2(vertexBuffer[i2 * vertexStride + 6], vertexBuffer[i2 * vertexStride + 7]);
        float2 uv = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;
        uv.y = 1.0 - uv.y; // flip V for glTF
        result.uv = uv;
        
        // --- PBR texture sampling (explicit LOD 0 for compute) ---
        float2 uvSample = uv;
        float4 baseColor = float4(material.albedo, material.alpha);
        if (material.baseColorTextureSet >= 0) {
            uint tiBase = (uint)min(max(material.baseColorTexIndex, 0), int(RQ_MAX_TEX - 1));
            baseColor = baseColorTex[NonUniformResourceIndex(tiBase)].SampleLevel(uvSample, 0.0);
            baseColor *= float4(material.albedo, material.alpha);
            result.texIndex = tiBase;
        } else {
            result.texIndex = 0u;
        }

        result.baseColor = saturate(baseColor.rgb);

        result.alphaMode = material.alphaMode;
        result.opacity = clamp(baseColor.a, 0.0, 1.0);

        // Physical descriptor: metallic-roughness (default) or spec-gloss
        float4 mrOrSpecGloss = float4(1.0, 1.0, 1.0, 1.0);
        if (material.physicalDescriptorTextureSet >= 0) {
            uint tiPhys = (uint)min(max(material.physicalTexIndex, 0), int(RQ_MAX_TEX - 1));
            mrOrSpecGloss = baseColorTex[NonUniformResourceIndex(tiPhys)].SampleLevel(uvSample, 0.0);
        }

        float metallic = 0.0;
        float roughness = 1.0;
        float3 F0 = float3(0.04, 0.04, 0.04);
        float3 albedo = baseColor.rgb;

        if (material.useSpecGlossWorkflow != 0) {
            float3 specColorSG = mrOrSpecGloss.rgb * material.specularFactor;
            float gloss = clamp(mrOrSpecGloss.a * material.glossinessFactor, 0.0, 1.0);
            roughness = clamp(1.0 - gloss, 0.0, 1.0);
            F0 = specColorSG;
            float maxF0 = max(F0.r, max(F0.g, F0.b));
            albedo = baseColor.rgb * (1.0 - maxF0);
        } else {
            float metallicTex = mrOrSpecGloss.b;
            float roughnessTex = mrOrSpecGloss.g;
            metallic = clamp(metallicTex * material.metallic, 0.0, 1.0);
            roughness = clamp(roughnessTex * material.roughness, 0.0, 1.0);
            F0 = lerp(float3(0.04, 0.04, 0.04), baseColor.rgb, metallic);
            albedo = baseColor.rgb * (1.0 - metallic);
        }

        // Ambient occlusion
        float ao = material.ao;
        if (material.occlusionTextureSet >= 0) {
            uint tiAO = (uint)min(max(material.occlusionTexIndex, 0), int(RQ_MAX_TEX - 1));
            ao *= baseColorTex[NonUniformResourceIndex(tiAO)].SampleLevel(uvSample, 0.0).r;
        }

        // Emissive
        float3 emissiveTex = float3(1.0, 1.0, 1.0);
        if (material.emissiveTextureSet >= 0) {
            uint tiE = (uint)min(max(material.emissiveTexIndex, 0), int(RQ_MAX_TEX - 1));
            emissiveTex = baseColorTex[NonUniformResourceIndex(tiE)].SampleLevel(uvSample, 0.0).rgb;
        }
        float3 emissive = emissiveTex * material.emissive;
        if (material.hasEmissiveStrengthExt != 0) {
            emissive *= material.emissiveStrength;
        }

        // Store F0 so the caller can compute Fresnel for reflection/transmission.
        result.F0 = F0;
        result.ior = max(material.ior, 1.0);
        result.isLiquid = (material.isLiquid != 0);

        // Normal mapping (tangent space)
        if (material.normalTextureSet >= 0) {
            uint tiN = (uint)min(max(material.normalTexIndex, 0), int(RQ_MAX_TEX - 1));
            float3 tangentNormal = baseColorTex[NonUniformResourceIndex(tiN)].SampleLevel(uvSample, 0.0).xyz * 2.0 - 1.0;

            // Read and interpolate tangent (object-space) from vertex buffer
            float4 t0 = float4(vertexBuffer[i0 * vertexStride + 8],
                               vertexBuffer[i0 * vertexStride + 9],
                               vertexBuffer[i0 * vertexStride + 10],
                               vertexBuffer[i0 * vertexStride + 11]);
            float4 t1 = float4(vertexBuffer[i1 * vertexStride + 8],
                               vertexBuffer[i1 * vertexStride + 9],
                               vertexBuffer[i1 * vertexStride + 10],
                               vertexBuffer[i1 * vertexStride + 11]);
            float4 t2 = float4(vertexBuffer[i2 * vertexStride + 8],
                               vertexBuffer[i2 * vertexStride + 9],
                               vertexBuffer[i2 * vertexStride + 10],
                               vertexBuffer[i2 * vertexStride + 11]);
            float4 tan4 = t0 * barycentrics.x + t1 * barycentrics.y + t2 * barycentrics.z;
            float3 T = normalize(mul(nrmMat, tan4.xyz));

            // We flip V for glTF (uv.y = 1-uv.y). In tangent space this inverts bitangent.
            // glTF tangent.w encodes bitangent sign in unflipped UV space, so negate it.
            float handedness = -tan4.w;
            float3 B = normalize(cross(N, T)) * handedness;
            float3x3 TBN = float3x3(T, B, N);
            N = normalize(mul(TBN, tangentNormal));
            result.normal = N;
        }

        // --- Direct lighting (GGX) ---
        float3 V = normalize(ubo.camPos.xyz - result.worldPos);
        float3 diffuseLighting = float3(0.0, 0.0, 0.0);
        float3 specularLighting = float3(0.0, 0.0, 0.0);

        int lc = max(ubo.lightCount, 0);
        for (int li = 0; li < lc; ++li) {
            LightData light = lightBuffer[li];
            float3 L;
            float3 radiance;
            if (light.lightType == 1) {
                L = normalize(-light.position.xyz);
                radiance = light.color.rgb;
            } else {
                float3 toLight = light.position.xyz - result.worldPos;
                float d = length(toLight);
                L = (d > 1e-5) ? (toLight / d) : float3(0, 0, 1);
                if (light.lightType == 3) {
                    float r = max(light.range, 0.001);
                    float att = 1.0 / (1.0 + (d / r) * (d / r));
                    radiance = light.color.rgb * att;
                } else {
                    radiance = light.color.rgb / max(d * d, 0.0001);
                }
            }
            float rawDot = dot(N, L);
            float NdotL = (light.lightType == 3) ? abs(rawDot) : max(rawDot, 0.0);
            if (NdotL > 0.0) {
                float3 H = normalize(V + L);
                float NdotV = max(dot(N, V), 0.0);
                float NdotH = max(dot(N, H), 0.0);
                float HdotV = max(dot(H, V), 0.0);
                float D = DistributionGGX(NdotH, roughness);
                float G = GeometrySmith(NdotV, NdotL, roughness);
                float3 F = FresnelSchlick(HdotV, F0);
                float3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
                float3 kD = (1.0 - F) * (1.0 - metallic);
                specularLighting += spec * radiance * NdotL;
                diffuseLighting += (kD * albedo / PI) * radiance * NdotL;
            }
        }

        float3 ambient = albedo * (ubo.scaleIBLAmbient) * ao;
        float3 color = ambient + diffuseLighting + specularLighting + emissive;

        result.color = color;
        result.metallic = metallic;
        result.roughness = roughness;
        result.transmission = material.transmissionFactor;
        result.isGlass = (material.isGlass != 0);

        // (No debug-only overrides in production)
    }
    
    return result;
}

// Compute shader entry point
[[shader("compute")]]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    uint2 imageDim = uint2(ubo.screenDimensions);
    
    // Bounds check
    if (pixelCoord.x >= imageDim.x || pixelCoord.y >= imageDim.y) {
        return;
    }
    
    // (No debug-only heartbeat/screen-test paths in production)

    // Generate primary ray
    float2 uv = (float2(pixelCoord) + 0.5) / float2(imageDim);
    float2 ndc = uv * 2.0 - 1.0;
    
    // Compute ray direction using inverse view-projection
    // Unproject a point on the near plane (z=0 in Vulkan NDC) to get direction
    float4x4 invProj = inverse(ubo.proj);
    float4x4 invView = inverse(ubo.view);
    
    // Unproject near plane point (z=0) and far plane point (z=1) to get ray direction
    // Near plane in clip space: z=0, w=1
    float4 nearClip = float4(ndc, 0.0, 1.0);
    float4 farClip = float4(ndc, 1.0, 1.0);
    
    // Transform to view space
    float4 nearView = mul(invProj, nearClip);
    float4 farView = mul(invProj, farClip);
    nearView /= nearView.w;
    farView /= farView.w;
    
    // Transform to world space
    float3 nearWorld = mul(invView, nearView).xyz;
    float3 farWorld = mul(invView, farView).xyz;
    
    // Primary ray from camera position through the pixel
    float3 rayOrigin = ubo.camPos.xyz;
    float3 rayDir = normalize(farWorld - nearWorld);
    
    HitInfo hit = traceRay(rayOrigin, rayDir, 0.0001, 10000.0);
    
    // Production: normal shading (no instance visualization)
    bool colorByInstance = false;

    if (hit.hit) {
        float3 c = shadeWithSecondaryRays(rayOrigin, rayDir, hit);
        // Output linear HDR-ish color; composite pass will apply exposure/gamma.
        outputImage[pixelCoord] = float4(c, 1.0);

        // (Debug print hooks intentionally removed from default path)
    } else {
        // Sky/background color
        outputImage[pixelCoord] = float4(skyColor(rayDir), 1.0);
    }
    // Debug probes disabled in production
}
