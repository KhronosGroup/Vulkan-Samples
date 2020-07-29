/* Copyright (c) 2018-2020, Arm Limited and Contributors
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

#include "utils.h"

#include <queue>
#include <stdexcept>

#include "core/pipeline_layout.h"
#include "core/shader_module.h"
#include "graphing/framework_graph.h"
#include "graphing/scene_graph.h"
#include "scene_graph/components/image.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/pbr_material.h"
#include "scene_graph/components/perspective_camera.h"
#include "scene_graph/components/sampler.h"
#include "scene_graph/components/sub_mesh.h"
#include "scene_graph/components/texture.h"
#include "scene_graph/components/transform.h"
#include "scene_graph/node.h"
#include "scene_graph/script.h"
#include "scene_graph/scripts/free_camera.h"

namespace vkb
{
std::string get_extension(const std::string &uri)
{
	auto dot_pos = uri.find_last_of('.');
	if (dot_pos == std::string::npos)
	{
		throw std::runtime_error{"Uri has no extension"};
	}

	return uri.substr(dot_pos + 1);
}

void screenshot(RenderContext &render_context, const std::string &filename)
{
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	// We want the last completed frame since we don't want to be reading from an incomplete framebuffer
	auto &frame          = render_context.get_last_rendered_frame();
	auto &src_image_view = frame.get_render_target().get_views().at(0);

	auto width  = render_context.get_surface_extent().width;
	auto height = render_context.get_surface_extent().height;

	core::Image dst_image{render_context.get_device(),
	                      VkExtent3D{width, height, 1},
	                      format,
	                      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	                      VMA_MEMORY_USAGE_GPU_TO_CPU,
	                      VK_SAMPLE_COUNT_1_BIT,
	                      1, 1,
	                      VK_IMAGE_TILING_LINEAR};

	core::ImageView dst_image_view{dst_image, VK_IMAGE_VIEW_TYPE_2D};

	const auto &queue = render_context.get_device().get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

	auto &cmd_buf = render_context.get_device().request_command_buffer();

	cmd_buf.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	// Enable destination image to be written to
	{
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.src_access_mask = 0;
		memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memory_barrier.old_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.new_layout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memory_barrier.src_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.dst_stage_mask  = VK_PIPELINE_STAGE_TRANSFER_BIT;

		cmd_buf.image_memory_barrier(dst_image_view, memory_barrier);
	}

	// Enable framebuffer image view to be read from
	{
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		memory_barrier.new_layout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

		cmd_buf.image_memory_barrier(src_image_view, memory_barrier);
	}

	bool swizzle = false;

	// Check if framebuffer images are in a BGR format
	auto bgr_formats = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM};
	swizzle          = std::find(bgr_formats.begin(), bgr_formats.end(), src_image_view.get_format()) != bgr_formats.end();

	// Copy framebuffer image memory
	VkImageCopy image_copy_region{};
	image_copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_copy_region.srcSubresource.layerCount = 1;
	image_copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_copy_region.dstSubresource.layerCount = 1;
	image_copy_region.extent.width              = width;
	image_copy_region.extent.height             = height;
	image_copy_region.extent.depth              = 1;

	cmd_buf.copy_image(src_image_view.get_image(), dst_image, {image_copy_region});

	// Enable destination image to map image memory
	{
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memory_barrier.new_layout     = VK_IMAGE_LAYOUT_GENERAL;
		memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

		cmd_buf.image_memory_barrier(dst_image_view, memory_barrier);
	}

	// Revert back the framebuffer image view from transfer to present
	{
		ImageMemoryBarrier memory_barrier{};
		memory_barrier.old_layout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		memory_barrier.new_layout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

		cmd_buf.image_memory_barrier(src_image_view, memory_barrier);
	}

	cmd_buf.end();

	queue.submit(cmd_buf, frame.request_fence());

	queue.wait_idle();

	auto raw_data = dst_image.map();

	// Android requires the sub resource to be queried while the memory is mapped
	VkImageSubresource  subresource{VK_IMAGE_ASPECT_COLOR_BIT};
	VkSubresourceLayout subresource_layout;
	vkGetImageSubresourceLayout(render_context.get_device().get_handle(), dst_image.get_handle(), &subresource, &subresource_layout);

	raw_data += subresource_layout.offset;

	// Creates a pointer to the address of the first byte past the offset of the image data
	// Replace the A component with 255 (remove transparency)
	// If swapchain format is BGR, swapping the R and B components
	uint8_t *data = raw_data;
	if (swizzle)
	{
		for (size_t i = 0; i < height; ++i)
		{
			// Gets the first pixel of the first row of the image (a pixel is 4 bytes)
			unsigned int *pixel = (unsigned int *) data;

			// Iterate over each pixel, swapping R and B components and writing the max value for alpha
			for (size_t j = 0; j < width; ++j)
			{
				auto temp                = *((uint8_t *) pixel + 2);
				*((uint8_t *) pixel + 2) = *((uint8_t *) pixel);
				*((uint8_t *) pixel)     = temp;
				*((uint8_t *) pixel + 3) = 255;

				// Get next pixel
				pixel++;
			}
			// Pointer arithmetic to get the first byte in the next row of pixels
			data += subresource_layout.rowPitch;
		}
	}
	else
	{
		for (size_t i = 0; i < height; ++i)
		{
			// Gets the first pixel of the first row of the image (a pixel is 4 bytes)
			unsigned int *pixel = (unsigned int *) data;

			// Iterate over each pixel, writing the max value for alpha
			for (size_t j = 0; j < width; ++j)
			{
				*((uint8_t *) pixel + 3) = 255;

				// Get next pixel
				pixel++;
			}
			// Pointer arithmetic to get the first byte in the next row of pixels
			data += subresource_layout.rowPitch;
		}
	}

	vkb::fs::write_image(raw_data,
	                     filename,
	                     width,
	                     height,
	                     4,
	                     static_cast<uint32_t>(subresource_layout.rowPitch));

	dst_image.unmap();
}        // namespace vkb

std::string to_snake_case(const std::string &text)
{
	std::stringstream result;

	for (const auto ch : text)
	{
		if (std::isalpha(ch))
		{
			if (std::isspace(ch))
			{
				result << "_";
			}
			else
			{
				if (std::isupper(ch))
				{
					result << "_";
				}

				result << static_cast<char>(std::tolower(ch));
			}
		}
		else
		{
			result << ch;
		}
	}

	return result.str();
}

sg::Light &add_light(sg::Scene &scene, sg::LightType type, const glm::vec3 &position, const glm::quat &rotation, const sg::LightProperties &props, sg::Node *parent_node)
{
	auto light_ptr = std::make_unique<sg::Light>("light");
	auto node      = std::make_unique<sg::Node>(-1, "light node");

	if (parent_node)
	{
		node->set_parent(*parent_node);
	}

	light_ptr->set_node(*node);
	light_ptr->set_light_type(type);
	light_ptr->set_properties(props);

	auto &t = node->get_transform();
	t.set_translation(position);
	t.set_rotation(rotation);

	// Storing the light component because the unique_ptr will be moved to the scene
	auto &light = *light_ptr;

	node->set_component(light);
	scene.add_child(*node);
	scene.add_component(std::move(light_ptr));
	scene.add_node(std::move(node));

	return light;
}

sg::Light &add_point_light(sg::Scene &scene, const glm::vec3 &position, const sg::LightProperties &props, sg::Node *parent_node)
{
	return add_light(scene, sg::LightType::Point, position, {}, props, parent_node);
}

sg::Light &add_directional_light(sg::Scene &scene, const glm::quat &rotation, const sg::LightProperties &props, sg::Node *parent_node)
{
	return add_light(scene, sg::LightType::Directional, {}, rotation, props, parent_node);
}

sg::Light &add_spot_light(sg::Scene &scene, const glm::vec3 &position, const glm::quat &rotation, const sg::LightProperties &props, sg::Node *parent_node)
{
	return add_light(scene, sg::LightType::Spot, position, rotation, props, parent_node);
}

sg::Node &add_free_camera(sg::Scene &scene, const std::string &node_name, VkExtent2D extent)
{
	auto camera_node = scene.find_node(node_name);

	if (!camera_node)
	{
		LOGW("Camera node `{}` not found. Looking for `default_camera` node.", node_name.c_str());

		camera_node = scene.find_node("default_camera");
	}

	if (!camera_node)
	{
		throw std::runtime_error("Camera node with name `" + node_name + "` not found.");
	}

	if (!camera_node->has_component<sg::Camera>())
	{
		throw std::runtime_error("No camera component found for `" + node_name + "` node.");
	}

	auto free_camera_script = std::make_unique<sg::FreeCamera>(*camera_node);

	free_camera_script->resize(extent.width, extent.height);

	scene.add_component(std::move(free_camera_script), *camera_node);

	return *camera_node;
}

namespace graphs
{
bool generate_all(RenderContext &context, sg::Scene &scene)
{
	bool success = true;
	if (!graphing::framework_graph::generate(context))
	{
		LOGE("Failed to save render context graph");
		success = false;
	}
	if (!graphing::scene_graph::generate(scene))
	{
		LOGE("Failed to save scene graph");
		success = false;
	}
	return success;
}
}        // namespace graphs

}        // namespace vkb
