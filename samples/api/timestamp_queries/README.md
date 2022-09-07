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

The sample, based on the HDR one, does multiple render passes and will use timestamp queries to get GPU timings for the different render passes. This is done by writing GPU timestamps at certain points within a command buffer. These can then be read on the host and used for light profiling and to e.g. improve performance where needed.

## Introduction

Vulkan offers several [query types](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#queries) that allow you to query different types of information from the GPU. One such query type is the [timestamp query](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#queries-timestamps).

This provides your application with a mechanism to time the execution of commands on the GPU. As with the other query types, a query pool is then used to either directly fetch or copy over the results to the host.

## A note on timestamp queries

It's important to know that timestamp queries differ greatly from how timing can be done on the CPU with e.g. the high performance counter on Windows. This is mostly due to how a GPU's dispatches, overlaps and finishes work across different stages of the pipeline. So while in theory you can specify any pipeline stage at which the timestamp should be written, a lot of stage combinations and orderings won't give meaningful result.

So while it may sound reasonable to e.g. write timestamps for the vertex and fragment shader stage one after another, it will not give useful results.

And so for this example, we take the same approach as some popular CPU/GPU profilers by only using the top and bottom stages of the pipeline. This combination is known to give proper timing results on most GPUs.

## Checking for support

Not all devices support timestamp queries, so before using them we need to make sure that they can be used. This differs slightly from checking other features with a simple `VkBool`. Here we need to check if the `timestampPeriod` limit of the physical device is greater than zero. If that's the case, timestamp queries are supported:

```cpp
// Check if the selected device supports timestamps. A value of zero means no support.
VkPhysicalDeviceLimits device_limits = device->get_gpu().get_properties().limits;
if (device_limits.timestampPeriod == 0)
{
    throw std::runtime_error{"The selected device does not support timestamp queries!"};
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

// @todo: talk about pipeline stages

// @todo: When vkCmdWriteTimestamp is submitted to a queue, it defines an execution dependency on
commands that were submitted before it, and writes a timestamp to a query pool.

If an implementation is unable to detect completion and latch the timer
immediately after stage has completed, it may instead do so at any logically later
stage

```cpp
vkCmdWriteTimestamp(draw_cmd_buffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, query_pool_timestamps, 0);
// Do some work
vkCmdWriteTimestamp(draw_cmd_buffers[i], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_timestamps, 1);
```

@todo: maybe insert image with pipeline stages from top to bottom

## Getting the results

Reading back the results can be done in two ways:

- Copy the results into a `VkBuffer` inside the command buffer using `vkCmdCopyQueryPoolResults`
- Get the results after the command buffer has finished executing using `vkGetQueryPoolResults`

For our sample we'll use option two (see `get_time_stamp_results`):

@todo: another option is to do a bufer copy in the command buffer, but that requires a dedicated buffer.

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

Most arguments are straightforward, e.g. where the data will be copied to (the `time_stamps` vector). The important part here are the `VK_QUERY_RESULT´ flags used here.

`VK_QUERY_RESULT_64_BIT` will tell the api that we want to get the results as 64 bit values. Without this flag, we would only get 32 bit values. And since timestamp queries operate in nanoseconds, only using 32 bits could result into an overflow. E.g. if your device has a `timestampPeriod` of 1, so that one increment in the result maps to exactly one nanosecond, with 32 bit precision you'd run into such an overflow after only about 0.43 seconds.

The `VK_QUERY_RESULT_WAIT_BIT` bit then tells the api to wait for all results to be available. So when using this flag the values written to our `time_stamps` vector is guaranteed to be available after calling `vkGetQueryPoolResults`. This is fine for our use-case where we want to immediately access the results, but may introduce unnecessary stalls in other scenarios. An alternative is using the `VK_QUERY_RESULT_WITH_AVAILABILITY_BIT`, which will let you poll the availability of the results and defer writing new timestamps until the results are available.

## Interpreting the results

After we have read back the results to the host, we are ready to interprete them. E.g. for displaying them in a user interface.

The results we got back do not actually contain a time value, but rather a number of "ticks". So to get the actual time value we need to translate these values first.

This is done using `timestampPeriod` limit of the pyhsical device. It contains the number of nanseconds it takes for a timestamp query value to be increased by 1 ("tick").

So if the `timestampPeriod` limit for your device is 1, a query result of 10 means 10 nanoseconds. If the device limit is 10, then that same query result means 100 nanoseconds. // @todo: check if math is correct.

In our sample, we want to display the delta between two timestamps in milliseconds, so in addition to the above rule we also multiply the value accordingly.

```cpp
VkPhysicalDeviceLimits device_limits = device->get_gpu().get_properties().limits;
float delta_in_ms = float(time_stamps[1] - time_stamps[0]) * device_limits.timestampPeriod / 1000000.0f;
```

## Misc

From the spec

`timestampComputeAndGraphics` specifies support for timestamps on all graphics and compute queues. If this limit is set to VK_TRUE, all queues that advertise the VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT in the VkQueueFamilyProperties::queueFlags support VkQueueFamilyProperties::timestampValidBits of at least 36. See Timestamp Queries.

`timestampValidBits` is the unsigned integer count of meaningful bits in the timestamps written via vkCmdWriteTimestamp2 or vkCmdWriteTimestamp. The valid range for the count is 36..64 bits, or a value of 0, indicating no support for timestamps. Bits outside the valid range are guaranteed to be zeros.
(from VkQueueFamilyProperties)

## Verdict

Can be used for light and coarse profiling, but with limitations.

