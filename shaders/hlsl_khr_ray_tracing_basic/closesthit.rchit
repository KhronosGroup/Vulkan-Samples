/* Copyright (c) 2019-2020, Sascha Willems
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

struct Payload
{
    float3 hitValue;
};

struct Attribute
{
    float2 bary;
};

[shader("closesthit")]
void main(inout Payload payload, in Attribute attribs)
{
    const float3 barycentrics = float3(1.0 - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    payload.hitValue = barycentrics;
}