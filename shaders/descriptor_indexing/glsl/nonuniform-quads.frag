#version 450
/* Copyright (c) 2021-2024, Arm Limited and Contributors
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

// Adds support for nonuniformEXT and unsized descriptor arrays.
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform texture2D Textures[];
layout(set = 1, binding = 0) uniform sampler ImmutableSampler;

layout(location = 0) in vec2 in_uv;
layout(location = 1) flat in int in_texture_index;
layout(location = 0) out vec4 out_frag_color;

void main()
{
    // Here we are indexing into the texture array, with a non-uniform index.
    // Across the draw-call, there are different instance indices being used, so we must use nonuniformEXT.
    // It is important to note that to be 100% correct, we must use:
    // nonuniformEXT(sampler2D()).
    // It is the final argument to a call like texture() which determines if the access is to be considered non-uniform.
    // It is very common in the wild to see code like:
    // - sampler2D(Textures[nonuniformEXT(in_texture_index)], ...)
    // This looks very similar to HLSL, but it is somewhat wrong.
    // Generally, it will work on drivers, but it is not technically correct.

    // To quote GL_EXT_nonuniform_qualifier:

    /*
        Only some operations discussed in Chapter 5 (Operators and Expressions)
        can be applied to nonuniform value(s) and still yield a result that is
        nonuniform. The operations that do so are listed below. When a
        nonuniform value is operated on with one of these operators (regardless
        of whether any and other operands are nonuniform), the result is
        implicitly nonuniform:

        ...

        * Structure and Array Operations in Section 5.7, except for the length
        method and assignment operator.

        ...

        Constructors and builtin functions, which all have return types that
        are not qualified by nonuniformEXT, will not generate nonuniform results.
        Shaders need to use the constructor syntax (or assignment to a
        nonuniformEXT-qualified variable) to re-add the nonuniformEXT qualifier
        to the result of builtin functions.

        ...
    */

    // sampler2D is such a constructor, so we must add nonuniformEXT afterwards.
    out_frag_color = texture(nonuniformEXT(sampler2D(Textures[in_texture_index], ImmutableSampler)), in_uv);
    // For all other use cases of nonuniformEXT however, we can write code like:
    // uniform UBO { vec4 data; } UBOs[]; vec4 foo = UBOs[nonuniformEXT(index)].data;
    // buffer SSBO { vec4 data; } SSBOs[]; vec4 foo = SSBOs[nonuniformEXT(index)].data;
    // uniform sampler2D Tex[]; vec4 foo = texture(Tex[nonuniformEXT(index)], uv);
    // uniform uimage2D Img[]; uint count = imageAtomicAdd(Img[nonuniformEXT(index)], uv, val);
    // etc. The nonuniform qualifier will propagate up to the final argument which is used in the load/store or atomic operation.

    // Using implicit LOD with nonuniformEXT can be spicy! If the threads in a quad do not have the same index,
    // LOD might not be computed correctly. The quadDivergentImplicitLOD property lets you know if it will work.
    // In this case however, it is completely fine, since the helper lanes in a quad must come from the same primitive,
    // which all have the same flat fragment input.

    // You might consider using subgroup operations to implement nonuniformEXT on your own.
    // This is technically out of spec, since the SPIR-V specification states that to avoid nonuniformEXT,
    // the shader must guarantee that the index is "dynamically uniform".
    // "Dynamically uniform" means the value is the same across all invocations in an "invocation group".
    // The invocation group is defined to be all invocations (threads) for:
    // - An entire draw command (for graphics)
    // - A single workgroup (for compute).
    // Avoiding nonuniformEXT with clever programming is far more likely to succeed when writing compute shaders,
    // since the workgroup boundary serves as a much easier boundary to control than entire draw commands.
    // It is often possible to match workgroup to subgroup 1:1, unlike graphics where you cannot control how
    // quads are packed into subgroups at all.
    // The recommended approach here is to just let the compiler do its thing to avoid horrible bugs in the future.
}
