#version 450
/*
 * Copyright 2023 Nintendo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

layout (set = 0, binding = 0) uniform sampler2D samplerColorMap;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

void main()
{
	vec3 color = vec3(0);
	float weights[9] = {0,1,0,1,-4,1,0,1,0 };

	color += textureOffset(samplerColorMap, inUV, ivec2(-1, 1)).rgb * weights[0];
	color += textureOffset(samplerColorMap, inUV, ivec2( 0, 1)).rgb * weights[1];
	color += textureOffset(samplerColorMap, inUV, ivec2( 1, 1)).rgb * weights[2];
	color += textureOffset(samplerColorMap, inUV, ivec2(-1, 0)).rgb * weights[3];
	color += textureOffset(samplerColorMap, inUV, ivec2( 0, 0)).rgb * weights[4];
	color += textureOffset(samplerColorMap, inUV, ivec2( 1, 0)).rgb * weights[5];
	color += textureOffset(samplerColorMap, inUV, ivec2(-1,-1)).rgb * weights[6];
	color += textureOffset(samplerColorMap, inUV, ivec2( 0,-1)).rgb * weights[7];
	color += textureOffset(samplerColorMap, inUV, ivec2( 1,-1)).rgb * weights[8];

	outColor = vec4(color * 2.0f, 1.0f);
}
