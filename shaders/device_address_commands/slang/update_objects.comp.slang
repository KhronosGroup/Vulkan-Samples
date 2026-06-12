/* Copyright (c) 2026, Holochip Inc.
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

/**
 * Slang compute shader for the device_address_commands sample.
 *
 * Buffer accesses use Slang's native pointer syntax: typed pointers are
 * declared as struct fields and passed through the uniform push-constant
 * struct.  This is the Slang equivalent of GL_EXT_buffer_reference2 in GLSL.
 */

// Orbit parameters per object: (radius, phase, orbit_speed, scale).
struct OrbitParams
{
    float4 data;
};

// Single-element struct for the mat4 transform array.
// Accessed as float4x4* (pointer to first element, indexed by object id).
// (Slang does not support unsized array members in pointer-accessed structs.)

// VkDrawIndexedIndirectCommand packed as 5 uint32 values.
struct DrawCmd
{
    uint index_count;    // = 24
    uint instance_count; // = 1 (visible) or 0 (culled)
    uint first_index;    // = 0
    uint vertex_offset;  // = 0
    uint first_instance; // = object index
};

// Push-constant struct — layout must match DeviceAddressCommands::PushCompute.
struct PushCompute
{
    float         time;
    float         max_vis_dist;
    OrbitParams*  orbit_params;  // 64-bit device address at offset 8
    float4x4*     transforms;    // 64-bit device address at offset 16
    DrawCmd*      draw_cmds;     // 64-bit device address at offset 24
    uint*         draw_count;    // 64-bit device address at offset 32
    float3        camera_pos;    // offset 40
    float         _pad;          // offset 52
};

float4x4 translate_mat(float3 t)
{
    float4x4 m = float4x4(1, 0, 0, 0,
                           0, 1, 0, 0,
                           0, 0, 1, 0,
                           t.x, t.y, t.z, 1);
    return m;
}

float4x4 rotate_y_mat(float a)
{
    float c = cos(a), s = sin(a);
    return float4x4( c, 0,-s, 0,
                     0, 1, 0, 0,
                     s, 0, c, 0,
                     0, 0, 0, 1);
}

float4x4 rotate_x_mat(float a)
{
    float c = cos(a), s = sin(a);
    return float4x4(1, 0, 0, 0,
                    0, c, s, 0,
                    0,-s, c, 0,
                    0, 0, 0, 1);
}

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID, uniform PushCompute pc)
{
    uint idx = threadID.x;
    if (idx >= 256u) return;

    // Animate
    float4 p           = pc.orbit_params[idx].data;
    float  radius      = p.x;
    float  phase       = p.y;
    float  orbit_speed = p.z;
    float  obj_scale   = p.w;

    float angle = phase + orbit_speed * pc.time;
    float px    = radius * cos(angle);
    float pz    = radius * sin(angle);
    float py    = sin(angle * 1.7 + phase) * 3.0;

    float4x4 scale_m = float4x4(obj_scale, 0, 0, 0,
                                 0, obj_scale, 0, 0,
                                 0, 0, obj_scale, 0,
                                 0, 0, 0, 1);

    float4x4 model = mul(translate_mat(float3(px, py, pz)),
                    mul(rotate_y_mat(pc.time * orbit_speed * 2.0),
                    mul(rotate_x_mat(pc.time * orbit_speed * 1.3),
                        scale_m)));

    pc.transforms[idx] = model;

    // Culling
    float dist = length(float3(px, py, pz) - pc.camera_pos);
    if (dist >= pc.max_vis_dist) return;

    // Emit draw command via atomic slot allocation.
    uint slot;
    InterlockedAdd(*pc.draw_count, 1u, slot);

    DrawCmd cmd;
    cmd.index_count    = 24u;
    cmd.instance_count = 1u;
    cmd.first_index    = 0u;
    cmd.vertex_offset  = 0u;
    cmd.first_instance = idx;
    pc.draw_cmds[slot] = cmd;
}
