<!--
- Copyright (c) 2019-2021, The Khronos Group
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
### Sub passes<br/>
Vulkan introduces the concept of _subpasses_ to subdivide a single [render pass](../../performance/render_passes/render_passes_tutorial.md) into separate logical phases. The benefit of using subpasses over multiple render passes is that a GPU is able to perform various optimizations. Tile-based renderers, for example, can take advantage of tile memory, which being on chip is decisively faster than external memory, potentially saving a considerable amount of bandwidth.

  - 🎓 [Benefits of subpasses over multiple render passes, use of transient attachments, and G-buffer recommended size](./subpasses_tutorial.md)
