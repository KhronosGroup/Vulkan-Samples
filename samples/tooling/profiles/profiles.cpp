/* Copyright (c) 2022-2024, Sascha Willems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Using Vulkan profiles from the LunarG SDK for device and instance setup
 */

#include "profiles.h"

#include "common/error.h"
#include "common/vk_common.h"
#include "core/command_pool.h"
#include "core/queue.h"
#include "fence_pool.h"

// The Vulkan Profiles library is part of the SDK and has been copied to the sample's folder for convenience
#include "vulkan_profiles.hpp"

// This sample uses the VP_LUNARG_desktop_portability_2021 profile that defines feature sets for common desktop platforms with drivers supporting Vulkan 1.1 on Windows and Linux, and the VP_LUNARG_desktop_portability_2021_subset profile on portability platforms like macOS
#if (defined(VKB_ENABLE_PORTABILITY) && defined(VP_LUNARG_desktop_portability_2021_subset))
#	define PROFILE_NAME VP_LUNARG_DESKTOP_PORTABILITY_2021_SUBSET_NAME
#	define PROFILE_SPEC_VERSION VP_LUNARG_DESKTOP_PORTABILITY_2021_SUBSET_SPEC_VERSION
#else
#	define PROFILE_NAME VP_LUNARG_DESKTOP_PORTABILITY_2021_NAME
#	define PROFILE_SPEC_VERSION VP_LUNARG_DESKTOP_PORTABILITY_2021_SPEC_VERSION
#endif

Profiles::Profiles()
{
	title = "Vulkan Profiles";
}

Profiles::~Profiles()
{
	if (has_device())
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class

		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);

		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), base_descriptor_set_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), sampler_descriptor_set_layout, nullptr);
	}
}

// This sample overrides the device creation part of the framework
// Instead of manually setting up all extensions, features, etc. we use the Vulkan Profiles library to simplify device setup
std::unique_ptr<vkb::Device> Profiles::create_device(vkb::PhysicalDevice &gpu)
{
	// Simplified queue setup (only graphics)
	uint32_t                selected_queue_family   = 0;
	const auto             &queue_family_properties = gpu.get_queue_family_properties();
	const float             default_queue_priority{0.0f};
	VkDeviceQueueCreateInfo queue_create_info{};
	queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueCount       = 1;
	queue_create_info.pQueuePriorities = &default_queue_priority;
	for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++)
	{
		if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queue_create_info.queueFamilyIndex = i;
			selected_queue_family              = i;
			break;
		}
	}

	VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	create_info.pNext                = gpu.get_extension_feature_chain();
	create_info.pQueueCreateInfos    = &queue_create_info;
	create_info.queueCreateInfoCount = 1;

	const VpProfileProperties profile_properties = {PROFILE_NAME, PROFILE_SPEC_VERSION};

	// Check if the profile is supported at device level
	VkBool32 profile_supported;
	vpGetPhysicalDeviceProfileSupport(get_instance().get_handle(), gpu.get_handle(), &profile_properties, &profile_supported);
	if (!profile_supported)
	{
		throw std::runtime_error{"The selected profile is not supported (error at creating the device)!"};
	}

	// Create the device using the profile tool library
	VpDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.pCreateInfo = &create_info;
	deviceCreateInfo.pProfile    = &profile_properties;
	deviceCreateInfo.flags       = VP_DEVICE_CREATE_MERGE_EXTENSIONS_BIT;
	VkDevice vulkan_device;
	VkResult result = vpCreateDevice(gpu.get_handle(), &deviceCreateInfo, nullptr, &vulkan_device);

	if (result != VK_SUCCESS)
	{
		throw vkb::VulkanException{result, "Could not create device with the selected profile. The device may not support all features required by this profile!"};
	}

	// Post device setup required for the framework
	auto device = std::make_unique<vkb::Device>(gpu, vulkan_device, get_surface());
	device->add_queue(0, queue_create_info.queueFamilyIndex, queue_family_properties[selected_queue_family], true);
	device->prepare_memory_allocator();
	device->create_internal_command_pool();
	device->create_internal_fence_pool();

	return device;
}

