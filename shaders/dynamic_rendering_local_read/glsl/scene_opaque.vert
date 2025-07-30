#version 450
/* Copyright (c) 2024, Sascha Willems
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

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outWorldPos;

layout(push_constant) uniform SceneNode {
	mat4 matrix;
	vec4 color;
} sceneNode;

void main() 
{
	mat4 nodeMat = ubo.model * sceneNode.matrix;

	gl_Position = ubo.projection * ubo.view * nodeMat * inPos;
	
	// Vertex position in world space
	outWorldPos = vec3(sceneNode.matrix * inPos);
	// GL to Vulkan coord space
	outWorldPos.y = -outWorldPos.y;
	
	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(mat3(nodeMat))));
	outNormal = normalize(mNormal * inNormal);
	
	outColor = sceneNode.color;
}
