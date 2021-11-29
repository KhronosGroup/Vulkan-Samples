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
### Descriptor management<br/>
An application using Vulkan will have to implement a system to manage descriptor pools and sets. The most straightforward and flexible approach is to re-create them for each frame, but doing so might be very inefficient, especially on mobile platforms. The problem of descriptor management is intertwined with that of buffer management, that is choosing how to pack data in `VkBuffer` objects. This sample will explore a few options to improve both descriptor and buffer management.

- 🎓 [Descriptor and buffer management](./descriptor_management_tutorial.md)