// This sample overrides the instance creation part of the framework
// Instead of manually setting up all properties we use the Vulkan Profiles library to simplify instance setup
std::unique_ptr<vkb::Instance> Profiles::create_instance(bool headless)
{
	// Initialize Volk Vulkan Loader
	VkResult result = volkInitialize();
	if (result)
	{
		throw vkb::VulkanException(result, "Failed to initialize volk.");
	}

	const VpProfileProperties profile_properties = {PROFILE_NAME, PROFILE_SPEC_VERSION};

	// Check if the profile is supported at instance level
	VkBool32 profile_supported;
	vpGetInstanceProfileSupport(nullptr, &profile_properties, &profile_supported);
	if (!profile_supported)
	{
		throw std::runtime_error{"The selected profile is not supported (error at creating the instance)!"};
	}

	// Even when using profiles we still need to provide the platform specific get_surface() extension
	std::vector<const char *> enabled_extensions;
	enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

	for (const char *extension_name : window->get_required_surface_extensions())
	{
		enabled_extensions.push_back(extension_name);
	}

	VkInstanceCreateInfo create_info{};

#if (defined(VKB_ENABLE_PORTABILITY))
	uint32_t instance_extension_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));
	std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

	// If VK_KHR_portability_enumeration is available at runtime, enable the extension and flag for instance creation
	if (std::any_of(available_instance_extensions.begin(),
	                available_instance_extensions.end(),
	                [](VkExtensionProperties const &extension) { return strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0; }))
	{
		enabled_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}

#	if defined(PLATFORM__MACOS) && TARGET_OS_OSX
	// On macOS use layer setting to configure MoltenVK for using Metal argument buffers (needed for descriptor indexing/scaling)
	VkLayerSettingEXT            layerSetting{};
	const int32_t                useMetalArgumentBuffers = 1;
	VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo{};

	if (std::any_of(available_instance_extensions.begin(),
	                available_instance_extensions.end(),
	                [](VkExtensionProperties const &extension) { return strcmp(extension.extensionName, VK_EXT_LAYER_SETTINGS_EXTENSION_NAME) == 0; }))
	{
		enabled_extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);

		layerSetting.pLayerName   = "MoltenVK";
		layerSetting.pSettingName = "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS";
		layerSetting.type         = VK_LAYER_SETTING_TYPE_INT32_EXT;
		layerSetting.valueCount   = 1;
		layerSetting.pValues      = &useMetalArgumentBuffers;

		layerSettingsCreateInfo.sType        = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
		layerSettingsCreateInfo.settingCount = 1;
		layerSettingsCreateInfo.pSettings    = &layerSetting;

		create_info.pNext = &layerSettingsCreateInfo;
	}
	else
	{
		// If layer settings is not available at runtime, set macOS environment variable for support of older Vulkan SDKs
		// Will not work in batch mode, but is the best we can do short of using the deprecated MoltenVK private config API
		setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "1", 1);
	}
#	endif
#endif

	create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.ppEnabledExtensionNames = enabled_extensions.data();
	create_info.enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size());
	// Note: We don't explicitly set an application info here so the one from the profile is used
	// This also defines the api version to be used

	// Create the instance using the profile tool library
	// We set VP_INSTANCE_CREATE_MERGE_EXTENSIONS_BIT so the extensions defined in the profile will be merged with the extensions we specified manually
	VpInstanceCreateInfo instance_create_info{};
	instance_create_info.pProfile    = &profile_properties;
	instance_create_info.pCreateInfo = &create_info;
	instance_create_info.flags       = VP_INSTANCE_CREATE_MERGE_EXTENSIONS_BIT;
	VkInstance vulkan_instance;

	result = vpCreateInstance(&instance_create_info, nullptr, &vulkan_instance);

	if (result != VK_SUCCESS)
	{
		throw vkb::VulkanException{result, "Could not create instance with the selected profile. The instance may not support all features required by this profile!"};
	}

	volkLoadInstance(vulkan_instance);

	return std::make_unique<vkb::Instance>(vulkan_instance);
}

