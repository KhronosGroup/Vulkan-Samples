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
### Constant data<br/>
The Vulkan API exposes a few different ways in which we can send uniform data into our shaders. There are enough methods that it raises the question "Which one is fastest?", and more often than not the answer is "It depends". The main issue for developers is that the fastest methods may differ between the various vendors, so often there is no "one size fits all" solution. This sample aims to highlight this issue, and help move the Vulkan ecosystem to a point where we are better equipped to solve this for developers. This is done by having an interactive way to toggle different constant data methods that the Vulkan API expose to us. This can then be run on a platform of the developers choice to see the performance implications that each of them bring.

- 🎓 [Sending constant data to the shaders](./constant_data_tutorial.md)
