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
### OpenGL interoperability

**Extensions**: [```VK_KHR_external_memory```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_external_memory.html), [```VK_KHR_external_semaphore```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_external_semaphore.html)

Render a procedural image using OpenGL and incorporate that rendered content into a Vulkan scene.  Demonstrates using the same backing memory for a texture in both OpenGL and Vulkan and how to synchronize the APIs using shared semaphores and barriers.