void Profiles::generate_textures()
{
	// Generate random textures to be sourced from a single descriptor

	// Image info is same for all textures
	const int32_t dim = 2;

	VkImageCreateInfo image_info = vkb::initializers::image_create_info();
	image_info.format            = VK_FORMAT_R8G8B8A8_UNORM;
	image_info.extent            = {dim, dim, 1};
	image_info.mipLevels         = 1;
	image_info.arrayLayers       = 1;
	image_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage             = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	image_info.imageType         = VK_IMAGE_TYPE_2D;
	image_info.samples           = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
	image_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

	VkImageViewCreateInfo image_view           = vkb::initializers::image_view_create_info();
	image_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	image_view.format                          = VK_FORMAT_R8G8B8A8_UNORM;
	image_view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view.subresourceRange.baseMipLevel   = 0;
	image_view.subresourceRange.levelCount     = 1;
	image_view.subresourceRange.baseArrayLayer = 0;
	image_view.subresourceRange.layerCount     = 1;

	auto staging_buffer = vkb::core::Buffer::create_staging_buffer(get_device(), image_info.extent.width * image_info.extent.height * sizeof(uint32_t));

	textures.resize(32);
	for (size_t i = 0; i < textures.size(); i++)
	{
		VK_CHECK(vkCreateImage(get_device().get_handle(), &image_info, nullptr, &textures[i].image));
		VkMemoryAllocateInfo memory_allocation_info = vkb::initializers::memory_allocate_info();
		VkMemoryRequirements memory_requirements;
		vkGetImageMemoryRequirements(get_device().get_handle(), textures[i].image, &memory_requirements);
		memory_allocation_info.allocationSize  = memory_requirements.size;
		memory_allocation_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocation_info, nullptr, &textures[i].memory));
		VK_CHECK(vkBindImageMemory(get_device().get_handle(), textures[i].image, textures[i].memory, 0));
		image_view.image = textures[i].image;
		VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view, nullptr, &textures[i].image_view));

		// Generate a random texture
		std::random_device                   rnd_device;
		std::default_random_engine           rnd_engine(rnd_device());
		std::uniform_int_distribution<short> rnd_dist(0, 255);
		const size_t                         buffer_size = dim * dim * 4;
		uint8_t                             *buffer      = staging_buffer.map();
		for (size_t i = 0; i < dim * dim; i++)
		{
			buffer[i * 4]     = static_cast<uint8_t>(rnd_dist(rnd_engine));
			buffer[i * 4 + 1] = static_cast<uint8_t>(rnd_dist(rnd_engine));
			buffer[i * 4 + 2] = static_cast<uint8_t>(rnd_dist(rnd_engine));
			buffer[i * 4 + 3] = 255;
		}

		staging_buffer.unmap();
		staging_buffer.flush();

		auto &cmd = get_device().request_command_buffer();
		cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		vkb::image_layout_transition(cmd.get_handle(), textures[i].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copy_info{};
		copy_info.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy_info.imageExtent      = image_info.extent;
		vkCmdCopyBufferToImage(cmd.get_handle(), staging_buffer.get_handle(), textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);

		vkb::image_layout_transition(cmd.get_handle(), textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VK_CHECK(cmd.end());

		get_device().get_suitable_graphics_queue().submit(cmd, VK_NULL_HANDLE);
		get_device().get_suitable_graphics_queue().wait_idle();
	}

	// Create immutable sampler for the textures
	VkSamplerCreateInfo sampler_info = vkb::initializers::sampler_create_info();
	sampler_info.magFilter           = VK_FILTER_NEAREST;
	sampler_info.minFilter           = VK_FILTER_NEAREST;
	sampler_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.mipLodBias          = 0.0f;
	sampler_info.compareOp           = VK_COMPARE_OP_NEVER;
	sampler_info.minLod              = 0.0f;
	sampler_info.maxLod              = 0.0f;
	sampler_info.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler_info, nullptr, &sampler));
}

