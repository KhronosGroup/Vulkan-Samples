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
// Tonemapping utilities for HDR to LDR conversion
// Shared between rasterization and ray query shaders

// Hable/Uncharted 2 filmic tonemapping operator
// Provides a cinematic look with good highlight rolloff
namespace Hable_Filmic_Tonemapping {
    static const float A = 0.15; 
    static const float B = 0.50;
    static const float C = 0.10; 
    static const float D = 0.20;
    static const float E = 0.02; 
    static const float F = 0.30;
    static const float W = 11.2;
    
    float3 Uncharted2Tonemap(float3 x) {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    }
}
