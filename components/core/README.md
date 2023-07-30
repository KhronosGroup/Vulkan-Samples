<!--
- Copyright (c) 2023, Thomas Atkinson
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

# Core

Core is a collection of pure interfaces or small utilities which are used across the project. The core component is the only component which does not follow the component pattern in its entirety. The only major difference between core and other components is the header prefix used is `core/<sub_dir>` instead of `components/core/<sub_dir>`.

## Platform

A Platform is the name we have given to the physical hardware and operating system that the project is executing on. We support multiple platforms which can be identified by the following defines

- `PLATFORM__ANDROID`
- `PLATFORM__WINDOWS`
- `PLATFORM__LINUX_D2D`
- `PLATFORM__LINUX`
- `PLATFORM__MACOS`

Using these platforms should be as transparent as possible to a sample. Components on the other hand may add platform specific code paths if required.

An application can create a cross platform entrypoint by using the `CUSTOM_MAIN(context_name)` macro

```cpp

#include <core/platform/entrypoint.hpp>

CUSTOM_MAIN(context)
{
    context.arguments();
    context.external_storage_directory();
    context.temp_directory();

    // Components using platform specific contexts
    FileSystem fs = FileSystem::from_context(context);
}

```

## Utilities

- Error - A collection of error handling macros
- Hash - A collection of hashing functions
- Strings - A collection of string utilities