void Profiles::generate_cubes()
{
	// Generate cubes with randomized per-face texture indices (as base for descriptor indexing)
	std::vector<VertexStructure> vertices;
	std::vector<uint32_t>        indices;

	// Generate random per-face texture indices
	std::random_device                     rndDevice;
	std::default_random_engine             rndEngine(rndDevice());
	std::uniform_int_distribution<int32_t> rndDist(0, static_cast<uint32_t>(textures.size()) - 1);

	// Generate cubes with random per-face texture indices
	const uint32_t count = 6;
	for (uint32_t i = 0; i < count; i++)
	{
		// Get a random texture index that the shader will sample from via the vertex attribute
		const auto texture_index = [&rndDist, &rndEngine]() {
			return rndDist(rndEngine);
		};

		// Push vertices to buffer
		float pos = 2.5f * i - (count * 2.5f / 2.0f);

		const std::vector<VertexStructure> cube = {
		    {{-1.0f + pos, -1.0f, 1.0f}, {0.0f, 0.0f}, texture_index()},
		    {{1.0f + pos, -1.0f, 1.0f}, {1.0f, 0.0f}, texture_index()},
		    {{1.0f + pos, 1.0f, 1.0f}, {1.0f, 1.0f}, texture_index()},
		    {{-1.0f + pos, 1.0f, 1.0f}, {0.0f, 1.0f}, texture_index()},

		    {{1.0f + pos, 1.0f, 1.0f}, {0.0f, 0.0f}, texture_index()},
		    {{1.0f + pos, 1.0f, -1.0f}, {1.0f, 0.0f}, texture_index()},
		    {{1.0f + pos, -1.0f, -1.0f}, {1.0f, 1.0f}, texture_index()},
		    {{1.0f + pos, -1.0f, 1.0f}, {0.0f, 1.0f}, texture_index()},

		    {{-1.0f + pos, -1.0f, -1.0f}, {0.0f, 0.0f}, texture_index()},
		    {{1.0f + pos, -1.0f, -1.0f}, {1.0f, 0.0f}, texture_index()},
		    {{1.0f + pos, 1.0f, -1.0f}, {1.0f, 1.0f}, texture_index()},
		    {{-1.0f + pos, 1.0f, -1.0f}, {0.0f, 1.0f}, texture_index()},

		    {{-1.0f + pos, -1.0f, -1.0f}, {0.0f, 0.0f}, texture_index()},
		    {{-1.0f + pos, -1.0f, 1.0f}, {1.0f, 0.0f}, texture_index()},
		    {{-1.0f + pos, 1.0f, 1.0f}, {1.0f, 1.0f}, texture_index()},
		    {{-1.0f + pos, 1.0f, -1.0f}, {0.0f, 1.0f}, texture_index()},

		    {{1.0f + pos, 1.0f, 1.0f}, {0.0f, 0.0f}, texture_index()},
		    {{-1.0f + pos, 1.0f, 1.0f}, {1.0f, 0.0f}, texture_index()},
		    {{-1.0f + pos, 1.0f, -1.0f}, {1.0f, 1.0f}, texture_index()},
		    {{1.0f + pos, 1.0f, -1.0f}, {0.0f, 1.0f}, texture_index()},

		    {{-1.0f + pos, -1.0f, -1.0f}, {0.0f, 0.0f}, texture_index()},
		    {{1.0f + pos, -1.0f, -1.0f}, {1.0f, 0.0f}, texture_index()},
		    {{1.0f + pos, -1.0f, 1.0f}, {1.0f, 1.0f}, texture_index()},
		    {{-1.0f + pos, -1.0f, 1.0f}, {0.0f, 1.0f}, texture_index()},
		};
		for (auto &vertex : cube)
		{
			vertices.push_back(vertex);
		}
		// Push indices to buffer
		const std::vector<uint32_t> cubeIndices = {
		    0, 1, 2, 0, 2, 3,
		    4, 5, 6, 4, 6, 7,
		    8, 9, 10, 8, 10, 11,
		    12, 13, 14, 12, 14, 15,
		    16, 17, 18, 16, 18, 19,
		    20, 21, 22, 20, 22, 23};
		for (auto &index : cubeIndices)
		{
			indices.push_back(index + static_cast<uint32_t>(vertices.size()));
		}
	}

	index_count = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vkb::to_u32(vertices.size() * sizeof(VertexStructure));
	auto index_buffer_size  = vkb::to_u32(indices.size() * sizeof(uint32_t));

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                    vertex_buffer_size,
	                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                   index_buffer_size,
	                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                                   VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer->update(indices.data(), index_buffer_size);
}

