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
### Render passes<br/>
Vulkan render-passes use attachments to describe input and output render targets. This sample shows how loading and storing attachments might affect performance on mobile. During the creation of a render-pass, you can specify various color attachments and a depth-stencil attachment. Each of those is described by a [`VkAttachmentDescription`](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAttachmentDescription.html) struct, which contains attributes to specify the [load operation](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAttachmentLoadOp.html) (`loadOp`) and the [store operation](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAttachmentStoreOp.html) (`storeOp`). This sample lets you choose between different combinations of these operations at runtime.

- 🎓 [Appropriate use of load/store operations, and use of transient attachments](./render_passes_tutorial.md)
