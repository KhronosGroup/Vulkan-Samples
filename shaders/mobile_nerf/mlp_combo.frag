/* Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
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
 *
 * ------------------------------------------------------------------------
 *
 * THIS IS A MODIFIED VERSION OF THE ORIGINAL FILE
 *
 * The original file, along with the original Apache-2.0 LICENSE can be found at:
 * https://github.com/google-research/jax3d/tree/main/jax3d/projects/mobilenerf
 *
 * Modification details: Shader code was updated to work on Vulkan (originally
 * built for WebGL)
 * Contributor: (Qualcomm) Rodrigo Holztrattner - quic_rholztra@quicinc.com
 */
#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputFeature_0;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputFeature_1;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput rayDirectionIn;
layout (input_attachment_index = 3, binding = 3) uniform usubpassInput weightsIndex;

layout(location = 0) out vec4 o_color;

// Try defining constants in the shader itself
precision highp float;

#define WEIGHTS_0_COUNT (176)
#define WEIGHTS_1_COUNT (256)
// The third layer's size is changed from 48 to 64 to make sure a 16 bytes alignement
//#define WEIGHTS_2_COUNT (48)
#define WEIGHTS_2_COUNT (64)
#define BIAS_0_COUNT (16)
#define BIAS_1_COUNT (16)
// The third layer bias' size is changed from 3 to 4 to make sure a 16 bytes alignement
#define BIAS_2_COUNT (4)
layout(binding = 4) uniform mlp_weights
{
	vec4 data[(WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT +
               BIAS_0_COUNT + BIAS_1_COUNT + BIAS_2_COUNT)/4];        // Array of floats
} weights_arr[];

