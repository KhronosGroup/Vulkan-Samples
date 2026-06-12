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
 * @brief Vertex shader for the device_address_commands sample.
 *
 * All per-frame data (camera matrix and per-object transforms) is accessed
 * through GL_EXT_buffer_reference2 pointers carried in push constants.
 * This mirrors the approach of the buffer_device_address sample and shows
 * how every piece of per-draw data can be fed through device addresses rather
 * than descriptor sets.
 *
 * gl_InstanceIndex == firstInstance (set by the compute shader) == the index
 * of the object in the orbit/transform arrays.
 */

#version 450
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// Camera data uploaded each frame via vkCmdUpdateMemoryKHR.
layout(std430, buffer_reference, buffer_reference_align = 16) readonly buffer CameraRef
{
    mat4 view_projection;
};

// Per-object model matrices written by the compute shader.
layout(std430, buffer_reference, buffer_reference_align = 16) readonly buffer TransformRef
{
    mat4 data[];
};

// Push constants — must match DeviceAddressCommands::PushVertex.
layout(push_constant) uniform PC
{
    CameraRef    camera;     // 0   device address → mat4 view_projection
    TransformRef transforms; // 8   device address → mat4[]
} pc;

layout(location = 0) in  vec3 in_position;
layout(location = 0) out vec3 out_color;

// Simple HSV → RGB for per-object colour variety.
vec3 hsv_to_rgb(float h, float s, float v)
{
    vec4 k = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(vec3(h) + k.xyz) * 6.0 - k.www);
    return v * mix(k.xxx, clamp(p - k.xxx, 0.0, 1.0), s);
}

void main()
{
    // firstInstance carries the object index set by the compute shader.
    int obj = gl_InstanceIndex;

    mat4 model = pc.transforms.data[obj];
    gl_Position = pc.camera.view_projection * model * vec4(in_position, 1.0);

    // Spread hue evenly across 256 objects; use arm-based brightness bands
    // for added visual structure.
    float hue  = float(obj) / 256.0;
    float value = 0.7 + 0.3 * float(obj % 4) / 3.0;
    out_color = hsv_to_rgb(hue, 0.85, value);
}
