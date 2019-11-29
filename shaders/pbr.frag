#version 320 es
/* Copyright (c) 2019, Arm Limited and Contributors
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

/* References will be formatted by: [source] explination
 *
 * Sources:
 * [0] https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf (Frostbites transition to PBR)
 * [1] https://learnopengl.com/PBR/Theory (Theory, Lighting and IBL sections)
 *
 * Extra:
 * glTF sample viewer PBR: https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/metallic-roughness.frag
 * Google Filament Engine: https://google.github.io/filament/Filament.html
 */

precision highp float;

#ifdef HAS_BASE_COLOR_TEXTURE
layout(set = 0, binding = 0) uniform sampler2D base_color_texture;
#endif

#ifdef HAS_NORMAL_TEXTURE
layout(set = 0, binding = 2) uniform sampler2D normal_texture;
#endif

#ifdef HAS_METALLIC_ROUGHNESS_TEXTURE
layout(set = 0, binding = 3) uniform sampler2D metallic_roughness_texture;
#endif

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 1) uniform GlobalUniform
{
	mat4 model;
	mat4 view_proj;
	vec3 camera_position;
}
global_uniform;

struct Light
{
	vec4 position;         // position.w represents type of light
	vec4 color;            // color.w represents light intensity
	vec4 direction;        // direction.w represents range
	vec2 info;             // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
};

layout(set = 0, binding = 4) uniform LightsInfo
{
	uint  count;
	Light lights[MAX_FORWARD_LIGHT_COUNT];
}
lights;

layout(push_constant, std430) uniform PBRMaterialUniform
{
	vec4  base_color_factor;
	float metallic_factor;
	float roughness_factor;
}
pbr_material_uniform;

const float PI = 3.14159265359;

vec3 F0 = vec3(0.04);

// [0] Frensel Schlick
vec3 F_Schlick(vec3 f0, float f90, float u)
{
	return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

// [1] IBL Defuse Irradiance
vec3 F_Schlick_Roughness(vec3 F0, float cos_theta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}

// [0] Diffuse Term
float Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float roughness)
{
	float E_bias        = 0.0 * (1.0 - roughness) + 0.5 * roughness;
	float E_factor      = 1.0 * (1.0 - roughness) + (1.0 / 1.51) * roughness;
	float fd90          = E_bias + 2.0 * LdotH * LdotH * roughness;
	vec3  f0            = vec3(1.0);
	float light_scatter = F_Schlick(f0, fd90, NdotL).r;
	float view_scatter  = F_Schlick(f0, fd90, NdotV).r;
	return light_scatter * view_scatter * E_factor;
}

// [0] Specular Microfacet Model
float V_SmithGGXCorrelated(float NdotV, float NdotL, float roughness)
{
	float alphaRoughnessSq = roughness * roughness;

	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

	float GGX = GGXV + GGXL;
	if (GGX > 0.0)
	{
		return 0.5 / GGX;
	}
	return 0.0;
}

// [0] GGX Normal Distribution Function
float D_GGX(float NdotH, float roughness)
{
	float alphaRoughnessSq = roughness * roughness;
	float f                = (NdotH * alphaRoughnessSq - NdotH) * NdotH + 1.0;
	return alphaRoughnessSq / (PI * f * f);
}

vec3 normal()
{
	vec3 pos_dx = dFdx(in_pos);
	vec3 pos_dy = dFdy(in_pos);
	vec3 st1    = dFdx(vec3(in_uv, 0.0));
	vec3 st2    = dFdy(vec3(in_uv, 0.0));
	vec3 T      = (st2.t * pos_dx - st1.t * pos_dy) / (st1.s * st2.t - st2.s * st1.t);
	vec3 N      = normalize(in_normal);
	T           = normalize(T - N * dot(N, T));
	vec3 B      = normalize(cross(N, T));
	mat3 TBN    = mat3(T, B, N);

#ifdef HAS_NORMAL_TEXTURE
	vec3 n = texture(normal_texture, in_uv).rgb;
	return normalize(TBN * (2.0 * n - 1.0));
#else
	return normalize(TBN[2].xyz);
#endif
}

