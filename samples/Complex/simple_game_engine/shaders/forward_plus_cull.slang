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
// Forward+ tiled light culling (2D tiles)

struct LightData {
    [[vk::offset(0)]]   float4 position;
    [[vk::offset(16)]]  float4 color;
    [[vk::offset(32)]]  column_major float4x4 lightSpaceMatrix;
    [[vk::offset(96)]]  int   lightType;
    [[vk::offset(100)]] float range;
    [[vk::offset(104)]] float innerConeAngle;
    [[vk::offset(108)]] float outerConeAngle;
};

struct TileHeader { uint offset; uint count; uint pad0; uint pad1; };

// Params packed by the engine (see updateForwardPlusParams)
struct FPParams {
    column_major float4x4 view;
    column_major float4x4 proj;
    float4 screenTile; // x=width,y=height,z=tileX,w=tileY
    uint4 counts;      // x=lightCount,y=maxPerTile,z=tilesX,w=tilesY
    float4 zParams;    // x=nearZ, y=farZ, z=slicesZ, w=0
};

[[vk::binding(0, 0)]] StructuredBuffer<LightData> lightsRO;
[[vk::binding(1, 0)]] RWStructuredBuffer<TileHeader> tileHeadersRW;
[[vk::binding(2, 0)]] RWStructuredBuffer<uint> tileLightIndicesRW;
[[vk::binding(3, 0)]] ConstantBuffer<FPParams> params;

// NOTE: This implementation performs a conservative 2D geometric test per tile:
// it projects each point light to screen-space and computes an approximate screen-space
// radius from its world-space range using the projection matrix. A light is included in
// a tile if the circle intersects the tile rectangle. Depth/cluster slicing can be added later.

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint tilesX = params.counts.z;
    uint tilesY = params.counts.w;
    uint slicesZ = (uint)params.zParams.z;
    uint maxPerTile = params.counts.y;
    uint lightCount = params.counts.x;

    uint cx = min(DTid.x, (tilesX > 0) ? tilesX - 1 : 0);
    uint cy = min(DTid.y, (tilesY > 0) ? tilesY - 1 : 0);
    uint cz = (slicesZ > 0) ? min(DTid.z, slicesZ - 1) : 0;

    uint tileId = (cz * tilesY + cy) * tilesX + cx;

    // Screen and tile metrics
    float2 screenSize = params.screenTile.xy;
    float2 tileSize   = params.screenTile.zw; // (tileXSize, tileYSize)
    float2 tileMin = float2(cx, cy) * tileSize;
    float2 tileMax = tileMin + tileSize;

    uint base = tileId * maxPerTile;
    uint count = 0;

    // Precompute projection scaling terms to estimate screen-space radius
    // For a perspective matrix, proj[0][0] and proj[1][1] scale x/y by f/z.
    float projXX = params.proj[0][0];
    float projYY = params.proj[1][1];

    // Log-sliced depth range for this cluster (positive distances)
    float nearZ = max(params.zParams.x, 1e-3);
    float farZ  = max(params.zParams.y, nearZ + 1e-3);
    float fcz0 = (slicesZ > 0) ? (float(cz) / float(slicesZ)) : 0.0;
    float fcz1 = (slicesZ > 0) ? (float(cz + 1) / float(slicesZ)) : 1.0;
    float sliceNear = exp(lerp(log(nearZ), log(farZ), fcz0));
    float sliceFar  = exp(lerp(log(nearZ), log(farZ), fcz1));

    // Iterate over all lights and append those intersecting this tile
    [loop]
    for (uint li = 0; li < lightCount; ++li)
    {
        if (count >= maxPerTile) { break; }

        LightData L = lightsRO[li];

        // Only point and spot lights have finite range spheres; treat directional as global (include all tiles/slices)
        bool isDirectional = (L.lightType == 1);
        bool includeAll = isDirectional;

        float2 centerPx = float2(0.0, 0.0);
        float radiusPx = 1e9; // huge for directional
        bool zOverlap = true;

        if (!includeAll)
        {
            // Transform light center to view space
            float4 posVS = mul(params.view, float4(L.position.xyz, 1.0));

            // Use positive depth distance
            float z = max(1e-3, abs(posVS.z));

            // Z overlap test with this slice
            float zMin = max(0.0, z - L.range);
            float zMax = z + L.range;
            zOverlap = (zMax >= sliceNear) && (zMin <= sliceFar);

            // Project to clip then NDC
            float4 clip = mul(params.proj, float4(posVS.xyz, 1.0));
            float invW = (clip.w != 0.0) ? rcp(clip.w) : 0.0;
            float2 ndc = clip.xy * invW;            // [-1,1]
            centerPx = (ndc * 0.5 + 0.5) * screenSize; // pixels

            // Approximate screen-space radius from world radius (range)
            float rx = abs(L.range * projXX / z) * (screenSize.x * 0.5);
            float ry = abs(L.range * projYY / z) * (screenSize.y * 0.5);
            radiusPx = max(rx, ry);
        }

        // Circle vs axis-aligned rectangle overlap test (conservative)
        float2 closest = clamp(centerPx, tileMin, tileMax);
        float2 d = closest - centerPx;
        float dist2 = dot(d, d);
        if (zOverlap && dist2 <= radiusPx * radiusPx)
        {
            tileLightIndicesRW[base + count] = li;
            count++;
        }
    }

    // Write header
    TileHeader hdr;
    hdr.offset = base;
    hdr.count = count;
    hdr.pad0 = 0; hdr.pad1 = 0;
    tileHeadersRW[tileId] = hdr;
}