vec3 evaluateNetwork(  vec4 f0,  vec4 f1,  vec4 viewdir,  uint idx) {

        vec3 res;

        int bias_0_ind = WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT;
        vec4 intermediate_one[4] = vec4[](
           weights_arr[nonuniformEXT(idx)].data[bias_0_ind/4],
           weights_arr[nonuniformEXT(idx)].data[bias_0_ind/4 + 1],
           weights_arr[nonuniformEXT(idx)].data[bias_0_ind/4 + 2],
           weights_arr[nonuniformEXT(idx)].data[bias_0_ind/4 + 3]
        );

#define  APPLY_WEIGHTS_0(multiplier, weightFirstInd) \
        intermediate_one[ 0] += (multiplier) * weights_arr[nonuniformEXT(idx)].data[ weightFirstInd/4]; \
        intermediate_one[ 1] += (multiplier) * weights_arr[nonuniformEXT(idx)].data[ weightFirstInd/4 + 1]; \
        intermediate_one[ 2] += (multiplier) * weights_arr[nonuniformEXT(idx)].data[ weightFirstInd/4 + 2]; \
        intermediate_one[ 3] += (multiplier) * weights_arr[nonuniformEXT(idx)].data[ weightFirstInd/4 + 3];

         APPLY_WEIGHTS_0( f0.r,         0)
         APPLY_WEIGHTS_0( f0.g,        16)
         APPLY_WEIGHTS_0( f0.b,        32)
         APPLY_WEIGHTS_0( f0.a,        48)
         APPLY_WEIGHTS_0( f1.r,        64)
         APPLY_WEIGHTS_0( f1.g,        80)
         APPLY_WEIGHTS_0( f1.b,        96)
         APPLY_WEIGHTS_0( f1.a,       112)
         // For models form original mobile nerf, use the original code
         APPLY_WEIGHTS_0( viewdir.r,  128)
         APPLY_WEIGHTS_0( -viewdir.b, 144)
         APPLY_WEIGHTS_0( viewdir.g,  160)

        int bias_1_ind = WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT +
                         BIAS_0_COUNT;
        vec4 intermediate_two[4] = vec4[](
           weights_arr[nonuniformEXT(idx)].data[bias_1_ind/4],
           weights_arr[nonuniformEXT(idx)].data[bias_1_ind/4 + 1],
           weights_arr[nonuniformEXT(idx)].data[bias_1_ind/4 + 2],
           weights_arr[nonuniformEXT(idx)].data[bias_1_ind/4 + 3]
        );

#define  APPLY_WEIGHTS_1(intermediate, oneInd) \
         if(intermediate > 0.0f){ \
            intermediate_two[ 0] += intermediate * weights_arr[nonuniformEXT(idx)].data[ WEIGHTS_0_COUNT/4 + oneInd * 4 + 0]; \
            intermediate_two[ 1] += intermediate * weights_arr[nonuniformEXT(idx)].data[ WEIGHTS_0_COUNT/4 + oneInd * 4 + 1]; \
            intermediate_two[ 2] += intermediate * weights_arr[nonuniformEXT(idx)].data[ WEIGHTS_0_COUNT/4 + oneInd * 4 + 2]; \
            intermediate_two[ 3] += intermediate * weights_arr[nonuniformEXT(idx)].data[ WEIGHTS_0_COUNT/4 + oneInd * 4 + 3]; \
         }

         APPLY_WEIGHTS_1( intermediate_one[0].r, 0)
         APPLY_WEIGHTS_1( intermediate_one[0].g, 1)
         APPLY_WEIGHTS_1( intermediate_one[0].b, 2)
         APPLY_WEIGHTS_1( intermediate_one[0].a, 3)
         APPLY_WEIGHTS_1( intermediate_one[1].r, 4)
         APPLY_WEIGHTS_1( intermediate_one[1].g, 5)
         APPLY_WEIGHTS_1( intermediate_one[1].b, 6)
         APPLY_WEIGHTS_1( intermediate_one[1].a, 7)
         APPLY_WEIGHTS_1( intermediate_one[2].r, 8)
         APPLY_WEIGHTS_1( intermediate_one[2].g, 9)
         APPLY_WEIGHTS_1( intermediate_one[2].b, 10)
         APPLY_WEIGHTS_1( intermediate_one[2].a, 11)
         APPLY_WEIGHTS_1( intermediate_one[3].r, 12)
         APPLY_WEIGHTS_1( intermediate_one[3].g, 13)
         APPLY_WEIGHTS_1( intermediate_one[3].b, 14)
         APPLY_WEIGHTS_1( intermediate_one[3].a, 15)

        int bias_2_ind = WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT +
                         BIAS_0_COUNT + BIAS_1_COUNT;
        vec4 result = weights_arr[nonuniformEXT(idx)].data[bias_2_ind/4];

#define  APPLY_WEIGHTS_2(intermediate, oneInd) \
         if(intermediate > 0.0f){ \
            result += intermediate * weights_arr[nonuniformEXT(idx)].data[ WEIGHTS_0_COUNT/4 + WEIGHTS_1_COUNT/4 + oneInd]; \
         }

         APPLY_WEIGHTS_2(intermediate_two[0].r, 0)
         APPLY_WEIGHTS_2(intermediate_two[0].g, 1)
         APPLY_WEIGHTS_2(intermediate_two[0].b, 2)
         APPLY_WEIGHTS_2(intermediate_two[0].a, 3)
         APPLY_WEIGHTS_2(intermediate_two[1].r, 4)
         APPLY_WEIGHTS_2(intermediate_two[1].g, 5)
         APPLY_WEIGHTS_2(intermediate_two[1].b, 6)
         APPLY_WEIGHTS_2(intermediate_two[1].a, 7)
         APPLY_WEIGHTS_2(intermediate_two[2].r, 8)
         APPLY_WEIGHTS_2(intermediate_two[2].g, 9)
         APPLY_WEIGHTS_2(intermediate_two[2].b,10)
         APPLY_WEIGHTS_2(intermediate_two[2].a,11)
         APPLY_WEIGHTS_2(intermediate_two[3].r,12)
         APPLY_WEIGHTS_2(intermediate_two[3].g,13)
         APPLY_WEIGHTS_2(intermediate_two[3].b,14)
         APPLY_WEIGHTS_2(intermediate_two[3].a,15)

		 result = 1.0 / (1.0 + exp(-result));
         return vec3(result * viewdir.a+(1.0-viewdir.a));
      }

//////////////////////////////////////////////////////////////
// MLP was trained with gamma-corrected values              //
// convert to linear so sRGB conversion isn't applied twice //
//////////////////////////////////////////////////////////////

float Convert_sRGB_ToLinear(float value)
{
    return value <= 0.04045
        ? value / 12.92
        : pow((value + 0.055) / 1.055, 2.4);
}

vec3 Convert_sRGB_ToLinear(vec3 value)
{
    return vec3(Convert_sRGB_ToLinear(value.x), Convert_sRGB_ToLinear(value.y), Convert_sRGB_ToLinear(value.z));
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void main(void)
{
	vec4 feature_0 = subpassLoad(inputFeature_0).rgba;
	vec4 feature_1 = subpassLoad(inputFeature_1).rgba;
	vec4 rayDirection = subpassLoad(rayDirectionIn).rgba;
    uint idx = subpassLoad(weightsIndex).r;

	if (rayDirection.a < 0.6) discard;

	//deal with iphone
    feature_0.a = feature_0.a*2.0-1.0;
    feature_1.a = feature_1.a*2.0-1.0;
    rayDirection.a = rayDirection.a*2.0-1.0;

    // Original
    o_color.rgb = Convert_sRGB_ToLinear(evaluateNetwork(feature_0,feature_1,rayDirection,idx));
    o_color.a = 1.0;
}
