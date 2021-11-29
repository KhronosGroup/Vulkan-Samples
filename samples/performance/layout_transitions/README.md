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
### Layout transitions<br/>
Vulkan requires the application to manage image layouts, so that all render pass attachments are in the correct layout when the render pass begins. This is usually done using pipeline barriers or the `initialLayout` and `finalLayout` parameters of the render pass. If the rendering pipeline is complex, transitioning each image to its correct layout is not trivial, as it requires some sort of state tracking. If previous image contents are not needed, there is an easy way out, that is setting `oldLayout`/`initialLayout` to `VK_IMAGE_LAYOUT_UNDEFINED`. While this is functionally correct, it can have performance implications as it may prevent the GPU from performing some optimizations. This sample will cover an example of such optimizations and how to avoid the performance overhead from using sub-optimal layouts.

- 🎓 [Choosing the correct layout when transitioning images](./layout_transitions_tutorial.md)
