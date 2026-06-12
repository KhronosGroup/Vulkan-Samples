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
 * Slang vertex shader for the device_address_commands sample.
 *
 * Camera and per-object transforms are accessed through typed device-address
 * pointers in the push-constant struct — the Slang equivalent of the
 * GL_EXT_buffer_reference2 approach used in the GLSL counterpart.
 */

struct CameraData
{
    float4x4 view_projection;
};

// Push constants — must match DeviceAddressCommands::PushVertex.
struct PushVertex
{
    CameraData* camera;      // 0   device address → mat4 view_projection
    float4x4*   transforms;  // 8   device address → mat4[]
};

struct VSOutput
{
    float4 pos   : SV_Position;
    float3 color : COLOR0;
};

// Simple HSV → RGB conversion.
float3 hsv_to_rgb(float h, float s, float v)
{
    float4 k = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(h.xxx + k.xyz) * 6.0 - k.www);
    return v * lerp(k.xxx, clamp(p - k.xxx, 0.0, 1.0), s);
}

[shader("vertex")]
VSOutput main(float3 inPos    : POSITION,
              uint   instanceIndex : SV_InstanceID,
              uniform PushVertex pc)
{
    // SV_InstanceID picks up firstInstance from the draw command, which the
    // compute shader sets to the object index in the orbit/transform arrays.
    int obj = (int)instanceIndex;

    float4x4 model = pc.transforms[obj];
    float4x4 vp    = pc.camera->view_projection;

    VSOutput output;
    output.pos = mul(vp, mul(model, float4(inPos, 1.0)));

    float hue   = float(obj) / 256.0;
    float value = 0.7 + 0.3 * float(obj % 4) / 3.0;
    output.color = hsv_to_rgb(hue, 0.85, value);
    return output;
}
