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

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 posOffset;

layout(binding = 2) uniform GlobalUniform
{
	mat4x4 model;
	mat4x4 view;
	mat4x4 proj;
    vec3 camera_position;
    vec3 camera_side;
    vec3 camera_up;
    vec3 camera_lookat;
    vec2 img_dim;
}
global_uniform;

layout(location = 0) out vec2 texCoord_frag;
layout(location = 1) out vec3 rayDirection;

void main(void)
{
	texCoord_frag = texCoord;

	vec3 pos = position + posOffset;

	gl_Position = global_uniform.proj * global_uniform.view * global_uniform.model * vec4(pos.x, -pos.y, pos.z, 1.0);
	rayDirection = pos - vec3(global_uniform.camera_position.x, -global_uniform.camera_position.y, global_uniform.camera_position.z);
}
