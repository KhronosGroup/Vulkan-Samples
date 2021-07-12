/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#version 460
#extension GL_EXT_ray_tracing : enable

struct hitPayload
{
	vec3 radiance;
	vec3 attenuation;
	int  done;
	vec3 rayOrigin;
	vec3 rayDir;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
	prd.radiance = vec3(0.3) * prd.attenuation;
	prd.done     = 1;
}
