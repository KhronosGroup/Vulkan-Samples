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
// Lighting utilities for evaluating lights and accumulating BRDF contributions
// Shared between rasterization and ray query shaders

import common_types;
import pbr_utils;

// Result of evaluating a single light
struct LightEvaluation {
    float3 L;          // Direction to light (normalized)
    float3 radiance;   // Incident radiance
    float NdotL;       // Clamped N·L for BRDF evaluation
    bool valid;        // True if light contributes
};

// Evaluate a single light at a world position
// Returns light direction, radiance, and N·L
LightEvaluation evaluateLight(LightData light, float3 worldPos, float3 N) {
    LightEvaluation result;
    result.valid = false;
    
    if (light.lightType == 1) {
        // Directional light
        result.L = normalize(-light.position.xyz);
        result.radiance = light.color.rgb;
        result.valid = true;
    } else {
        // Point/spot/emissive light: position.xyz is light position in world space
        float3 toLight = light.position.xyz - worldPos;
        float d = length(toLight);
        result.L = (d > 1e-5) ? toLight / d : float3(0, 0, 1);
        
        if (light.lightType == 3) {
            // Emissive light: soft falloff using range as characteristic radius
            float r = max(light.range, 0.001);
            float att = 1.0 / (1.0 + (d / r) * (d / r));
            result.radiance = light.color.rgb * att;
            result.valid = true;
        } else if (light.lightType == 0 || light.lightType == 2) {
            // Point or spot light: inverse square falloff
            result.radiance = light.color.rgb / max(d * d, 0.0001);
            result.valid = true;
        }
    }
    
    if (result.valid) {
        // For emissive lights, treat lighting as two-sided to avoid self-occlusion
        float rawDot = dot(N, result.L);
        result.NdotL = (light.lightType == 3) ? abs(rawDot) : max(rawDot, 0.0);
        result.valid = (result.NdotL > 0.0);
    }
    
    return result;
}

// Accumulate lighting contribution from a single light using GGX BRDF
// Adds diffuse and specular contributions to the provided accumulators
void accumulateLighting(
    LightEvaluation lightEval,
    float3 N,
    float3 V,
    float3 albedo,
    float metallic,
    float roughness,
    float3 F0,
    inout float3 diffuseLighting,
    inout float3 specularLighting)
{
    if (!lightEval.valid) return;
    
    float3 H = normalize(V + lightEval.L);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    // GGX microfacet BRDF
    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, lightEval.NdotL, roughness);
    float3 F = FresnelSchlick(HdotV, F0);
    
    float3 spec = (D * G * F) / max(4.0 * NdotV * lightEval.NdotL, 0.0001);
    float3 kD = (1.0 - F) * (1.0 - metallic);
    
    specularLighting += spec * lightEval.radiance * lightEval.NdotL;
    diffuseLighting += (kD * albedo / PI) * lightEval.radiance * lightEval.NdotL;
}
