/* Copyright (c) 2022, Mobica Limited
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
#version 450


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 4) in vec3 inLightVec;
layout(location = 5) in vec3 inLightColor[2];
layout(location = 7) in vec3 inViewVec;
layout(location = 8) in float inLightIntensity;

layout(location = 0) out vec4 outColor0;

layout(constant_id = 0) const int type = 0;

void main()
{
    float attenuation = 1.0 / dot(inLightVec, inLightVec);
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);

    vec3 diffuse = inLightColor[0] * attenuation * max(dot(N, L) ,0) * inLightIntensity;
	vec3 ambient = inLightColor[1];
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.65);
    //specular = vec3(0,0,0);
	outColor0 = vec4((ambient + diffuse) * inColor.rgb + (specular * inLightIntensity/50), inColor.a);		

}
