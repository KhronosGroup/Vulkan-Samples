<!--
- Copyright (c) 2023, Holochip Corporation
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

# Calibrated Timestamps

This sample demonstrates how to incorporate Vulkan ```VK_EXT_calibrated_timestamps``` extension. The calibrated
timestamps is different from timestamp queries, and compares to the timestamp queries, it profiles any given portion of
a code, unlike timestamp queries, which only profiles an entire graphic queue. Hence, calibrated timestamps provides
more flexibilities, and offers more customizations.

## Overview

To enable the ```VK_EXT_calibrated_timestamps``` extension, ```VK_KHR_get_physical_device_properties2``` must be added
as an instance extension, and ```VK_EXT_calibrated_timestamps``` added as a device extension. Where:

```cpp
CalibratedTimestamps ::CalibratedTimestamps()
{
	title = "Calibrated Timestamps";

	// Add instance extensions required for calibrated timestamps
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	// NOTICE THAT: calibrated timestamps is a DEVICE extension!
	add_device_extension(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);
}
```

## Introduction

This sample is built upon the framework of the Vulkan Sample ```HDR```. Where, users can select object type, activate or
deactivate skybox, and the bloom effect. And those changes will apply to ```build_command_buffers()```, which is
profiled by calibrated timestamps.

##                                     * Time domain, timestamp, timestamp period, and max deviation

This is an optional section.

One shall notice that, a timestamp **DOES NOT** return an actual time reading. A timestamp only indicates "how many
units of timestamp periods have passed referring to a **FIXED** point on its timeline". The timestamp period, however,
is measured by real-time unit, and is commonly measured in nanoseconds. In general, one must take two timestamps in
order to measure the time elapsed within a block of code. For instance:

```cpp
uint64_t myFirstTimestamp = getMyTimestamps(); // marks the 1st timestamp
{
    /* A block of code */
}
uint64_t mySecondTimestamp = getMytimestamps(); // marks the 2nd timestamp

float timestamps_period = getMyTimestampsPeriod(); // gets the timestamps period from the host

// Total time elapsed from the block of code
float timeElapsed = static_cast<float>(mySecondTimestamp - myFirstTimestamp) * timestamps_period;
```

Furthermore, reference points of each time domains are different, and the measurement of their associated timestamp
periods may vary. Additional helps to increase the precision of timestamps are provided by max deviations, and the
uncertainty of its associated timestamps are described.

## Get time domain and timestamps

A list of time domains can be extracted by using ```vkGetPhysicalDeviceCalibrateableTimeDomainsEXT``` call. And the
Vulkan time domain is defined by the enum class```VkTimeDomainEXT```, where:

1. ```VK_TIME_DOMAIN_DEVICE_EXT```, enum value: 0
2. ```VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT```, enum value: 1
3. ```VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT```, enum value: 2
4. ```VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT```, enum value: 3

To interoperate it, a helper function ```time_domain_to_string()``` function is introduced, where:

```cpp
std::string time_domain_to_string(VkTimeDomainEXT input_time_domain)
{
	switch (input_time_domain)
	{
		case VK_TIME_DOMAIN_DEVICE_EXT:
			return "device time domain";
		case VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT:
			return "clock monotonic time domain";
		case VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT:
			return "clock monotonic raw time domain";
		case VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT:
			return "query performance time domain";
		default:
			return "unknown time domain";
	}
}
```

And the function ```get_time_domains()``` is created:

```cpp
void CalibratedTimestamps::update_time_domains()
{
	// Initialize time domain count:
	uint32_t time_domain_count = 0;
	// Update time domain count:
	VkResult result = vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(get_device().get_gpu().get_handle(), &time_domain_count, nullptr);

	if (result == VK_SUCCESS)
	{
		// Initialize time domain vector
		time_domains.clear();
		// Resize time domains vector:
		time_domains.resize(static_cast<int>(time_domain_count));        // this needs static cast to int
		// Update time_domain vector:
		result = vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(get_device().get_gpu().get_handle(), &time_domain_count, time_domains.data());
	}

	if (time_domain_count > 0)
	{
		for (VkTimeDomainEXT time_domain : time_domains)
		{
			// Initialize in-scope time stamp info variable:
			VkCalibratedTimestampInfoEXT timestamp_info{};

			// Configure timestamp info variable:
			timestamp_info.sType      = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
			timestamp_info.pNext      = nullptr;
			timestamp_info.timeDomain = time_domain;

			// Push-back timestamp info to timestamps info vector:
			timestamps_info.push_back(timestamp_info);
		}

		// Resize time stamps vector
		timestamps.resize(static_cast<int>(time_domain_count));
		// Resize max deviations vector
		max_deviations.resize(static_cast<int>(time_domain_count));
	}

	// Time domain is successfully updated:
	is_time_domain_updated = ((result == VK_SUCCESS) && (time_domain_count > 0));
}
```

Notice that, a ```VkCalibratedTimestampInfoEXT``` variable ```timestamp_info``` must be created. Its ```sType```
must be ```VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT```, its ```pNext``` is connected to a ```nullptr```, and
its ```timeDomain``` must be connected to either the reference of a ```VkTimeDomainEXT``` variable, or the reference of
a vector of ```VkTimeDomainEXT``` variables.

```get_time_domain()``` registers all time domains, and flags its success using ```is_time_domain_updated```. The
function ```get_timestamps()``` then extracts all associated timestamps and max deviations using  
```vkGetCalibratedTimestampsEXT``` call, where:

