<!--
- Copyright (c) 2020-2021, The Khronos Group
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

### Extended hardware accelerated ray tracing<br/>
**Extensions**: [```VK_KHR_ray_tracing_pipeline```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_ray_tracing_pipeline), [```VK_KHR_acceleration_structure```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_KHR_acceleration_structure)<br/>
Render Sponza with Ambient Occlusion.  Place a vase in center.  Generate a particle fire that 
demonstrates the TLAS (Top Level Acceleration Structure) animation for the same underlying geometry.
Procedurally generate a transparent quad and deform the geometry of the quad in the BLAS (Bottom Level Acceleration 
Structure) to demonstrate how to animate with deforming geometry.
Shows how to rebuild the acceleration structure and when to set it to fast rebuild vs fast traversal.

- 🎓 [Ray-tracing: Extended features and dynamic objects](./tutorial.md)
