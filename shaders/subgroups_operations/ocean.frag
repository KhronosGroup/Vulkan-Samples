#version 450
/* Copyright (c) 2023, Mobica Limited
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

#define LAMBERT_VAL 0.3183098861837907f

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 outFragColor;

layout (binding = 0) uniform Ubo
{
    mat4 projection;
    mat4 view;
    mat4 model;
} ubo;

layout (binding = 3) uniform CameraPos
{
    vec4 position;
} cam;

layout (binding = 4) uniform sampler2D fft_normal_map;

layout (binding = 5) uniform OceanParamsUbo
{
    vec3 light_color;
    vec3 light_position;
    vec3 ocean_color;
} ocean_ubo;

layout (binding = 6) uniform samplerCube skybox_texture_map;

struct ReflectedLight
{
    vec3 dir_diffuse;
    vec3 indirect_diffuse;
    vec3 dir_specular;
    vec3 indirect_specular;
};

struct Geometry
{
    vec3 position;
    vec3 normal;
    vec3 view_dir;
};

struct PointLight
{
    vec3 position;
    vec3 color;
    float dist;
    float decay;
};

struct IncidentLight
{
    vec3 color;
    vec3 dir;
    bool is_visiable;
};

struct Material
{
    vec3 diffuse_color;
    vec3 specular_color;
    vec3 attenuation_color;
    float roughness;
    float ior;
    float thickness;
    float specular_f90;
};

float pow2(float x) { return x * x; }
vec3 pow2(vec3 x) { return x * x; }

vec3 brdf_lambert(vec3 diffuse_color)
{
    return LAMBERT_VAL * diffuse_color;
}

vec2 dfg_approx(vec3 normal, vec3 view_dir, float roughness)
{
    float dotNV = clamp(dot(normal, view_dir), 0.0f, 1.0f);
    vec4 c0 = vec4(-1.0f, -0.0275f, -0.572f, 0.022f);
    vec4 c1 = vec4(1.0f, 0.0425f, 1.04f, -0.04f);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(- 9.28 * dotNV)) * r.x + r.y;
    vec2 fab = vec2(-1.04f, 1.04f) * a004 + r.zw;
    return fab;
}

float dist_attenuation(float light_dist, float cutoff_dist, float decay_exponent)
{
    if (cutoff_dist > 0.0f && decay_exponent > 0.0f)
    {
        return pow(clamp(-light_dist / cutoff_dist + 1.0f, 0.0f, 1.0f), decay_exponent);
    }
    return 1.0f;
}

vec3 schlick(vec3 f0, float f90, float dotVH)
{
    float fres = exp2((-5.55f * dotVH - 6.98f) * dotVH);
    return f0 * (1.0f - fres) + (f90 * fres);
}

float smith_correlated(float alpha, float dotNL, float dotNV)
{
    float a2 = pow2(alpha);
    float gv = dotNL * sqrt(a2 + (1.0f - a2) * pow2(dotNV));
    float gl = dotNV * sqrt(a2 + (1.0f - a2) * pow2(dotNL));
    return 0.5f / max(gv + gl, 1e-6);
}

float ggx(float alpha, float dotNH)
{
    float a2 = pow2(alpha);
    float den = pow2(dotNH) * (a2 - 1.0f) + 1.0f;
    return LAMBERT_VAL * a2 / pow2(den);
}

vec3 brdf_ggx(vec3 light_dir, vec3 view_dir, vec3 normal, vec3 f0, float f90, float roughness)
{
    float alpha = pow2(roughness);
    vec3 half_dir = normalize(light_dir + view_dir);
    float dotNL = clamp(dot(normal, light_dir), 0.0f, 1.0f);
    float dotNV = clamp(dot(normal, view_dir), 0.0f, 1.0f);
    float dotNH = clamp(dot(normal, half_dir), 0.0f, 1.0f);
    float dotVH = clamp(dot(view_dir, half_dir), 0.0f, 1.0f);

    vec3 F = schlick(f0, f90, dotVH);
    float D = smith_correlated(alpha, dotNL, dotNV);
    float V = ggx(alpha, dotNH);
    return F * (D * V);
}



IncidentLight get_point_light_info(PointLight pl, Geometry gem)
{
    vec3 len_vec = pl.position - gem.position;
    float light_dist = length(len_vec);
    IncidentLight result;
    result.dir = normalize(len_vec);
    result.color = pl.color;
    result.color *= dist_attenuation(light_dist, pl.dist, pl.decay);
    result.is_visiable = bool(result.color != vec3(0.0f));
    return result;
}

ReflectedLight direct_physical(IncidentLight il, Geometry gem, Material mat)
{
    float dotNL = clamp(dot(gem.normal, il.dir), 0.0f, 1.0f);
    vec3 irradiance = dotNL * il.color;
    ReflectedLight result;
    result.dir_specular += irradiance * brdf_ggx(il.dir, gem.view_dir, gem.normal, mat.specular_color, mat.specular_f90, mat.roughness);
    result.dir_diffuse += irradiance * brdf_lambert(mat.diffuse_color);
    return result;
}

void main()
{
    vec3 total_emissive = vec3(0.3f, 0.3f, 0.3f);
    ReflectedLight ref_light = ReflectedLight(vec3(1.0f), vec3(1.0f), vec3(1.0f), vec3(1.0f));
    float metalness = 0.91f;
    float roughness = 0.91f;
    float ior = 0.5f;
    vec3 specular_color = vec3(1.0f);
    float specular_intensity = 1.0f;

    vec4 diffuse_color = vec4(ocean_ubo.ocean_color.rgb, 0.5f);
    vec3 normal = normalize(texture(fft_normal_map, in_uv).rgb);

    Material mat;
    mat.diffuse_color = diffuse_color.rgb * (1.0f - metalness);
    vec3 dxy = max(abs(dFdx(normal)), abs(dFdy(normal)));
    float gem_roughness = max(max(dxy.x, dxy.y), dxy.z);

    mat.roughness = max(roughness, 0.0525f);
    mat.roughness += gem_roughness;
    mat.roughness = min(mat.roughness, 1.0f);
    mat.ior = ior;
    mat.specular_f90 = mix(specular_intensity, 1.0f, metalness);
    mat.specular_color = mix(min(pow2((mat.ior - 1.0f) / (mat.ior + 1.0f)) * specular_color, vec3(1.0f)) * specular_intensity, diffuse_color.rgb, metalness);

    Geometry geom;
    geom.normal = normal;
    geom.position = -in_pos;
    geom.view_dir = normalize(cam.position.xyz);

    PointLight point_light;
    point_light.position = ocean_ubo.light_position;
    point_light.color = ocean_ubo.light_color;
    point_light.dist = length(ocean_ubo.light_position - in_pos);
    point_light.decay = 2.0f;

    IncidentLight dir_light = get_point_light_info(point_light, geom);

    ref_light = direct_physical(dir_light, geom, mat);

    vec3 irradiance = ocean_ubo.ocean_color;
    vec3 ibl_irradiance = vec3(1.0f);// envinroment color value - TODO

    vec3 radiance = vec3(1.0f);
    // radiance += get_ibl_radiance(geom.view_dir, geom.normal, mat.roughness); // envinroment color value - TODO

    ref_light.indirect_diffuse += irradiance * brdf_lambert(mat.diffuse_color);

    vec3 single_scattering = vec3(0.0f);
    vec3 multi_scattering = vec3(0.0f);
    vec3 cos_weighted_irradiance = irradiance * LAMBERT_VAL;

    vec2 fab = dfg_approx(geom.normal, geom.view_dir, roughness);
    vec3 FssEss = mat.specular_color + fab.x + mat.specular_f90 * fab.y;
    float Ess = fab.x + fab.y;
    float Ems = 1.0f - Ess;
    vec3 f_avg = mat.specular_color * (1.0f - mat.specular_color) * 0.0476f;
    vec3 f_ms =  FssEss * f_avg / (1.0f - Ems * f_avg);
    single_scattering += FssEss;
    multi_scattering += f_ms * Ems;

    vec3 total_scattering = single_scattering + multi_scattering;
    vec3 scattering_diffiuse = mat.diffuse_color * (1.0f - max(max(total_scattering.r, total_scattering.g), total_scattering.b));
    ref_light.indirect_specular += radiance * single_scattering;
    ref_light.indirect_specular += multi_scattering * cos_weighted_irradiance;
    ref_light.indirect_diffuse += scattering_diffiuse * cos_weighted_irradiance;


    vec3 total_diffuse = ref_light.dir_diffuse + ref_light.indirect_diffuse;
    vec3 total_specular = ref_light.dir_specular + ref_light.indirect_specular;

    outFragColor = vec4(total_diffuse + total_specular + total_emissive, diffuse_color.a);
}