void Profiles::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	VkClearValue clear_values[2]{};
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = 2;
	render_pass_begin_info.pClearValues             = clear_values;

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];
		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));
		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &base_descriptor_set, 0, nullptr);
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
		vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);
		draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);
		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void Profiles::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void Profiles::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	    vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(textures.size()))};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(
	        static_cast<uint32_t>(pool_sizes.size()),
	        pool_sizes.data(),
	        3);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void Profiles::setup_descriptor_set_layout()
{
	// We separate the descriptor sets for the uniform buffer + image and samplers, so we don't need to duplicate the descriptors for the former
	VkDescriptorSetLayoutCreateInfo           descriptor_layout_create_info{};
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings{};

	// Mark second slot as variable for descriptor indexing
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT descriptor_set_layout_binding_flags{};
	descriptor_set_layout_binding_flags.sType                         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	descriptor_set_layout_binding_flags.bindingCount                  = 2;
	std::vector<VkDescriptorBindingFlagsEXT> descriptor_binding_flags = {
	    0,
	    VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT};
	descriptor_set_layout_binding_flags.pBindingFlags = descriptor_binding_flags.data();

	descriptor_layout_create_info.pNext = &descriptor_set_layout_binding_flags;

	// Set layout for the uniform buffer and the image
	set_layout_bindings = {
	    // Binding 0 : Vertex shader uniform buffer
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        VK_SHADER_STAGE_VERTEX_BIT,
	        0),
	    // Binding 1 : Fragment shader combined image and sampler
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        VK_SHADER_STAGE_FRAGMENT_BIT,
	        1,
	        static_cast<uint32_t>(textures.size()))};
	descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &base_descriptor_set_layout));

	// Set layout for the samplers
	set_layout_bindings = {
	    // Binding 0: Fragment shader sampler
	    vkb::initializers::descriptor_set_layout_binding(
	        VK_DESCRIPTOR_TYPE_SAMPLER,
	        VK_SHADER_STAGE_FRAGMENT_BIT,
	        0,
	        static_cast<uint32_t>(textures.size()))};
	descriptor_layout_create_info =
	    vkb::initializers::descriptor_set_layout_create_info(
	        set_layout_bindings.data(),
	        static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &sampler_descriptor_set_layout));

	// Pipeline layout
	// Set layout for the base descriptors in set 0 and set layout for the sampler descriptors in set 1
	std::vector<VkDescriptorSetLayout> set_layouts = {base_descriptor_set_layout, sampler_descriptor_set_layout};
	VkPipelineLayoutCreateInfo         pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        set_layouts.data(),
	        static_cast<uint32_t>(set_layouts.size()));
	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void Profiles::setup_descriptor_set()
{
	// We separate the descriptor sets for the uniform buffer + image and samplers, so we don't need to duplicate the descriptors for the former
	VkDescriptorSetAllocateInfo descriptor_set_alloc_info{};

	// Descriptors set for the uniform buffer and the image
	descriptor_set_alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(
	        descriptor_pool,
	        &base_descriptor_set_layout,
	        1);

	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocInfo = {};
	uint32_t                                              variableDescCounts[]             = {static_cast<uint32_t>(textures.size())};
	variableDescriptorCountAllocInfo.sType                                                 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	variableDescriptorCountAllocInfo.descriptorSetCount                                    = 1;
	variableDescriptorCountAllocInfo.pDescriptorCounts                                     = variableDescCounts;

	descriptor_set_alloc_info.pNext = &variableDescriptorCountAllocInfo;
	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_alloc_info, &base_descriptor_set));

	VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer_vs);

	// Combined image descriptor for the texture
	VkDescriptorImageInfo image_descriptor{};
	image_descriptor.imageView   = textures[0].image_view;
	image_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_descriptor.sampler     = sampler;

	std::vector<VkWriteDescriptorSet> write_descriptor_sets(2);
	// Binding 0 : Vertex shader uniform buffer
	write_descriptor_sets[0] = vkb::initializers::write_descriptor_set(
	    base_descriptor_set,
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	    0,
	    &buffer_descriptor);

	// Binding 1 : Fragment shader sampled image
	// Put all images into a single array
	std::vector<VkDescriptorImageInfo> texture_descriptors(textures.size());
	for (size_t i = 0; i < textures.size(); i++)
	{
		texture_descriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texture_descriptors[i].sampler     = sampler;
		texture_descriptors[i].imageView   = textures[i].image_view;
	}

	// Unlike an array texture, these are addressed like typical arrays
	write_descriptor_sets[1]                 = {};
	write_descriptor_sets[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[1].dstBinding      = 1;
	write_descriptor_sets[1].dstArrayElement = 0;
	write_descriptor_sets[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_sets[1].descriptorCount = static_cast<uint32_t>(textures.size());
	write_descriptor_sets[1].pBufferInfo     = 0;
	write_descriptor_sets[1].dstSet          = base_descriptor_set;
	write_descriptor_sets[1].pImageInfo      = texture_descriptors.data();

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void Profiles::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(
	        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	        0,
	        VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(
	        VK_POLYGON_MODE_FILL,
	        VK_CULL_MODE_NONE,
	        VK_FRONT_FACE_CLOCKWISE,
	        0);

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(
	        0xf,
	        VK_FALSE);

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(
	        1,
	        &blend_attachment_state);

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(
	        VK_TRUE,
	        VK_TRUE,
	        VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(
	        VK_SAMPLE_COUNT_1_BIT,
	        0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	// Load shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
	shader_stages[0] = load_shader("profiles", "profiles.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader("profiles", "profiles.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(VertexStructure), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexStructure, pos)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexStructure, uv)),
	    vkb::initializers::vertex_input_attribute_description(0, 2, VK_FORMAT_R32_SINT, offsetof(VertexStructure, texture_index)),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);

	pipeline_create_info.pVertexInputState   = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.stageCount          = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages             = shader_stages.data();

	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms
void Profiles::prepare_uniform_buffers()
{
	// Vertex shader uniform buffer block
	uniform_buffer_vs = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                        sizeof(ubo_vs),
	                                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_uniform_buffers();
}

void Profiles::update_uniform_buffers()
{
	// Fixed ubo with projection and view matrices
	ubo_vs.projection = camera.matrices.perspective;
	ubo_vs.view       = camera.matrices.view;

	uniform_buffer_vs->convert_and_update(ubo_vs);
}

bool Profiles::prepare(const vkb::ApplicationOptions &options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -10.0f));
	camera.set_rotation(glm::vec3(0.0f));

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	generate_textures();
	generate_cubes();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

void Profiles::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
}

void Profiles::view_changed()
{
	update_uniform_buffers();
}

void Profiles::on_update_ui_overlay(vkb::Drawer &drawer)
{
	drawer.text("Enabled profile: %s", PROFILE_NAME);
}

std::unique_ptr<vkb::Application> create_profiles()
{
	return std::make_unique<Profiles>();
}
