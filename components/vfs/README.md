<!--
- Copyright (c) 2021-2022, Arm Limited and Contributors
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

# Virtual Filesystem

In most platform specific projects you may be able to load assets using a singular API and call it a day. However, when creating a cross-platform project, it is almost impossible to use just one API. In this case, a Virtual Filesystem may become useful. The idea is to load assets as usual from paths but to map these paths differently on different OS's using different libraries to load a files information.

A path for this virtual filesystem must always start with a `/` as this is what is expected by the filesystem implementations. Using this standard means that implementations do not need to sanitize or correct paths. This VFS also allows for mounting points which means the following paths are equivalent when `/scenes` is mounted on `/assets/scenes`.

```
/assets/scenes/sponza.gltf
/scenes/sponza.gltf
```

For now, the filesystem is treated as a singleton. It would be better down the road to not use this singleton and create Asset Loaders which take a reference to a `RootFileSystem`. This means that the applications can define different file systems for different purposes. For instance, if a part of the project must only be able to access temporary files, then a filesystem with only temporary file access should be passed. The same applies to Asset Loaders which must only read files and not write. Granular/Scoped file access like this is of great importance in applications which may load assets or scripts not controlled by the developer - games which load mods.

## Other Projects

PhysicsFS is a library to provide abstract access to various archives. It is intended for use in video games, and the design was somewhat inspired by Quake 3's file subsystem.

[icculus.org/physfs](https://icculus.org/physfs/)

NVIDIA Donut Engine VFS. Donut is a real-time rendering framework built by NVIDIA DevTech for use in various prototype renderers and code samples. This filesystem was used for inspiration for this VFS implementation.

[Donut VFS](https://github.com/NVIDIAGameWorks/donut/tree/main/include/donut/core/vfs)