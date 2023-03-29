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

# Basic HLSL hardware accelerated ray tracing<br/>

## Overview

This sample is based on raytracing_basic sample. The only difference is using precompiled HLSL shaders instead of GLSL shaders

## HLSL shaders

HLSL shaders can be compiled by Microsoft’s DirectX shader compiler (DXC). For compiling raytracing shaders above commend can be used:
dxc.exe -fspv-target-env=vulkan1.1spirv1.4 -spirv -T lib_6_3 <path_to_raytracing_shader> -Fo <output_path>