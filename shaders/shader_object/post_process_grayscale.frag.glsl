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
	vec3 color = texture(samplerColorMap, inUV).rgb;
	outColor = vec4(vec3(color.r + color.g + color.b) / 3.0f, 1.0f);
}