```cpp
void CalibratedTimestamps::get_timestamps()
{
	// Ensures that time domain exists
	if (is_time_domain_updated)
	{
		// Get calibrated timestamps:
		VkResult result = vkGetCalibratedTimestampsEXT(get_device().get_handle(), time_domains.size(), timestamps_info.data(), timestamps.data(), max_deviations.data());

		// Timestamps is successfully updated:
		is_timestamp_updated = (result == VK_SUCCESS);
	}
}
```

The boolean ```is_timestamp_updated``` flags its success.

##                            * Get device time domain

This is an optional section.

Since calibrated timestamps extracts all time domains and timestamps, it is a good practice to put such feature in use.
The function ```get_device_time_domain()``` is therefore, created to select the device time domain and its associated
timestamps, where:

```cpp
void CalibratedTimestamps::get_device_time_domain()
{
	init_time_domains_and_timestamps();
	get_time_domains();

	if (is_time_domain_updated && (time_domains.size() > 1))
	{
		auto iterator_device = std::find(time_domains.begin(), time_domains.end(), VK_TIME_DOMAIN_DEVICE_EXT);

		int device_index = static_cast<int>(iterator_device - time_domains.begin());

		selected_time_domain.index         = device_index;
		selected_time_domain.timeDomainEXT = time_domains[device_index];
	}
}
```

```init_time_domains_and_timestamps()``` simply resets time domain and timestamps, such that:

```cpp
void CalibratedTimestamps::init_time_domains_and_timestamps()
{
	is_time_domain_updated = false;
	is_timestamp_updated   = false;

	time_domains.clear();
	timestamps.clear();
	max_deviations.clear();
}
```

And ```selected_time_domain``` is a structure created to restore the associated data:

```cpp
	struct
	{
		int             index = 0;
		VkTimeDomainEXT timeDomainEXT{};
	} selected_time_domain{};
```

## Marking timestamps

To further elaborate the usage of calibrated timestamps, ```timestamps_begin()``` and ```timestamps_end()```
are introduced. They mark the beginning and the end of a targeted code block, as their names indicate. Associated data
is stored in the structure ```DeltaTimestamp```, defined as follows:

```cpp
struct DeltaTimestamp
{
    uint64_t    begin = 0;
    uint64_t    end   = 0;
    uint64_t    delta = 0;
    std::string tag   = "Untagged";

    void get_delta()
    {
        this->delta = this->end - this->begin;
    }
};
```

```timestamps_begin()``` marks the first timestamp at the beginning of a code block, where:

```cpp
void CalibratedTimestamps::timestamps_begin(const std::string &input_tag)
{
	// Initialize, then update time domains and timestamps
	is_timestamp_updated = false;
	timestamps.clear();
	max_deviations.clear();

	// Now to get timestamps
	get_timestamps();

	if (is_timestamp_updated)
	{
		// Create a local delta_timestamp to push back to the vector delta_timestamps
		DeltaTimestamp delta_timestamp{};

		// Naming the tic-toc tags and begin timestamp for this particular mark
		if (!input_tag.empty())
		{
			delta_timestamp.tag = input_tag;
		}
		delta_timestamp.begin = timestamps[selected_time_domain.index];

		// Push back this partially filled element to the vector, which will be filled in its corresponding toc
		delta_timestamps.push_back(delta_timestamp);
	}
}
```

and ```timestamps_end()``` marks the second timestamp by the end of the same code block, where:

```cpp
void CalibratedTimestamps::timestamps_end(const std::string &input_tag)
{
	if (delta_timestamps.empty() || (input_tag != delta_timestamps.back().tag))
	{
		LOGI("Timestamps begin-to-end Fatal Error: Timestamps end is not tagged the same as its begin!\n")
		return;        // exits the function here, further calculation is meaningless
	}

	// Initialize, then update time domains and timestamps
	is_timestamp_updated = false;
	timestamps.clear();
	max_deviations.clear();

	// Now to get timestamps
	get_timestamps();

	if (is_timestamp_updated)
	{
		// Add this data to the last term of delta_timestamps vector
		delta_timestamps.back().end = timestamps[selected_time_domain.index];
		delta_timestamps.back().get_delta();
	}
}
```

## UI overlay

![Sample](./images/calibrated_timestamps_ui_overlay.png)

From the UI overlay, the object type can be selected from 1) a sphere, 2) a teapot, and 3) a torus knot. Skybox and the
bloom effect can be switched on and off. In addition, the selected time domain will be displayed, as well as the
profiling information for the ```command buffers``` building process. ```timestampes_begin()```
and ```timestamps_end()``` are used to profile the ```build_command_buffers()```, where:

```cpp
void CalibratedTimestamps::build_command_buffers()
{
    // Reset the delta timestamps vector
    delta_timestamps.clear();

	timestamps_begin("Build Command Buffers");
    
    {/* ... build command buffers ... */}
    
    timestamps_end("Build Command Buffers");
}
```

and if ```delta_timestamps``` is not empty, the following code will be executed:

```cpp
if (!delta_timestamps.empty())
{
    drawer.text("Time Domain Selected:\n %s", time_domain_to_string(selected_time_domain.timeDomainEXT).c_str());

    for (const auto &delta_timestamp : delta_timestamps)
    {
        drawer.text("%s:\n %.1f Microseconds", delta_timestamp.tag.c_str(), static_cast<float>(delta_timestamp.delta) * timestamp_period * 0.001f);
    }
}
```