vec3 diffuse(vec3 albedo, float metallic)
{
	return albedo * (1.0 - metallic) + ((1.0 - metallic) * albedo) * metallic;
}

float saturate(float t)
{
	return clamp(t, 0.0, 1.0);
}

vec3 saturate(vec3 t)
{
	return clamp(t, 0.0, 1.0);
}

vec3 apply_directional_light(uint index, vec3 normal)
{
	vec3 world_to_light = -lights.lights[index].direction.xyz;

	world_to_light = normalize(world_to_light);

	float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);

	return ndotl * lights.lights[index].color.w * lights.lights[index].color.rgb;
}

vec3 apply_point_light(uint index, vec3 normal)
{
	vec3 world_to_light = lights.lights[index].position.xyz - in_pos.xyz;

	float dist = length(world_to_light);

	float atten = 1.0 / (dist * dist);

	world_to_light = normalize(world_to_light);

	float ndotl = clamp(dot(normal, world_to_light), 0.0, 1.0);

	return ndotl * lights.lights[index].color.w * atten * lights.lights[index].color.rgb;
}

vec3 get_light_direction(uint index)
{
	if (lights.lights[index].position.w == DIRECTIONAL_LIGHT)
	{
		return -lights.lights[index].direction.xyz;
	}
	if (lights.lights[index].position.w == POINT_LIGHT)
	{
		return lights.lights[index].position.xyz - in_pos.xyz;
	}
}

void main(void)
{
	// vec3 position = vec3(0, 0, 0);

	float F90        = saturate(50.0 * F0.r);
	vec4  base_color = vec4(1.0, 0.0, 0.0, 1.0);

#ifdef HAS_BASE_COLOR_TEXTURE
	base_color = texture(base_color_texture, in_uv);
#else
	base_color      = pbr_material_uniform.base_color_factor;
#endif

#ifdef HAS_METALLIC_ROUGHNESS_TEXTURE
	float roughness = saturate(texture(metallic_roughness_texture, in_uv).g);
	float metallic  = saturate(texture(metallic_roughness_texture, in_uv).b);
#else
	float roughness = pbr_material_uniform.roughness_factor;
	float metallic  = pbr_material_uniform.metallic_factor;
#endif

	vec3  N     = normal();
	vec3  V     = normalize(global_uniform.camera_position - in_pos);
	float NdotV = saturate(dot(N, V));

	vec3 LightContribution = vec3(0.0);
	vec3 diffuse_color     = base_color.rgb * (1.0 - metallic);

	for (uint i = 0U; i < lights.count; ++i)
	{
		vec3 L = get_light_direction(i);
		vec3 H = normalize(V + L);

		float LdotH = saturate(dot(L, H));
		float NdotH = saturate(dot(N, H));
		float NdotL = saturate(dot(N, L));

		vec3  F   = F_Schlick(F0, F90, LdotH);
		float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness);
		float D   = D_GGX(NdotH, roughness);
		vec3  Fr  = F * D * Vis;

		float Fd = Fr_DisneyDiffuse(NdotV, NdotL, LdotH, roughness);

		if (lights.lights[i].position.w == DIRECTIONAL_LIGHT)
		{
			LightContribution += apply_directional_light(i, N) * (diffuse_color * (vec3(1.0) - F) * Fd + Fr);
		}
		if (lights.lights[i].position.w == POINT_LIGHT)
		{
			LightContribution += apply_point_light(i, N) * (diffuse_color * (vec3(1.0) - F) * Fd + Fr);
		}
	}

	// [1] Tempory irradiance to fix dark metals
	// TODO: add specular irradiance for realistic metals
	vec3 irradiance  = vec3(0.5);
	vec3 F           = F_Schlick_Roughness(F0, max(dot(N, V), 0.0), roughness * roughness * roughness * roughness);
	vec3 ibl_diffuse = irradiance * base_color.rgb;

	vec3 ambient_color = ibl_diffuse;

	o_color = vec4(0.3 * ambient_color + LightContribution, base_color.a);
}
