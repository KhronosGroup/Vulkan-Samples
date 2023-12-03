/* Copyright (c) 2021-2023 Holochip Corporation
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

#version 460
#extension GL_EXT_ray_tracing : enable

struct Payload
{
  vec4 color;
  vec4 intersection; // {x, y, z, intersectionType}
  vec4 normal; // {nx, ny, nz, distance}
};


layout(location = 0) rayPayloadInEXT Payload hitValue;

void main()
{
    hitValue.intersection.w = 100;
    hitValue.normal.w = 10000;
}