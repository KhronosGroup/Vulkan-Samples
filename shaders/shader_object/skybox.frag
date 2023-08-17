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

layout (binding = 1) uniform sampler2D samplerEnvMap;

layout (location = 0) in vec3 inUVWorldPos;
layout (location = 1) in vec3 inPos;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outColor0;

layout (constant_id = 0) const int type = 0;

#define PI 3.1415926
#define TwoPI (2.0 * PI)

void main() 
{
	outColor0.rgb = texture(samplerEnvMap, vec2(inUVWorldPos.x, -inUVWorldPos.y)).rgb;
}
