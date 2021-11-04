#include "dynamic_rendering.h"

DynamicRendering::DynamicRendering()
{
	title = "Dynamic Rendering";

	// Dynamic Rendering is a Vulkan 1.2 extension
	set_api_version(VK_API_VERSION_1_2);
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	add_device_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
}

DynamicRendering::~DynamicRendering()
{
}

bool DynamicRendering::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	return true;
}

void DynamicRendering::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	auto &requested_dynamic_rendering            = gpu.request_extension_features<VkPhysicalDeviceDynamicRenderingFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR);
	requested_dynamic_rendering.dynamicRendering = VK_TRUE;

	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = true;
	}
}

void DynamicRendering::render(float delta_time)
{
}

void DynamicRendering::build_command_buffers()
{
}

void DynamicRendering::view_changed()
{
}

void DynamicRendering::on_update_ui_overlay(vkb::Drawer &drawer)
{
}

std::unique_ptr<vkb::VulkanSample> create_dynamic_rendering()
{
	return std::make_unique<DynamicRendering>();
}
