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

const vec2[3] positions = vec2[]
( 
	vec2(-1, -1),
	vec2(-1,  3),
	vec2( 3, -1) 
);

const vec2[3] uv = vec2[]
(
	vec2(0, 0),
	vec2(0, 2),
	vec2(2, 0)
);

layout (location = 0) out vec2 outUV;

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
	outUV = uv[gl_VertexIndex];
}
