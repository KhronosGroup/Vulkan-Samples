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
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_nonuniform_qualifier : enable

// Uncomment this to use opaque mode
// Should be faster, but would get VK_ERROR_DEVICE_LOST on some AMD devices
// #define USE_OPAQUE

struct Vertex
{
    vec3 position;
    vec2 texCoord;
};

struct GlobalUniform
{
    vec3 camera_position;
    vec3 camera_side;
    vec3 camera_up;
    vec3 camera_lookat;
    vec2 img_dim;
    float tan_half_fov;
};

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform AppData
{
  GlobalUniform params;
};

layout(set = 0, binding = 1) uniform accelerationStructureEXT topLevelAS;

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
layout(set = 0, binding = 2) uniform mlp_weights
{
	vec4 data[(WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT +
               BIAS_0_COUNT + BIAS_1_COUNT + BIAS_2_COUNT)/4];        // Array of floats
} weights;

layout(set = 1, binding = 0, scalar) readonly buffer Vertices
{
  Vertex vertices[];
} vertices_set[];

layout(set = 2, binding = 0, scalar) readonly buffer Indices
{
  uint indices[];
} indices_set[];

layout(set = 3, binding = 0) uniform sampler2D textureInput_0[];
layout(set = 4, binding = 0) uniform sampler2D textureInput_1[];

