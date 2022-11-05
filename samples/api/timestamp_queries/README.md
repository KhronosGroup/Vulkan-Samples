<!--
- Copyright (c) 2022, Sascha Willems
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

# Timestamp queries

This tutorial, along with the accompanying example code, shows how to use timestamp queries to measure timings on the GPU.

The sample, based on the HDR one, does multiple render passes and will use timestamp queries to get GPU timings for the different render passes. This is done by writing GPU timestamps at certain points within a command buffer. These can then be read on the host and used for approximate profiling and to e.g. improve performance where needed.

## Introduction

Vulkan offers several [query types](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#queries) that allow you to query different types of information from the GPU. One such query type is the [timestamp query](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#queries-timestamps).

This provides your application with a mechanism to time the execution of commands on the GPU. As with the other query types, a query pool is then used to either directly fetch or copy over the results to the host.

## A few important notes on timestamp queries

It's important to know that timestamp queries differ greatly from how timing can be done on the CPU with e.g. the high performance counter. This is mostly due to how a GPU's dispatches, overlaps and finishes work across different stages of the pipeline. So while technically you can specify any pipeline stage at which the timestamp should be written, a lot of stage combinations and orderings won't give meaningful result. This also means that you you can't compare timestamps taken on different queues.

So while it may may sound reasonable to write timestamps for the vertex and fragment shader stage directly one after another, that will usually not return meaningful results due to how the GPU works.

And so for this example, we take the same approach as some popular CPU/GPU profilers by only using the top and bottom stages of the pipeline. This combination is known to give proper approximate timing results on most GPUs.

## Checking for support

Not all GPUs support timestamp queries, so before using them we need to make sure that they can be used. This differs slightly from checking other features with a simple `VkBool`. Here we need to check if the `timestampPeriod` limit of the physical device is greater than zero. If that's the case, timestamp queries are supported:

```cpp
VkPhysicalDeviceLimits device_limits = device->get_gpu().get_properties().limits;
if (device_limits.timestampPeriod == 0)
{
	throw std::runtime_error{"The selected device does not support timestamp queries!"};
}
```

Another limit we need to check is `timestampComputeAndGraphics`. If this is `VK_TRUE`, all graphics and compute pipelines support timestamp queries and the above check is sufficient. If not, we need to check if the queue we want to use supports timestamps:

```cpp
if (!device_limits.timestampComputeAndGraphics)
{
	// Check if the graphics queue used in this sample supports time stamps
	VkQueueFamilyProperties graphics_queue_family_properties = device->get_suitable_graphics_queue().get_properties();
	if (graphics_queue_family_properties.timestampValidBits == 0)
	{
		throw std::runtime_error{"The selected graphics queue family does not support timestamp queries!"};
	}
}
```

## Creating the query pool

As with all query types, we first need to create a pool for the timestamp queries. This is used to store and read back the results (see `prepare_time_stamp_queries`):

```cpp
VkQueryPoolCreateInfo query_pool_info{};
query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
query_pool_info.queryCount = static_cast<uint32_t>(time_stamps.size());
VK_CHECK(vkCreateQueryPool(device->get_handle(), &query_pool_info, nullptr, &query_pool_timestamps));
```

The interesting parts are the `queryType`, which we set to `VK_QUERY_TYPE_TIMESTAMP` for using timestamp queries and the `queryCount`, which is the maximum number of the the timestamp query result this pool can store.
 
For this sample we'll be using 6 time points, one for the start and one for the end of three render passes. 

## Resetting the query pool

Before we can start writing data to the query pool, we need to reset it. When using Vulkan 1.0 or 1.1, this requires us to enable the `VK_EXT_host_query_reset` extension:

```cpp
add_device_extension(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
```

With using Vulkan 1.2 this extension has become part of the core and we won't have to manually enable it.

Independent of this, we also need to enable the `hostQueryReset` physical device feature:

```cpp
auto &requested_extension_features= gpu.request_extension_features<VkPhysicalDeviceHostQueryResetFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT);
requested_extension_features.hostQueryReset = VK_TRUE;
```

With features and extensions properly enabled, we can now reset the pool at the start of the command buffer, before writing the first timestamp. This is done using `vkCmdResetQueryPool`:

```cpp
...
vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info);
vkCmdResetQueryPool(draw_cmd_buffers[i], query_pool_timestamps, 0, static_cast<uint32_t>(time_stamps.size()));
```
------

## Writing time stamps

Unlike getting CPU side timing information that can be queried immediately, with GPU time stamps we need to tell the implementation inside a command buffer when/where to write timestamps instead. The results are then fetched afterwards (see below).

This is done inside the command buffer with `vkCmdWriteTimestamp`. This function will request a timestamp to be written from the GPU for a certain pipeline stage and write that value to memory.

The most interesting part of calling this function is the `pipelineStage` argument. As noted earlier, it's technically possible to use any pipeline stage in here, not all pipeline stages will yield proper results due to how GPUs overlap work. It's also important to note that not all implementations are able to latch timers at all pipeline stages (e.g. if they don't have hardware that maps to a given stage) and may return timers at a later pipeline stage instead.
 
Calling this function also defines an execution dependency similar to a barrier on all commands that were submitted before it.

```cpp
vkCmdWriteTimestamp(draw_cmd_buffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, query_pool_timestamps, 0);
// Do some work
for (int i = 0; i < draw_call_count; i++) {
	vkCmdDraw(...);
}
vkCmdWriteTimestamp(draw_cmd_buffers[i], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_timestamps, 1);
```

To measure GPU times for the draw calls(s) we first tell the GPU to write a timestamp at the `VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT` pipeline stage. This is not a real pipeline stage (as in e.g. the vertex or fragment stages) but a special constant that tells the GPU to write the timestamp when all previous commands have been processed by the GPU's command processor. This ensures that we get a timestamp right before starting on the draw calls we want to measure, which will be the base for calculating our delta time.

The second timestamp is written at the `VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT` pipeline stage. Once again this is not a real pipeline stage, but it again tells the GPU to write the timestamp after all work has been finished. 

## Getting the results

Reading back the results can be done in two ways:

- Copy the results into a `VkBuffer` inside the command buffer using `vkCmdCopyQueryPoolResults`
- Get the results after the command buffer has finished executing using `vkGetQueryPoolResults`

For our sample we'll use option two (see `get_time_stamp_results`):

```cpp
vkQueueSubmit();
...
// The number of timestamps changes if the bloom pass is disabled
uint32_t count = bloom ? time_stamps.size() : time_stamps.size() - 2;

vkGetQueryPoolResults(
	device->get_handle(),
	query_pool_timestamps,
	0,
	count,
	time_stamps.size() * sizeof(uint64_t),
	time_stamps.data(),
	sizeof(uint64_t),
	VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
```

Most arguments are straightforward, e.g. where the data will be copied to (the `time_stamps` vector). The important part here are the `VK_QUERY_RESULT_` flags used here.

`VK_QUERY_RESULT_64_BIT` will tell the api that we want to get the results as 64 bit values. Without this flag, we would only get 32 bit values. And since timestamp queries can operate in nanoseconds, only using 32 bits could result into an overflow. E.g. if your device has a `timestampPeriod` of 1, so that one increment in the result maps to exactly one nanosecond, with 32 bit precision you'd run into such an overflow after only about 0.43 seconds.

The `VK_QUERY_RESULT_WAIT_BIT` bit then tells the api to wait for all results to be available. So when using this flag the values written to our `time_stamps` vector is guaranteed to be available after calling `vkGetQueryPoolResults`. This is fine for our use-case where we want to immediately access the results, but may introduce unnecessary stalls in other scenarios. An alternative is using the `VK_QUERY_RESULT_WITH_AVAILABILITY_BIT`, which will let you poll the availability of the results and defer writing new timestamps until the results are available.

## Interpreting the results

After we have read back the results to the host, we are ready to interpret them. E.g. for displaying them in a user interface.

The results we got back do not actually contain a time value, but rather a number of "ticks". So to get the actual time value we need to translate these values first.

This is done using `timestampPeriod` limit of the physical device. It contains the number of nanoseconds it takes for a timestamp query value to be increased by 1 ("tick").

In our sample, we want to display the delta between two timestamps in milliseconds, so in addition to the above rule we also multiply the value accordingly.

```cpp
VkPhysicalDeviceLimits device_limits = device->get_gpu().get_properties().limits;
float delta_in_ms = float(time_stamps[1] - time_stamps[0]) * device_limits.timestampPeriod / 1000000.0f;
```

## vkCmdWriteTimestamp2

The [VK_KHR_synchronization2](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_synchronization2.html) extension introduced `vkCmdWriteTimestamp2`. This is pretty much the same as the `vkCmdWriteTimestamp` function used in this sample, but adds support for some additional pipeline stages using `VkPipelineStageFlags2`.

## Verdict

Even though timestamp queries are limited due to how a GPU works, they can still be useful for profiling and finding performance GPU bottlenecks.