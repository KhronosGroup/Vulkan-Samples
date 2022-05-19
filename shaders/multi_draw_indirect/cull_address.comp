#version 460

/* Copyright (c) 2021 Holochip Corporation
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

#extension GL_EXT_buffer_reference : require

layout(local_size_x = 64) in;

struct ModelInformation
{
	float x;
	float y;
	float z;
	float r;
	uint  texture_index;
	uint  firstIndex;
	uint  indexCount;
	uint  _pad;
};

struct VkDrawIndexedIndirectCommand
{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int  vertexOffset;
	uint firstInstance;
};

layout(std430, set = 0, binding = 0) buffer readonly ModelInformationBuffer
{
	ModelInformation arr[];
}
model_buffer;

layout(set = 0, binding = 2) uniform GlobalUniform
{
	mat4 view;
	mat4 proj;
	mat4 proj_view;
	uint model_count;
}
global_uniform;

layout(std430, buffer_reference) buffer CommandBuffer
{
	VkDrawIndexedIndirectCommand commands[];
}
command_buffer;

layout(set = 0, binding = 4) buffer Addresses
{
	CommandBuffer command_buffer;
}
addresses;

// See "VisibilityTester" in the C++ code for explanation
bool check_is_visible(mat4 mat, vec3 origin, float radius)
{
	uint plane_index = 0;
	for (uint i = 0; i < 3; ++i)
	{
		for (uint j = 0; j < 2; ++j, ++plane_index)
		{
			if (plane_index == 2 || plane_index == 3)
			{
				continue;
			}
			const float sign  = (j > 0) ? 1.f : -1.f;
			vec4        plane = vec4(0, 0, 0, 0);
			for (uint k = 0; k < 4; ++k)
			{
				plane[k] = mat[k][3] + sign * mat[k][i];
			}
			plane.xyzw /= sqrt(dot(plane.xyz, plane.xyz));
			if (dot(origin, plane.xyz) + plane.w + radius < 0)
			{
				return false;
			}
		}
	}
	return true;
}

void main()
{
	uint             id         = gl_GlobalInvocationID.x;
	if (id >= global_uniform.model_count)
	{
		return;
	}
	ModelInformation model      = model_buffer.arr[id];
	bool             is_visible = check_is_visible(global_uniform.proj_view, vec3(model.x, model.y, model.z), model.r);

	addresses.command_buffer.commands[id].instanceCount = is_visible ? 1 : 0;
}