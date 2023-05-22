<!--
- Copyright (c) 2019-2023, The Khronos Group
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
### Fragment shading rate

**Extension**: [```VK_KHR_fragment_shading_rate```](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_fragment_shading_rate.html)

Uses a special framebuffer attachment to control fragment shading rates for different framebuffer regions. This allows explicit control over the number of fragment shader invocations for each pixel covered by a fragment, which is e.g. useful for foveated rendering.

