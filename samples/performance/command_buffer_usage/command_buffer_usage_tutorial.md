<!--
- Copyright (c) 2019, Arm Limited and Contributors
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

# Command Buffer Usage

## Overview

Vulkan provides different ways to manage and allocate command buffers. This sample compares them and demonstrates the best approach.

- [Allocate and Free](#allocate-and-free)
- [Resetting individual command buffers](#resetting-individual-command-buffers)
- [Resetting the command pool](#resetting-the-command-pool)

## Allocate and free

Command buffers are allocated from a command pool with [vkAllocateCommandBuffers](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkAllocateCommandBuffers.html).
They can then be recorded and submitted to a queue for the Vulkan device to execute them.

A possible approach to managing the command buffers for each frame in our application would be to free them once they are executed, using [vkFreeCommandBuffers](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkFreeCommandBuffers.html).

The command pool will not automatically recycle memory from deleted command buffers if the command pool was created without the [RESET_COMMAND_BUFFER_BIT](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandPoolCreateFlagBits.html) flag.
This flag however will force separate internal allocators to be used for each command buffer in the pool, which can increase CPU overhead compared to a single pool reset.

This is the worst-performing method of managing command buffers as it involves a significant CPU overhead for allocating and freeing memory frequently.
The sample shows how to use the framework to follow this approach and profile its performance.

![Allocate and Free](images/allocate_and_free.jpg)

Rather than freeing and re-allocating the memory used by a command buffer, it is more efficient to recycle it for recording new commands.
There are two ways of resetting a command buffer: individually, with [vkResetCommandBuffer](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkResetCommandBuffer.html), or indirectly by resetting the command pool with [vkResetCommandPool](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkResetCommandPool.html).

## Resetting individual command buffers

In order to reset command buffers individually with [vkResetCommandBuffer](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkResetCommandBuffer.html), the pool must have been created with the [RESET_COMMAND_BUFFER_BIT](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandPoolCreateFlagBits.html) flag set.
The buffer will then return to a recordable state and the command pool can reuse the memory it allocated for it.

However frequent calls to [vkResetCommandBuffer](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkResetCommandBuffer.html) are more expensive than a command pool reset.

![Reset Buffers](images/reset_buffers.jpg)

## Resetting the command pool

Resetting the pool with [vkResetCommandPool](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkResetCommandPool.html) automatically resets all the command buffers allocated by it.
Doing this periodically will allow the pool to reuse the memory allocated for command buffers with lower CPU overhead.

To reset the pool the flag [RESET_COMMAND_BUFFER_BIT](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandPoolCreateFlagBits.html) is _not_ required, and it is actually better to avoid it since it prevents it from using a single large allocator for all buffers in the pool thus increasing memory overhead.

![Reset Pool](images/reset_pool.jpg)

## The sample

Since the scene rendered is simple it can use a single command buffer to record all drawing operations. This sample also provides an option to use secondary command buffers instead, one for each mesh.
The latter approach is definitely not recommended.
It is expected that applications will need to use secondary command buffers, typically to allow multi-threaded command buffer construction.
However they should be used sparingly since their invocations are expensive.
In this particular sample, using secondary command buffers increases the number of required command buffers to about 500.
This causes the application to become CPU bound and makes the differences between the described memory allocation approaches more pronounced.

All command buffers in this sample are initialized with the [ONE_TIME_SUBMIT_BIT](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandBufferUsageFlagBits.html) flag set. This indicates to the driver that the buffer will not be re-submitted after execution, and allows it to optimize accordingly.
Performance may be reduced if the [SIMULTANEOUS_USE_BIT](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandBufferUsageFlagBits.html) flag is set instead.

This sample provides options to try the three different approaches to command buffer management described above, as well as enabling and disabling secondary command buffers to increase the CPU overhead and intensify their differences.
These are relatively obvious directly on the device by monitoring frame time.

Since the application is CPU bound, the [Android Profiler](https://developer.android.com/studio/profile/android-profiler) is a helpful tool to analyze the differences in performance. As expected, most of the time goes into the CommandPool framework functions: `request_command_buffer` and `reset`. These handle the different modes of operation exposed by this sample.

![Android Profiler: Allocate and Free](images/android_profiler_allocate_and_free.png)
_Android Profiler capture: use the Flame Chart to visualize which functions take the most time in the capture. Filter for command buffer functions, and use the tooltips to find out how much time out of the overall capture was used by each function._

Capturing the C++ calls this way allows us to determine how much each function contributed to the overall running time. The results are captured in the table below.

Mode | Request + Reset command buffers (ms) | Total capture (ms) | Contribution
---|---|---|---
Reset pool | 53.3 | 11 877 | 0.45 %
Reset buffers | 140.29 | 12 087 | 1.16 %
Allocate and free | 3 319.25 | 11 513 | 28.8 %

In this application the differences between individual reset and pool reset are more subtle, but allocating and freeing buffers are clearly the bottleneck in the worst performing case.

## Further reading

 - [Command Buffer Allocation and Management](https://vulkan.lunarg.com/doc/view/1.0.33.0/linux/vkspec.chunked/ch05s02.html)
 - [Command Buffer Lifecycle](https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/chap5.html#commandbuffers-lifecycle)

## Best-practice summary

**Do**

* Set [ONE_TIME_SUBMIT_BIT](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandBufferUsageFlagBits.html) if you are not going to reuse the command buffer.
* Periodically call [vkResetCommandPool()](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkResetCommandPool.html) to release the memory if you are not reusing command buffers.
* Minimize the number of secondary command buffer invocations used per frame.

**Don't**

* Set [RESET_COMMAND_BUFFER_BIT](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandPoolCreateFlagBits.html) if you only need to free the whole pool. If the bit is not set, some implementations might use a single large allocator for the pool, reducing memory management overhead.
* Call [vkResetCommandBuffer()](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkResetCommandBuffer.html) on a high frequency call path.

**Impact**

* Increased CPU load will be incurred with secondary command buffers.
* Increased CPU load will be incurred if command pool creation and command buffer begin flags are not used appropriately.
* Increased CPU overhead if command buffer resets are too frequent.
* Increased memory usage until a manual command pool reset is triggered.

**Debugging**

* Evaluate every use of any command buffer flag other than [ONE_TIME_SUBMIT_BIT](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandBufferUsageFlagBits.html), and review whether it's a necessary use of the flag combination.
* Evaluate every use of [vkResetCommandBuffer()](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkResetCommandBuffer.html) and see if it could be replaced with [vkResetCommandPool()](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkResetCommandPool.html) instead.
