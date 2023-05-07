<!--
- Copyright (c) 2020-2023, The Khronos Group 
-
- SPDX-License-Identifier: Apache-2.0
-
- Licensed under the Apache License, Version 2.0 the "License";
- you may not use this file except in compliance with the License.
- You may obtain a copy of the License at
-
-     http://www.apache.org/licenses/LICENSE-2.0
-
- Unless required by applicable law or agreed to in writing, software
- distributed under the License is distributed on an "AS IS" BASIS,
- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
- See the License for the specific language governing permissions and
- limitations under the License.
-
-->
### Basic ray queries

**Extensions**: [```VK_KHR_ray_query```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_ray_query), [```VK_KHR_acceleration_structure```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_acceleration_structure)

Render a sponza scene using the ray query extension. Shows how to set up all data structures required for ray queries, including the bottom and top level acceleration structures for the geometry and a standard vertex/fragment shader pipeline. Shadows are cast dynamically by ray queries being cast by the fragment shader.
