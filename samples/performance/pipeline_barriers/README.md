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
### Pipeline barriers<br/>
Vulkan gives the application significant control over memory access for resources. Pipeline barriers are particularly convenient for synchronizing memory accesses between render passes. Having barriers is required whenever there is a memory dependency - the application should not assume that render passes are executed in order. However, having too many or too strict barriers can affect the application's performance. This sample will cover how to set up pipeline barriers efficiently, with a focus on pipeline stages.

- 🎓 [Using pipeline barriers efficiently](./pipeline_barriers_tutorial.md)
