/* Copyright (c) 2021, Arm Limited and Contributors
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
layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0, rgba8) writeonly uniform image2D ImageOutput;
layout(set = 1, binding = 0) uniform sampler2D ImageInput;

void main()
{
    ivec2 index = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (vec2(index) + 0.5) / vec2(textureSize(ImageInput, 0));
    int neighbors = 0;

    vec4 self = textureLod(ImageInput, uv, 0.0);
    bool is_alive = any(notEqual(self.rgb, vec3(0.0)));
    vec3 total = self.rgb;

#define CHECK_OFFSET(x, y) { \
    vec3 tmp; \
    tmp = textureLodOffset(ImageInput, uv, 0.0, ivec2(x, y)).rgb; \
    if (any(notEqual(tmp, vec3(0.0)))) { \
        neighbors++; \
        total += tmp.rgb; \
    } \
}
    CHECK_OFFSET(-1, -1)
    CHECK_OFFSET( 0, -1)
    CHECK_OFFSET(+1, -1)
    CHECK_OFFSET(-1,  0)
    CHECK_OFFSET(+1,  0)
    CHECK_OFFSET(-1, +1)
    CHECK_OFFSET( 0, +1)
    CHECK_OFFSET(+1, +1)

    if (is_alive)
    {
        is_alive = neighbors == 2u || neighbors == 3u;
        if (is_alive)
        {
            total /= float(neighbors);
        }
        else
        {
            total = vec3(0.0);
        }
    }
    else
    {
        is_alive = neighbors == 3u;
        if (is_alive)
        {
            vec3 fresh_color = vec3(uv.x, uv.y, 1.0 - uv.x - uv.y);
            total = fresh_color;
        }
        else
        {
            total = vec3(0.0);
        }
    }

    imageStore(ImageOutput, index, vec4(total, 0.0));
}
