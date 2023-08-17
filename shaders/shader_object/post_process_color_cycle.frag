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

layout(push_constant) uniform pushConstants
{
	float elapsed_time;
} push;

//Based on HSV to RGB in https://en.wikipedia.org/wiki/HSL_and_HSV
vec3 rgb_from_hsv(float hue, float sat, float value)
{
	float h_prime = hue / 60.0f;
	float c = value * sat;
	float x = c * (1 - abs(mod(h_prime, 2) - 1));

	vec3 rgb;
	if(h_prime < 1)
	{
		rgb = vec3(c, x, 0);
	}
	else if(h_prime < 2)
	{
		rgb = vec3(x, c, 0);
	}
	else if(h_prime < 3)
	{
		rgb = vec3(0, c, x);
	}
	else if(h_prime < 4)
	{
		rgb = vec3(0, x, c);
	}
	else if(h_prime < 5)
	{
		rgb = vec3(x, 0, c);
	}
	else if(h_prime < 6)
	{
		rgb = vec3(c, 0, x);
	}

	return rgb + vec3(value - c);
}

void main()
{
	vec3 color = texture(samplerColorMap, inUV).rgb;
	float time_scale = 0.1f;
	outColor = vec4(color * rgb_from_hsv(mod(push.elapsed_time * 60.f, 360.f), 1.f, 1.f), 1.0f);
}