vec3 evaluateNetwork(  vec4 f0,  vec4 f1,  vec4 viewdir)
{

    vec3 res;

    int bias_0_ind = WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT;
    vec4 intermediate_one[4] = vec4[](
        weights.data[bias_0_ind/4],
        weights.data[bias_0_ind/4 + 1],
        weights.data[bias_0_ind/4 + 2],
        weights.data[bias_0_ind/4 + 3]
    );


#define  APPLY_WEIGHTS_0(multiplier, weightFirstInd) \
        intermediate_one[ 0] += (multiplier) * weights.data[ weightFirstInd/4]; \
        intermediate_one[ 1] += (multiplier) * weights.data[ weightFirstInd/4 + 1]; \
        intermediate_one[ 2] += (multiplier) * weights.data[ weightFirstInd/4 + 2]; \
        intermediate_one[ 3] += (multiplier) * weights.data[ weightFirstInd/4 + 3];

    APPLY_WEIGHTS_0( f0.r,         0)
    APPLY_WEIGHTS_0( f0.g,        16)
    APPLY_WEIGHTS_0( f0.b,        32)
    APPLY_WEIGHTS_0( f0.a,        48)
    APPLY_WEIGHTS_0( f1.r,        64)
    APPLY_WEIGHTS_0( f1.g,        80)
    APPLY_WEIGHTS_0( f1.b,        96)
    APPLY_WEIGHTS_0( f1.a,       112)
    // For models form original mobile nerf, use the original code
    APPLY_WEIGHTS_0( (viewdir.r + 1.0 )/2,  128)
    APPLY_WEIGHTS_0( (-viewdir.b + 1.0 )/2, 144)
    APPLY_WEIGHTS_0( (viewdir.g + 1.0 )/2,  160)

    int bias_1_ind = WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT +
                        BIAS_0_COUNT;
    vec4 intermediate_two[4] = vec4[](
        weights.data[bias_1_ind/4],
        weights.data[bias_1_ind/4 + 1],
        weights.data[bias_1_ind/4 + 2],
        weights.data[bias_1_ind/4 + 3]
    );


#define  APPLY_WEIGHTS_1(intermediate, oneInd) \
         if(intermediate > 0.0f){ \
            intermediate_two[ 0] += intermediate * weights.data[ WEIGHTS_0_COUNT/4 + oneInd * 4 + 0]; \
            intermediate_two[ 1] += intermediate * weights.data[ WEIGHTS_0_COUNT/4 + oneInd * 4 + 1]; \
            intermediate_two[ 2] += intermediate * weights.data[ WEIGHTS_0_COUNT/4 + oneInd * 4 + 2]; \
            intermediate_two[ 3] += intermediate * weights.data[ WEIGHTS_0_COUNT/4 + oneInd * 4 + 3]; \
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
    vec4 result = weights.data[bias_2_ind/4];

#define  APPLY_WEIGHTS_2(intermediate, oneInd) \
         if(intermediate > 0.0f){ \
            result += intermediate * weights.data[ WEIGHTS_0_COUNT/4 + WEIGHTS_1_COUNT/4 + oneInd]; \
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

vec3 CalcRayDirComp(GlobalUniform params) {
    // On [0.0, 1.0]
    vec2 img_sample = vec2(gl_FragCoord.xy / params.img_dim);

    // Transform into [-0.5, 0.5]
    vec2 hom_sample = img_sample - vec2(0.5f, 0.5f);
    hom_sample.y *= -1.0f; // Vertical flip so that origin is top-left

    // Transform into [-dim.x/dim.y/2, dim.x/dim.y/2] x [-0.5, 0.5]
    vec2 c_sample = hom_sample * params.img_dim / params.img_dim.y;

    // Calculate direction to image plane
    const vec3 rayDir = vec3(params.camera_lookat * 0.5 / params.tan_half_fov + c_sample.x * params.camera_side + c_sample.y * params.camera_up);

    return normalize(rayDir);
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

#ifndef USE_OPAQUE
void main(void)
{
    vec3 rayDirection = CalcRayDirComp(params);

    // initialize a ray query object
    rayQueryEXT rayQuery;
    const uint rayFlags = gl_RayFlagsNoOpaqueEXT;           // Enable this so that we can get back fragment after discarding the transparent fragment
    const float tmin = 0.01f;
    const float tmax = 256.0f;
    vec4 pixel_0 = vec4(0.0f);
    vec2 commited_flipped = vec2(0.0f);
    int commited_instanceID = 0;
    rayQueryInitializeEXT(rayQuery,                         // Ray query
                          topLevelAS,                       // Top-level acceleration structure
                          rayFlags,                         // Ray flags, treat all geometry as non-opaque
                          0xFF,                             // 8-bit instance mask, trace against all instances
                          params.camera_position,           // Ray origin
                          tmin,                             // Minimum t-value
                          rayDirection,                     // Ray direction
                          tmax);                            // Maximum t-value

    while(rayQueryProceedEXT(rayQuery)) {
      if (rayQueryGetIntersectionTypeEXT(rayQuery, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
      {
          const int instanceID = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);

          // get primitive ID in order to access UVs of hitted triangle
          const int primitiveID = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
          const uint i0 = indices_set[nonuniformEXT(instanceID)].indices[3 * primitiveID];
          const uint i1 = indices_set[nonuniformEXT(instanceID)].indices[3 * primitiveID + 1];
          const uint i2 = indices_set[nonuniformEXT(instanceID)].indices[3 * primitiveID + 2];
          const vec2 uv0 = vertices_set[nonuniformEXT(instanceID)].vertices[i0].texCoord;
          const vec2 uv1 = vertices_set[nonuniformEXT(instanceID)].vertices[i1].texCoord;
          const vec2 uv2 = vertices_set[nonuniformEXT(instanceID)].vertices[i2].texCoord;

          // Get berycentric coordinate then interpolate the uv of the hit point
          vec3 barycentrics = vec3(0.0, rayQueryGetIntersectionBarycentricsEXT(rayQuery, false));
          barycentrics.x    = 1.0 - barycentrics.y - barycentrics.z;
          const vec2 hitpoint_uv = barycentrics.x * uv0 + barycentrics.y * uv1 + barycentrics.z * uv2;

          // Sample feature maps and check transparency
          const vec2 flipped = vec2( hitpoint_uv.x, 1.0 - hitpoint_uv.y );
          vec4 test_pixel = texture(textureInput_0[nonuniformEXT(instanceID)], flipped);

          if (test_pixel.r != 0.0) {
            rayQueryConfirmIntersectionEXT(rayQuery);
            pixel_0 = test_pixel;
            commited_flipped = flipped;
            commited_instanceID = instanceID;
          }
      }
    }

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT) {
      // Output feature inputs for mlp
      vec4 pixel_1 = texture(textureInput_1[nonuniformEXT(commited_instanceID)], commited_flipped);

      pixel_0.a = pixel_0.a*2.0-1.0;
      pixel_1.a = pixel_1.a*2.0-1.0;

      o_color.rgb = Convert_sRGB_ToLinear(evaluateNetwork(pixel_0, pixel_1, vec4(rayDirection, 1.0f)));
      o_color.a = 1.0;
    } else {
      discard;
    }
}

#else
// Much faster but not work correctly on Mobile nerf's original models
void main(void)
{
    vec3 rayDirection = CalcRayDirComp(params);

    // initialize a ray query object
    rayQueryEXT rayQuery;
    const uint rayFlags = gl_RayFlagsOpaqueEXT;
    const float tmin = 0.0f;
    const float tmax = 256.0f;
    rayQueryInitializeEXT(rayQuery,                         // Ray query
                          topLevelAS,                       // Top-level acceleration structure
                          rayFlags,                         // Ray flags, treat all geometry as opaque
                          0xFF,                             // 8-bit instance mask, trace against all instances
                          params.camera_position,           // Ray origin
                          tmin,                             // Minimum t-value
                          rayDirection,                     // Ray direction
                          tmax);                            // Maximum t-value

    // Start traversal
    rayQueryProceedEXT(rayQuery);

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT) {
      const int instanceID = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);

      // get primitive ID in order to access UVs of hitted triangle
      const int primitiveID = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
      const uint i0 = indices_set[nonuniformEXT(instanceID)].indices[3 * primitiveID];
      const uint i1 = indices_set[nonuniformEXT(instanceID)].indices[3 * primitiveID + 1];
      const uint i2 = indices_set[nonuniformEXT(instanceID)].indices[3 * primitiveID + 2];
      const vec2 uv0 = vertices_set[nonuniformEXT(instanceID)].vertices[i0].texCoord;
      const vec2 uv1 = vertices_set[nonuniformEXT(instanceID)].vertices[i1].texCoord;
      const vec2 uv2 = vertices_set[nonuniformEXT(instanceID)].vertices[i2].texCoord;

      // Get berycentric coordinate then interpolate the uv of the hit point
      vec3 barycentrics = vec3(0.0, rayQueryGetIntersectionBarycentricsEXT(rayQuery, true));
      barycentrics.x    = 1.0 - barycentrics.y - barycentrics.z;
      const vec2 hitpoint_uv = barycentrics.x * uv0 + barycentrics.y * uv1 + barycentrics.z * uv2;

      // Sample feature maps then output to second subpass
      vec2 flipped = vec2( hitpoint_uv.x, 1.0 - hitpoint_uv.y );
      vec4 pixel_0 = texture(textureInput_0[nonuniformEXT(instanceID)], flipped);
      vec4 pixel_1 = texture(textureInput_1[nonuniformEXT(instanceID)], flipped);

      pixel_0.a = pixel_0.a*2.0-1.0;
      pixel_1.a = pixel_1.a*2.0-1.0;

      o_color.rgb = Convert_sRGB_ToLinear(evaluateNetwork(pixel_0, pixel_1, vec4(rayDirection, 1.0f)));
      o_color.a = 1.0;
    } else {
      discard;
    }
}
#endif
