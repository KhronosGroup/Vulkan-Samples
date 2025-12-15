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
// Combined vertex and fragment shader for textured mesh rendering
// This shader provides basic textured rendering with simple lighting

// Input from vertex buffer
struct VSInput {
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float3 Normal;
    [[vk::location(2)]] float2 TexCoord;
    [[vk::location(3)]] float4 Tangent;

    // Per-instance data. Model matrix is a true 4x4, while the normal
    // matrix is provided as three float4 columns that match the CPU
    // layout (glm::mat3x4: 3 columns of vec4, xyz used, w unused).
    [[vk::location(4)]] column_major float4x4 InstanceModelMatrix;  // binding 1 (consumes 4 locations)
    [[vk::location(8)]] float4 InstanceNormal0;                     // normal matrix column 0
    [[vk::location(9)]] float4 InstanceNormal1;                     // normal matrix column 1
    [[vk::location(10)]] float4 InstanceNormal2;                    // normal matrix column 2
};

// Output from vertex shader / Input to fragment shader
struct VSOutput {
    float4 Position : SV_POSITION;
    float3 WorldPos;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Tangent : TANGENT; // Pass through tangent to satisfy validation layer
};

// Uniform buffer
struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

// Bindings
[[vk::binding(0, 0)]] ConstantBuffer<UniformBufferObject> ubo;
[[vk::binding(1, 0)]] Sampler2D texSampler;

// Vertex shader entry point
[[shader("vertex")]]
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    // Use instance matrices directly (CPU uploads column-major model
    // matrix and three float4 normal-matrix columns in attributes
    // 4..10)
    float4x4 instanceModelMatrix = input.InstanceModelMatrix;

    // Transform position to world space: entity model * instance model
    float4 worldPos = mul(ubo.model, mul(instanceModelMatrix, float4(input.Position, 1.0)));

    // Final clip space position
    output.Position = mul(ubo.proj, mul(ubo.view, worldPos));

    // Pass world position and transformed normal to fragment shader
    // (apply entity model to normals too). Reconstruct the 3x3 normal
    // matrix from the three uploaded columns and apply it in column
    // form to avoid any row/column layout ambiguity.
    float3x3 model3x3 = (float3x3)ubo.model;
    output.WorldPos = worldPos.xyz;

    float3 instNormal =
        input.InstanceNormal0.xyz * input.Normal.x +
        input.InstanceNormal1.xyz * input.Normal.y +
        input.InstanceNormal2.xyz * input.Normal.z;

    output.Normal = normalize(mul(model3x3, instNormal));
    output.TexCoord = input.TexCoord;
    output.Tangent = input.Tangent; // Pass through tangent (unused in basic rendering)

    return output;
}

// Fragment shader entry point
[[shader("fragment")]]
float4 PSMain(VSOutput input) : SV_TARGET
{
    // Sample the texture with flipped V coordinate (glTF UV origin vs Vulkan)
    float2 uv = float2(input.TexCoord.x, 1.0 - input.TexCoord.y);
    float4 texColor = texSampler.Sample(uv);

    // Simple directional lighting
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3)); // Fixed light direction
    float3 normal = normalize(input.Normal);
    float lightIntensity = max(dot(normal, lightDir), 0.2); // Minimum ambient of 0.2

    // If texture is nearly white, use a default color to avoid washed-out look
    float whiteness = (texColor.r + texColor.g + texColor.b) / 3.0;
    float4 finalColor = (whiteness > 0.95)
        ? float4(float3(0.8, 0.8, 0.8) * lightIntensity, 1.0)
        : float4(texColor.rgb * lightIntensity, texColor.a);

    return finalColor;
}
