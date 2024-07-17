/* Copyright (c) 2023-2024, Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "mobile_nerf.h"
#include "filesystem/legacy.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "gltf_loader.h"
#include "platform/platform.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/perspective_camera.h"

namespace
{
constexpr uint32_t MIN_THREAD_COUNT = 1;
struct RequestFeature
{
	vkb::PhysicalDevice &gpu;
	explicit RequestFeature(vkb::PhysicalDevice &gpu) :
	    gpu(gpu)
	{}

	template <typename T>
	RequestFeature &request(VkStructureType s_type, VkBool32 T::*member)
	{
		auto &member_feature   = gpu.request_extension_features<T>(s_type);
		member_feature.*member = VK_TRUE;
		return *this;
	}
};

template <typename T>
struct CopyBuffer
{
	std::vector<T> operator()(std::unordered_map<std::string, vkb::core::BufferC> &buffers, const char *buffer_name)
	{
		auto iter = buffers.find(buffer_name);
		if (iter == buffers.cend())
		{
			return {};
		}
		auto &buffer = iter->second;

		std::vector<T> out;

		const size_t sz = buffer.get_size();
		out.resize(sz / sizeof(T));
		const bool already_mapped = buffer.get_data() != nullptr;
		if (!already_mapped)
		{
			buffer.map();
		}
		memcpy(&out[0], buffer.get_data(), sz);
		if (!already_mapped)
		{
			buffer.unmap();
		}
		return out;
	}
};

void camera_set_look_at(vkb::Camera &camera, const glm::vec3 look, const glm::vec3 up)
{
	auto view_matrix = glm::lookAt(camera.position, look, up);

	glm::vec3 scale;
	glm::quat orientation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(view_matrix, scale, orientation, translation, skew, perspective);

	camera.set_rotation(glm::eulerAngles(orientation) * glm::pi<float>() / 180.f);
	camera.set_position(translation);
}

}        // namespace

MobileNerf::MobileNerf()
{
	title = "Mobile NeRF";
	// SPIRV 1.4 requires Vulkan 1.1
	set_api_version(VK_API_VERSION_1_1);
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	// Required by VK_KHR_spirv_1_4
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	// For choosing different sets of weights
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
}

MobileNerf::~MobileNerf()
{
	if (has_device())
	{
		if (render_pass_nerf)
		{
			vkDestroyRenderPass(get_device().get_handle(), render_pass_nerf, nullptr);
		}

		for (uint32_t i = 0; i < nerf_framebuffers.size(); i++)
		{
			if (nerf_framebuffers[i])
			{
				vkDestroyFramebuffer(get_device().get_handle(), nerf_framebuffers[i], nullptr);
			}
		}

		auto device_ptr = get_device().get_handle();

		for (auto &model : models)
		{
			model.vertex_buffer.reset();
			model.index_buffer.reset();

			vkDestroySampler(get_device().get_handle(), model.texture_input_0.sampler, nullptr);
			vkDestroySampler(get_device().get_handle(), model.texture_input_1.sampler, nullptr);

			vkDestroyPipeline(device_ptr, model.pipeline_first_pass, nullptr);
		}

		for (auto &weights_buffer : weights_buffers)
		{
			weights_buffer.reset();
		}

		for (auto &uniform_buffer : uniform_buffers)
		{
			uniform_buffer.reset();
		}

		vkDestroyPipelineLayout(device_ptr, pipeline_first_pass_layout, nullptr);
		vkDestroyDescriptorSetLayout(device_ptr, descriptor_set_first_pass_layout, nullptr);

		if (pipeline_baseline)
		{
			vkDestroyPipeline(get_device().get_handle(), pipeline_baseline, nullptr);
			vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout_baseline, nullptr);
			vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout_baseline, nullptr);
		}

		for (auto &attachment : frameAttachments)
		{
			attachment.feature_0.destroy();
			attachment.feature_1.destroy();
			attachment.feature_2.destroy();
			attachment.weights_idx.destroy();
		}
	}
}

void MobileNerf::read_json_map()
{
	std::string assetBase = vkb::fs::path::get(vkb::fs::path::Type::Assets);
	LOGI("Base assets path: {}", assetBase);

#if defined(NERF_JSON_FILE)
	const std::string nerf_obj_map = assetBase + "scenes/mobile_nerf_models.json";

	std::ifstream f(nerf_obj_map);

	if (!f)
	{
		LOGE("Failed to open nerf obj map data");
		assert(0);
	}

	LOGI("Parsing nerf obj map data {}", nerf_obj_map);

	json raw_asset_map = json::parse(f);
#else

	const std::string nerf_obj_json =
	    R"V0G0N(
        {
            "width": 0,

            "height": 0,

            "texture_type": "8bit",

            "target_model": "lego_combo",

            "deferred": false,

            "rotation": true,

            "lego_ball":{
                "path": "scenes/morpheus_team/lego_ball_phone/",
                "num_sub_model": 1,
                "original": false,
                "camera": [-1, 1, 1],
                "instancing":{
                    "dim": [1, 1, 1],
                    "interval": [2.0, 2.0, 2.0]
                }
            },

            "lego_boba_fett":{
                "path": "scenes/morpheus_team/lego_boba_fett_phone/",
                "num_sub_model": 1,
                "original": false,
                "camera": [-1, 1, 1],
                "instancing":{
                    "dim": [1, 1, 1],
                    "interval": [2.0, 2.0, 2.0]
                }
            },

            "lego_monster_truck":{
                "path": "scenes/morpheus_team/lego_monster_truck_phone/",
                "num_sub_model": 1,
                "original": false,
                "camera": [-1, 1, 1],
                "instancing":{
                    "dim": [1, 1, 1],
                    "interval": [2.0, 2.0, 2.0]
                }
            },

            "lego_tractor":{
                "path": "scenes/morpheus_team/lego_tractor_phone/",
                "num_sub_model": 1,
                "original": false,
                "camera": [-1, 1, 1],
                "instancing":{
                    "dim": [1, 1, 1],
                    "interval": [2.0, 2.0, 2.0]
                }
            },

            "lego_combo":{
                "combo": true,
                "models": ["scenes/morpheus_team/lego_ball_phone/", "scenes/morpheus_team/lego_boba_fett_phone/",
                            "scenes/morpheus_team/lego_monster_truck_phone/", "scenes/morpheus_team/lego_tractor_phone/"],
                "original": [false, false, false, false],
                "camera": [-0.0381453, 1.84186, -1.51744],
                "instancing":{
                    "dim": [2, 2, 2],
                    "interval": [1.5, 1.5, 1.5]
                }
            }
        }
        )V0G0N";

	json raw_asset_map = json::parse(nerf_obj_json);

#endif

	std::string target_model = raw_asset_map["target_model"].get<std::string>();
	asset_map                = raw_asset_map[target_model];

	// Load combo models or a single model
	if (!asset_map["combo"].is_null())
	{
		combo_mode = asset_map["combo"].get<bool>();
	}
	else
	{
		combo_mode = false;
	}

	if (combo_mode)
	{
		model_path.resize(asset_map["models"].size());
		using_original_nerf_models.resize(asset_map["models"].size());

		for (int i = 0; i < model_path.size(); i++)
		{
			model_path[i]                 = asset_map["models"][i].get<std::string>();
			using_original_nerf_models[i] = asset_map["original"][i].get<bool>();
			LOGI("Target model: {}, asset path: {}", target_model, model_path[i]);
		}
	}
	else
	{
		model_path.resize(1);
		model_path[0] = asset_map["path"].get<std::string>();
		using_original_nerf_models.resize(1);
		using_original_nerf_models[0] = asset_map["original"].get<bool>();
		LOGI("Target model: {}, asset path: {}", target_model, model_path[0]);
	}

	std::string textureType = raw_asset_map["texture_type"].get<std::string>();

	if (textureType == "8bit")
	{
		LOGI("Using VK_FORMAT_R8G8B8A8_UNORM for feature texture");
		feature_map_format = VK_FORMAT_R8G8B8A8_UNORM;
	}
	else if (textureType == "16bit")
	{
		LOGI("Using VK_FORMAT_R16G16B16A16_SFLOAT for feature texture");
		feature_map_format = VK_FORMAT_R16G16B16A16_SFLOAT;
	}
	else if (textureType == "32bit")
	{
		LOGI("Using VK_FORMAT_R32G32B32A32_SFLOAT for feature texture");
		feature_map_format = VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	else if (textureType == "8bit")
	{
		LOGI("Using VK_FORMAT_R8G8B8A8_UNORM for feature texture");
		feature_map_format = VK_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		LOGW("Unrecognized feature texture type, using VK_FORMAT_R32G32B32A32_SFLOAT");
		feature_map_format = VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	use_deferred = raw_asset_map["deferred"].get<bool>();
	do_rotation  = raw_asset_map["rotation"].get<bool>();

	view_port_width  = raw_asset_map["width"].get<int>();
	view_port_height = raw_asset_map["height"].get<int>();

	if (asset_map["camera"].is_array() && asset_map["camera"].size() == 3)
	{
		camera_pos = glm::vec3(asset_map["camera"][0].get<float>(), asset_map["camera"][1].get<float>(), asset_map["camera"][2].get<float>());
	}
	else
	{
		LOGW("Fail to read camera position. Use defualt value.");
	}

	json instacing_map = asset_map["instancing"];
	if (instacing_map["dim"].is_array() && instacing_map["dim"].size() == 3)
	{
		instancing_info.dim = glm::vec3(instacing_map["dim"][0].get<int>(), instacing_map["dim"][1].get<int>(), instacing_map["dim"][2].get<int>());
	}
	else
	{
		LOGE("Wrong instancing dimension. Terminating...");
		exit(1);
	}

	if (instacing_map["interval"].is_array() && instacing_map["interval"].size() == 3)
	{
		instancing_info.interval = glm::vec3(instacing_map["interval"][0].get<float>(), instacing_map["interval"][1].get<float>(), instacing_map["interval"][2].get<float>());
	}
	else
	{
		LOGE("Wrong instancing interval. Terminating...");
		exit(1);
	}

	if (instancing_info.dim.x <= 0 || instancing_info.dim.y <= 0 || instancing_info.dim.z <= 0 || instancing_info.interval.x <= 0.f || instancing_info.interval.y <= 0.f || instancing_info.interval.z <= 0.f)
	{
		LOGE("Instancing settings must be positive. Terminating...");
		exit(1);
	}
}

void MobileNerf::load_shaders()
{
	// Loading first pass shaders
	if (use_deferred)
	{
		// Loading first pass shaders
		shader_stages_first_pass[0] = load_shader("mobile_nerf/raster.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages_first_pass[1] = load_shader(
		    combo_mode ?
		        (using_original_nerf_models[0] ? "mobile_nerf/raster_combo.frag" : "mobile_nerf/raster_morpheus_combo.frag") :
		        (using_original_nerf_models[0] ? "mobile_nerf/raster.frag" : "mobile_nerf/raster_morpheus.frag"),
		    VK_SHADER_STAGE_FRAGMENT_BIT);

		// Loading second pass shaders
		shader_stages_second_pass[0] = load_shader("mobile_nerf/quad.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages_second_pass[1] = load_shader(
		    combo_mode ?
		        (using_original_nerf_models[0] ? "mobile_nerf/mlp_combo.frag" : "mobile_nerf/mlp_morpheus_combo.frag") :
		        (using_original_nerf_models[0] ? "mobile_nerf/mlp.frag" : "mobile_nerf/mlp_morpheus.frag"),
		    VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	else
	{
		// Loading one pass shaders
		shader_stages_first_pass[0] = load_shader("mobile_nerf/raster.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shader_stages_first_pass[1] = load_shader(
		    using_original_nerf_models[0] ? "mobile_nerf/merged.frag" : "mobile_nerf/merged_morpheus.frag",
		    VK_SHADER_STAGE_FRAGMENT_BIT);
	}
}

bool MobileNerf::prepare(const vkb::ApplicationOptions &options)
{
	read_json_map();

	// Load the mlp for each model
	mlp_weight_vector.resize(model_path.size());

	for (int i = 0; i < model_path.size(); i++)
	{
		initialize_mlp_uniform_buffers(i);
	}

	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	if (view_port_width == 0 || view_port_height == 0)
	{
		view_port_width        = width;
		view_port_height       = height;
		use_native_screen_size = true;
	}

	load_shaders();

	if (use_deferred)
	{
		update_render_pass_nerf_baseline();
	}
	else
	{
		update_render_pass_nerf_forward();
	}

	setup_nerf_framebuffer_baseline();
	// Because we have our own customized render pass, the UI render pass need to be updated with load on load so it won't
	// clear out the written color attachment
	update_render_pass_flags(RenderPassCreateFlags::ColorAttachmentLoad);

	camera.type  = vkb::CameraType::LookAt;
	camera_pos.y = -camera_pos.y;        // flip y to keep consistency of the init pos between rayquery and rasterization
	camera.set_position(camera_pos);
	camera_set_look_at(camera, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.01f, 256.0f);

	int models_entry = 0;

	for (int model_index = 0; model_index < model_path.size(); model_index++)
	{
		int num_sub_model = models[models_entry].sub_model_num;

		for (int sub_model_index = 0; sub_model_index < num_sub_model; sub_model_index++)
		{
			load_scene(model_index, sub_model_index, models_entry);
			create_texture(model_index, sub_model_index, models_entry);
			create_static_object_buffers(model_index, sub_model_index, models_entry);
			models_entry++;
		}
	}
	create_uniforms();
	prepare_instance_data();
	create_pipeline_layout_fist_pass();

	if (use_deferred)
	{
		create_pipeline_layout_baseline();
	}
	create_descriptor_pool();

	for (auto &model : models)
	{
		create_descriptor_sets_first_pass(model);
	}

	if (use_deferred)
	{
		create_descriptor_sets_baseline();
	}
	prepare_pipelines();
	build_command_buffers();

	prepared = true;
	LOGI("Prepare Done!");
	return true;
}

bool MobileNerf::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	rebuild_command_buffers();
	return true;
}

void MobileNerf::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	RequestFeature(gpu)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, &VkPhysicalDeviceDescriptorIndexingFeaturesEXT::shaderUniformBufferArrayNonUniformIndexing)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, &VkPhysicalDeviceDescriptorIndexingFeaturesEXT::runtimeDescriptorArray)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, &VkPhysicalDeviceDescriptorIndexingFeaturesEXT::descriptorBindingVariableDescriptorCount);
}

void MobileNerf::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	update_uniform_buffers();
}

void MobileNerf::FrameBufferAttachment::destroy()
{
	if (!image)
	{
		return;
	}

	auto &device = image->get_device();
	vkDestroyImageView(device.get_handle(), view, nullptr);
	image.reset();
}

void MobileNerf::setup_attachment(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment &attachment)
{
	if (attachment)
	{
		attachment.destroy();
	}

	VkImageAspectFlags aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageLayout      imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspectMask  = VK_IMAGE_ASPECT_DEPTH_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	auto surfaceExtent = get_render_context().get_surface_extent();
	attachment.image   = std::make_unique<vkb::core::Image>(
        get_device(),
        VkExtent3D{surfaceExtent.width, surfaceExtent.height, 1},
        format,
        usage,
        VMA_MEMORY_USAGE_GPU_ONLY);

	with_command_buffer([&](VkCommandBuffer command_buffer) {
		vkb::image_layout_transition(command_buffer, attachment.image->get_handle(),
		                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		                             {},
		                             {},
		                             VK_IMAGE_LAYOUT_UNDEFINED,
		                             VK_IMAGE_LAYOUT_GENERAL,
		                             {aspectMask, 0, 1, 0, 1});
	});

	VkImageViewCreateInfo color_image_view           = vkb::initializers::image_view_create_info();
	color_image_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	color_image_view.format                          = format;
	color_image_view.subresourceRange.aspectMask     = aspectMask;
	color_image_view.subresourceRange.baseMipLevel   = 0;
	color_image_view.subresourceRange.levelCount     = 1;
	color_image_view.subresourceRange.baseArrayLayer = 0;
	color_image_view.subresourceRange.layerCount     = 1;
	color_image_view.image                           = attachment.image->get_handle();
	VK_CHECK(vkCreateImageView(get_device().get_handle(), &color_image_view, nullptr, &attachment.view));
}

void MobileNerf::setup_nerf_framebuffer_baseline()
{
	if (use_deferred)
	{
		frameAttachments.resize(get_render_context().get_render_frames().size());

		for (auto i = 0; i < frameAttachments.size(); i++)
		{
			setup_attachment(feature_map_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, frameAttachments[i].feature_0);
			setup_attachment(feature_map_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, frameAttachments[i].feature_1);
			setup_attachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, frameAttachments[i].feature_2);
			if (combo_mode)
				setup_attachment(VK_FORMAT_R8_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, frameAttachments[i].weights_idx);
		}
	}

	// Delete existing frame buffers
	if (nerf_framebuffers.size() > 0)
	{
		for (uint32_t i = 0; i < nerf_framebuffers.size(); i++)
		{
			if (nerf_framebuffers[i] != VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(get_device().get_handle(), nerf_framebuffers[i], nullptr);
			}
		}
	}

	std::vector<VkImageView> views;

	if (use_deferred)
	{
		views.resize(combo_mode ? 6 : 5);
		views[depth_attach_idx] = depth_stencil.view;
	}
	else
	{
		views.resize(2);
		views[0] = depth_stencil.view;
	}

	// Depth/Stencil attachment is the same for all frame buffers

	VkFramebufferCreateInfo framebuffer_create_info = {};
	framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.pNext                   = NULL;
	framebuffer_create_info.renderPass              = render_pass_nerf;
	framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(views.size());
	framebuffer_create_info.pAttachments            = views.data();
	framebuffer_create_info.width                   = get_render_context().get_surface_extent().width;
	framebuffer_create_info.height                  = get_render_context().get_surface_extent().height;
	framebuffer_create_info.layers                  = 1;

	nerf_framebuffers.resize(swapchain_buffers.size());

	for (uint32_t i = 0; i < nerf_framebuffers.size(); i++)
	{
		if (use_deferred)
		{
			views[color_attach_0_idx] = frameAttachments[i].feature_0.view;
			views[color_attach_1_idx] = frameAttachments[i].feature_1.view;
			views[color_attach_2_idx] = frameAttachments[i].feature_2.view;
			if (combo_mode)
				views[color_attach_3_idx] = frameAttachments[i].weights_idx.view;
			views[swapchain_attach_idx] = swapchain_buffers[i].view;
		}
		else
		{
			views[1] = swapchain_buffers[i].view;
		}

		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &nerf_framebuffers[i]));
	}
}

void MobileNerf::update_descriptor_sets_baseline()
{
	for (int i = 0; i < nerf_framebuffers.size(); i++)
	{
		std::vector<VkDescriptorImageInfo> attachment_input_descriptors;
		attachment_input_descriptors.resize(combo_mode ? 4 : 3);

		attachment_input_descriptors[0].sampler     = VK_NULL_HANDLE;
		attachment_input_descriptors[0].imageView   = frameAttachments[i].feature_0.view;
		attachment_input_descriptors[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		attachment_input_descriptors[1].sampler     = VK_NULL_HANDLE;
		attachment_input_descriptors[1].imageView   = frameAttachments[i].feature_1.view;
		attachment_input_descriptors[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		attachment_input_descriptors[2].sampler     = VK_NULL_HANDLE;
		attachment_input_descriptors[2].imageView   = frameAttachments[i].feature_2.view;
		attachment_input_descriptors[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (combo_mode)
		{
			attachment_input_descriptors[3].sampler     = VK_NULL_HANDLE;
			attachment_input_descriptors[3].imageView   = frameAttachments[i].weights_idx.view;
			attachment_input_descriptors[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		VkWriteDescriptorSet texture_input_write_0 = vkb::initializers::write_descriptor_set(descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, &attachment_input_descriptors[0]);
		VkWriteDescriptorSet texture_input_write_1 = vkb::initializers::write_descriptor_set(descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &attachment_input_descriptors[1]);
		VkWriteDescriptorSet texture_input_write_2 = vkb::initializers::write_descriptor_set(descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, &attachment_input_descriptors[2]);

		if (combo_mode)
		{
			VkWriteDescriptorSet texture_input_write_3 = vkb::initializers::write_descriptor_set(descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3, &attachment_input_descriptors[3]);

			std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
			    texture_input_write_0,
			    texture_input_write_1,
			    texture_input_write_2,
			    texture_input_write_3};

			vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
		}
		else
		{
			std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
			    texture_input_write_0,
			    texture_input_write_1,
			    texture_input_write_2};

			vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
		}
	}
}

void MobileNerf::build_command_buffers()
{
	if (use_native_screen_size)
	{
		view_port_height = height;
		view_port_width  = width;
	}
	build_command_buffers_baseline();
}

void MobileNerf::build_command_buffers_baseline()
{
	// In case the screen is resized, need to update the storage image size and descriptor set
	// Note that the texture_rendered image has already been recreated at this point
	if (!prepared)
	{
		setup_nerf_framebuffer_baseline();

		if (use_deferred)
		{
			update_descriptor_sets_baseline();
		}
	}

	VkCommandBufferBeginInfo  command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();
	std::vector<VkClearValue> clear_values;

	if (use_deferred)
	{
		if (combo_mode)
		{
			clear_values.resize(6);
			clear_values[0].color        = {{0.025f, 0.025f, 0.025f, 0.5f}};        // default_clear_color;
			clear_values[1].color        = {{0.025f, 0.025f, 0.025f, 0.5f}};        // default_clear_color;
			clear_values[2].color        = {{0.025f, 0.025f, 0.025f, 0.5f}};        // default_clear_color;
			clear_values[3].color        = {{0, 0, 0, 0}};                          // default clear index for weights;
			clear_values[4].depthStencil = {1.0f, 0};
			clear_values[5].color        = {{1.0f, 1.0f, 1.0f, 0.5f}};        // default_clear_color;
		}
		else
		{
			clear_values.resize(5);
			clear_values[0].color        = {{0.025f, 0.025f, 0.025f, 0.5f}};        // default_clear_color;
			clear_values[1].color        = {{0.025f, 0.025f, 0.025f, 0.5f}};        // default_clear_color;
			clear_values[2].color        = {{0.025f, 0.025f, 0.025f, 0.5f}};        // default_clear_color;
			clear_values[3].depthStencil = {1.0f, 0};
			clear_values[4].color        = {{1.0f, 1.0f, 1.0f, 0.5f}};        // default_clear_color;
		}
	}
	else
	{
		clear_values.resize(2);
		clear_values[0].depthStencil = {1.0f, 0};
		clear_values[1].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};        // let's use this to distinguish forward rendering and deferred renderding
	}

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass_nerf;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = static_cast<uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues             = clear_values.data();

	VkClearValue clear_values_UI[2];
	clear_values_UI[0].color        = default_clear_color;
	clear_values_UI[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info_UI    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info_UI.renderPass               = render_pass;
	render_pass_begin_info_UI.renderArea.offset.x      = 0;
	render_pass_begin_info_UI.renderArea.offset.y      = 0;
	render_pass_begin_info_UI.renderArea.extent.width  = width;
	render_pass_begin_info_UI.renderArea.extent.height = height;
	render_pass_begin_info_UI.clearValueCount          = 2;
	render_pass_begin_info_UI.pClearValues             = clear_values_UI;

	VkImageSubresourceRange subresource_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	for (size_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = nerf_framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// First sub pass
		// Fills the attachments

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		const auto scissor  = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		auto &ii = instancing_info;
		for (auto &model : models)
		{
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, model.pipeline_first_pass);
			// If deferred, only use the first descriptor bounded with the model
			// If forward, each model has the swapchan number of descriptor
			int descriptorIndex = use_deferred ? 0 : static_cast<uint32_t>(i);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_first_pass_layout,
			                        0, 1, &model.descriptor_set_first_pass[descriptorIndex], 0, nullptr);
			VkDeviceSize offsets[1] = {0};
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, model.vertex_buffer->get(), offsets);
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 1, 1, instance_buffer->get(), offsets);
			vkCmdBindIndexBuffer(draw_cmd_buffers[i], model.index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);
			if (use_deferred && combo_mode)
			{
				PushConstants constants = {static_cast<unsigned int>(model.model_index)};
				vkCmdPushConstants(
				    draw_cmd_buffers[i],
				    pipeline_first_pass_layout,
				    VK_SHADER_STAGE_FRAGMENT_BIT,
				    0,
				    sizeof(PushConstants),
				    &constants);
			}
			vkCmdDrawIndexed(draw_cmd_buffers[i], static_cast<uint32_t>(model.indices.size()) * 3, ii.dim.x * ii.dim.y * ii.dim.z, 0, 0, 0);
		}

		if (use_deferred)
		{
			// Second sub pass
			// Render a full screen quad, reading from the previously written attachments via input attachments

			vkCmdNextSubpass(draw_cmd_buffers[i], VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_baseline);
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_baseline, 0, 1, &descriptor_set_baseline[i], 0, NULL);
			vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}
		else
		{
			vkCmdEndRenderPass(draw_cmd_buffers[i]);
		}

		// Render UI

		render_pass_begin_info_UI.framebuffer = framebuffers[i];

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info_UI, VK_SUBPASS_CONTENTS_INLINE);
		draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void MobileNerf::load_scene(int model_index, int sub_model_index, int models_entry)
{
	Model &model = models[models_entry];

	vkb::GLTFLoader loader{get_device()};
	int             total_sub_sub_model = using_original_nerf_models[model_index] ? 8 : 1;

	for (int sub_model = 0; sub_model < total_sub_sub_model; sub_model++)
	{
		std::string inputfile(model_path[model_index] + "shape" + std::to_string(sub_model_index));

		if (total_sub_sub_model > 1)
		{
			inputfile += ("_" + std::to_string(sub_model) + ".gltf");
		}
		else
		{
			inputfile += (".gltf");
		}

		LOGI("Parsing nerf obj {}", inputfile);

		auto scene = loader.read_scene_from_file(inputfile);

		for (auto &&mesh : scene->get_components<vkb::sg::Mesh>())
		{
			for (auto &&sub_mesh : mesh->get_submeshes())
			{
				auto       pts_               = CopyBuffer<glm::vec3>{}(sub_mesh->vertex_buffers, "position");
				const auto texcoord_          = CopyBuffer<glm::vec2>{}(sub_mesh->vertex_buffers, "texcoord_0");
				const auto vertex_start_index = static_cast<uint32_t>(model.vertices.size());

				// Copy vertex data
				{
					model.vertices.resize(vertex_start_index + pts_.size());
					for (size_t i = 0; i < pts_.size(); ++i)
					{
						model.vertices[vertex_start_index + i].position  = pts_[i];
						model.vertices[vertex_start_index + i].tex_coord = glm::vec2(texcoord_[i].x, 1.0f - texcoord_[i].y);
					}
				}

				// Copy index data
				{
					auto index_buffer_ = sub_mesh->index_buffer.get();
					if (index_buffer_)
					{
						assert(sub_mesh->index_type == VkIndexType::VK_INDEX_TYPE_UINT32);
						const size_t sz                   = index_buffer_->get_size();
						const size_t nTriangles           = sz / sizeof(uint32_t) / 3;
						const auto   triangle_start_index = static_cast<uint32_t>(model.indices.size());
						model.indices.resize(triangle_start_index + nTriangles);
						auto ptr = index_buffer_->get_data();
						assert(!!ptr);
						std::vector<uint32_t> tempBuffer(nTriangles * 3);
						memcpy(&tempBuffer[0], ptr, sz);
						for (size_t i = 0; i < nTriangles; ++i)
						{
							model.indices[triangle_start_index + i] = {vertex_start_index + static_cast<uint32_t>(tempBuffer[3 * i]),
							                                           vertex_start_index + static_cast<uint32_t>(tempBuffer[3 * i + 1]),
							                                           vertex_start_index + static_cast<uint32_t>(tempBuffer[3 * i + 2])};
						}
					}
				}
			}
		}
	}
}

void MobileNerf::create_descriptor_pool()
{
	if (use_deferred)
	{
		std::vector<VkDescriptorPoolSize> pool_sizes = {
		    // First Pass
		    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * static_cast<uint32_t>(models.size())},
		    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * static_cast<uint32_t>(models.size())},
		};
		// Second Pass
		if (combo_mode)
		{
			pool_sizes.push_back({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 4 * static_cast<uint32_t>(framebuffers.size())});
			pool_sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * static_cast<uint32_t>(framebuffers.size()) * static_cast<uint32_t>(model_path.size())});
		}
		else
		{
			pool_sizes.push_back({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3 * static_cast<uint32_t>(framebuffers.size())});
			pool_sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * static_cast<uint32_t>(framebuffers.size())});
		}

		VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, static_cast<uint32_t>(models.size() + framebuffers.size()));
		VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
	}
	else
	{
		std::vector<VkDescriptorPoolSize> pool_sizes = {
		    // First Pass
		    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * static_cast<uint32_t>(models.size()) * static_cast<uint32_t>(framebuffers.size())},
		    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * static_cast<uint32_t>(models.size()) * static_cast<uint32_t>(framebuffers.size())},
		    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * static_cast<uint32_t>(models.size()) * static_cast<uint32_t>(framebuffers.size())}};

		VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, static_cast<uint32_t>(models.size() * framebuffers.size()));
		VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
	}
}

void MobileNerf::create_pipeline_layout_fist_pass()
{
	// First Pass Descriptor set and layout

	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 2)};

	// If use forward, add uniform buffer descriptor for the weights
	if (!use_deferred)
	{
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3));
	}

	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_first_pass_layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_first_pass_layout,
	        1);

	if (use_deferred && combo_mode)
	{
		VkPushConstantRange pushConstantRange = vkb::initializers::push_constant_range(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushConstants), 0);

		pipeline_layout_create_info.pushConstantRangeCount = 1;
		pipeline_layout_create_info.pPushConstantRanges    = &pushConstantRange;
		VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_first_pass_layout));
	}
	else
	{
		VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_first_pass_layout));
	}
}

void MobileNerf::create_pipeline_layout_baseline()
{
	// Second Pass Descriptor set and layout

	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
	    // Two output color from the first pass and ray direction
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
	};
	if (combo_mode)
	{
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 3));
		// MLP weights array, using descriptor indexing
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, static_cast<uint32_t>(model_path.size())));
	}
	else
	{
		// MLP weights
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3));
	}

	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));
	if (combo_mode)
	{
		VkDescriptorBindingFlagsEXT                    flags[5] = {0, 0, 0, 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT};
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
		setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		setLayoutBindingFlags.bindingCount  = 5;
		setLayoutBindingFlags.pBindingFlags = flags;
		descriptor_layout.pNext             = &setLayoutBindingFlags;
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout_baseline));
	}
	else
	{
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout_baseline));
	}

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        &descriptor_set_layout_baseline,
	        1);

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout_baseline));
}

void MobileNerf::create_descriptor_sets_first_pass(Model &model)
{
	int numDescriptorPerModel = use_deferred ? 1 : static_cast<int>(nerf_framebuffers.size());
	model.descriptor_set_first_pass.resize(numDescriptorPerModel);

	for (int i = 0; i < numDescriptorPerModel; i++)
	{
		VkDescriptorSetAllocateInfo descriptor_set_allocate_info =
		    vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_first_pass_layout, 1);
		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &model.descriptor_set_first_pass[i]));

		std::array<VkDescriptorImageInfo, 2> texture_input_descriptors;

		texture_input_descriptors[0].sampler     = model.texture_input_0.sampler;
		texture_input_descriptors[0].imageView   = model.texture_input_0.image->get_vk_image_view().get_handle();
		texture_input_descriptors[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		texture_input_descriptors[1].sampler     = model.texture_input_1.sampler;
		texture_input_descriptors[1].imageView   = model.texture_input_1.image->get_vk_image_view().get_handle();
		texture_input_descriptors[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffers[model.model_index]);

		VkWriteDescriptorSet texture_input_write_0 = vkb::initializers::write_descriptor_set(model.descriptor_set_first_pass[i],
		                                                                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texture_input_descriptors[0]);
		VkWriteDescriptorSet texture_input_write_1 = vkb::initializers::write_descriptor_set(model.descriptor_set_first_pass[i],
		                                                                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &texture_input_descriptors[1]);
		VkWriteDescriptorSet uniform_buffer_write  = vkb::initializers::write_descriptor_set(model.descriptor_set_first_pass[i],
		                                                                                     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &buffer_descriptor);

		std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
		    texture_input_write_0,
		    texture_input_write_1,
		    uniform_buffer_write};

		VkDescriptorBufferInfo weights_buffer_descriptor;

		if (!use_deferred)
		{
			// Add in descriptor sets for MLP weights
			weights_buffer_descriptor = create_descriptor(*weights_buffers[model.model_index]);
			// Add in descriptor sets for MLP weights
			VkWriteDescriptorSet weights_buffer_write = vkb::initializers::write_descriptor_set(model.descriptor_set_first_pass[i],
			                                                                                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &weights_buffer_descriptor);
			write_descriptor_sets.push_back(weights_buffer_write);
		}

		vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
	}
}

void MobileNerf::create_descriptor_sets_baseline()
{
	descriptor_set_baseline.resize(nerf_framebuffers.size());

	for (int i = 0; i < nerf_framebuffers.size(); i++)
	{
		VkDescriptorSetAllocateInfo descriptor_set_allocate_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout_baseline, 1);
		if (combo_mode)
		{
			uint32_t counts[1];
			counts[0] = static_cast<uint32_t>(model_path.size());

			VkDescriptorSetVariableDescriptorCountAllocateInfo set_counts = {};
			set_counts.sType                                              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
			set_counts.descriptorSetCount                                 = 1;
			set_counts.pDescriptorCounts                                  = counts;

			descriptor_set_allocate_info.pNext = &set_counts;

			VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_set_baseline[i]));
		}
		else
		{
			VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_set_baseline[i]));
		}

		std::vector<VkDescriptorImageInfo> attachment_input_descriptors;
		attachment_input_descriptors.resize(combo_mode ? 4 : 3);

		attachment_input_descriptors[0].sampler     = VK_NULL_HANDLE;
		attachment_input_descriptors[0].imageView   = frameAttachments[i].feature_0.view;
		attachment_input_descriptors[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		attachment_input_descriptors[1].sampler     = VK_NULL_HANDLE;
		attachment_input_descriptors[1].imageView   = frameAttachments[i].feature_1.view;
		attachment_input_descriptors[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		attachment_input_descriptors[2].sampler     = VK_NULL_HANDLE;
		attachment_input_descriptors[2].imageView   = frameAttachments[i].feature_2.view;
		attachment_input_descriptors[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet texture_input_write_0 = vkb::initializers::write_descriptor_set(descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, &attachment_input_descriptors[0]);
		VkWriteDescriptorSet texture_input_write_1 = vkb::initializers::write_descriptor_set(descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &attachment_input_descriptors[1]);
		VkWriteDescriptorSet texture_input_write_2 = vkb::initializers::write_descriptor_set(descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, &attachment_input_descriptors[2]);

		if (combo_mode)
		{
			attachment_input_descriptors[3].sampler     = VK_NULL_HANDLE;
			attachment_input_descriptors[3].imageView   = frameAttachments[i].weights_idx.view;
			attachment_input_descriptors[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			VkWriteDescriptorSet texture_input_write_3  = vkb::initializers::write_descriptor_set(descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3, &attachment_input_descriptors[3]);

			std::vector<VkDescriptorBufferInfo> weights_buffer_descriptors;
			weights_buffer_descriptors.reserve(mlp_weight_vector.size());
			for (auto &weight_buffer : weights_buffers)
			{
				weights_buffer_descriptors.emplace_back(create_descriptor(*weight_buffer));
			}
			VkWriteDescriptorSet weights_buffer_write = vkb::initializers::write_descriptor_set(
			    descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, weights_buffer_descriptors.data(), static_cast<uint32_t>(weights_buffer_descriptors.size()));

			std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
			    texture_input_write_0,
			    texture_input_write_1,
			    texture_input_write_2,
			    texture_input_write_3,
			    weights_buffer_write};

			vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
		}
		else
		{
			VkDescriptorBufferInfo weights_buffer_descriptor = create_descriptor(*weights_buffers[models[0].model_index]);
			VkWriteDescriptorSet   weights_buffer_write      = vkb::initializers::write_descriptor_set(descriptor_set_baseline[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &weights_buffer_descriptor);        // UBO

			std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
			    texture_input_write_0,
			    texture_input_write_1,
			    texture_input_write_2,
			    weights_buffer_write};

			vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
		}
	}
}

void MobileNerf::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, /*VK_CULL_MODE_BACK_BIT*/ VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE /*VK_FRONT_FACE_CLOCKWISE*/, 0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states;
	blend_attachment_states.push_back(vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE));

	if (use_deferred)
	{
		blend_attachment_states.push_back(vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE));
		blend_attachment_states.push_back(vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE));
		if (combo_mode)
			blend_attachment_states.push_back(vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE));
	}

	VkPipelineColorBlendStateCreateInfo color_blend_state = vkb::initializers::pipeline_color_blend_state_create_info(static_cast<uint32_t>(blend_attachment_states.size()), blend_attachment_states.data());

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	depth_stencil_state.depthBoundsTestEnable                 = VK_FALSE;
	depth_stencil_state.minDepthBounds                        = 0.f;
	depth_stencil_state.maxDepthBounds                        = 1.f;

	VkPipelineViewportStateCreateInfo viewport_state = vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(
	        dynamic_state_enables.data(),
	        static_cast<uint32_t>(dynamic_state_enables.size()),
	        0);

	VkPipelineMultisampleStateCreateInfo multisample_state = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	// Vertex bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertex_input_bindings = {
	    vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	    vkb::initializers::vertex_input_binding_description(1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE),
	};
	const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes = {
	    vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),
	    vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex_coord)),
	    vkb::initializers::vertex_input_attribute_description(1, 2, VK_FORMAT_R32G32B32_SFLOAT, 0),
	};
	VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions           = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions         = vertex_input_attributes.data();

	// First Pass

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_first_pass_layout, render_pass_nerf, 0);
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pRasterizationState          = &rasterization_state;
	pipeline_create_info.pColorBlendState             = &color_blend_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;
	pipeline_create_info.subpass                      = 0;
	pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages_first_pass.size());
	pipeline_create_info.pStages                      = shader_stages_first_pass.data();

	// Each model will have its own pipeline
	for (auto &model : models)
	{
		VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &model.pipeline_first_pass));
	}

	if (use_deferred)
	{
		// Second Pass

		pipeline_create_info.layout  = pipeline_layout_baseline;
		pipeline_create_info.subpass = 1;

		VkPipelineVertexInputStateCreateInfo emptyInputStateCI{};
		emptyInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		pipeline_create_info.pVertexInputState = &emptyInputStateCI;
		color_blend_state.attachmentCount      = 1;
		rasterization_state.cullMode           = VK_CULL_MODE_NONE;
		depth_stencil_state.depthWriteEnable   = VK_FALSE;
		pipeline_create_info.stageCount        = static_cast<uint32_t>(shader_stages_second_pass.size());
		pipeline_create_info.pStages           = shader_stages_second_pass.data();

		VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline_baseline));
	}
}

void MobileNerf::create_static_object_buffers(int model_index, int sub_model_index, int models_entry)
{
	LOGI("Creating static object buffers");
	Model &model              = models[models_entry];
	auto   vertex_buffer_size = model.vertices.size() * sizeof(Vertex);
	auto   index_buffer_size  = model.indices.size() * sizeof(model.indices[0]);

	// Create destination buffers
	model.vertex_buffer = std::make_unique<vkb::core::BufferC>(
	    get_device(),
	    vertex_buffer_size,
	    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY);
	model.vertex_buffer->set_debug_name(fmt::format("Model #{} Sub-Model #{} vertices", model_index, sub_model_index));
	model.index_buffer = std::make_unique<vkb::core::BufferC>(
	    get_device(),
	    index_buffer_size,
	    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY);
	model.index_buffer->set_debug_name(fmt::format("Model #{} Sub-Model #{} indices", model_index, sub_model_index));

	// Create staging buffers
	std::unique_ptr<vkb::core::BufferC> staging_vertex_buffer = std::make_unique<vkb::core::BufferC>(
	    get_device(),
	    vertex_buffer_size,
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU);
	staging_vertex_buffer->update(model.vertices);

	std::unique_ptr<vkb::core::BufferC> staging_index_buffer = std::make_unique<vkb::core::BufferC>(
	    get_device(),
	    index_buffer_size,
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU);
	staging_index_buffer->update(model.indices);

	// Copy over the data for each of the models
	with_vkb_command_buffer([&](vkb::CommandBuffer &cmd) {
		cmd.copy_buffer(*staging_vertex_buffer, *model.vertex_buffer, staging_vertex_buffer->get_size());
		cmd.copy_buffer(*staging_index_buffer, *model.index_buffer, staging_index_buffer->get_size());
	});

	LOGI("Done Creating static object buffers");
}

void MobileNerf::create_uniforms()
{
	uniform_buffers.resize(model_path.size());
	weights_buffers.resize(model_path.size());

	for (int i = 0; i < model_path.size(); i++)
	{
		LOGI("Creating camera view uniform buffer for model {}", i);
		uniform_buffers[i] = std::make_unique<vkb::core::BufferC>(get_device(),
		                                                          sizeof(global_uniform),
		                                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		                                                          VMA_MEMORY_USAGE_CPU_TO_GPU);

		LOGI("Creating mlp weights uniform buffer for model {}", i);
		weights_buffers[i] = std::make_unique<vkb::core::BufferC>(get_device(),
		                                                          sizeof(MLP_Weights),
		                                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		                                                          VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	update_uniform_buffers();
	update_weights_buffers();
}

void MobileNerf::initialize_mlp_uniform_buffers(int model_index)
{
	std::string assetBase   = vkb::fs::path::get(vkb::fs::path::Type::Assets);
	std::string mlpJsonPath = assetBase + model_path[model_index] + "mlp.json";

	using json = nlohmann::json;

	std::ifstream f(mlpJsonPath);

	if (!f)
	{
		LOGE("Failed to open mlp data");
		assert(0);
	}

	LOGI("Parsing mlp data {}", mlpJsonPath);
	json data = json::parse(f);

	// Record a index of the first sub-model
	int first_sub_model = static_cast<int>(models.size());
	int obj_num         = data["obj_num"].get<int>();

	// Here we know the actual number of sub models
	int next_sub_model_index = static_cast<int>(models.size());
	models.resize(models.size() + obj_num);

	for (int i = next_sub_model_index; i < models.size(); i++)
	{
		models[i].model_index = model_index;
	}

	auto weights_0_array_raw = data["0_weights"].get<std::vector<std::vector<float>>>();

	std::vector<float> weights_0_array;

	for (auto ii = weights_0_array_raw.begin(); ii != weights_0_array_raw.end(); ii++)
	{
		weights_0_array.insert(weights_0_array.end(), (*ii).begin(), (*ii).end());
	}

	if (weights_0_array.size() != WEIGHTS_0_COUNT)
	{
		LOGE("MLP data layer 0 weights count is {}, rather than {}", weights_0_array.size(), WEIGHTS_0_COUNT);
	}

	auto bias_0_array = data["0_bias"].get<std::vector<float>>();

	if (bias_0_array.size() != BIAS_0_COUNT)
	{
		LOGE("MLP data layer 0 bias count is {}, rather than {}", bias_0_array.size(), BIAS_0_COUNT);
	}

	auto weights_1_array_raw = data["1_weights"].get<std::vector<std::vector<float>>>();

	std::vector<float> weights_1_array;

	for (auto ii = weights_1_array_raw.begin(); ii != weights_1_array_raw.end(); ii++)
	{
		weights_1_array.insert(weights_1_array.end(), (*ii).begin(), (*ii).end());
	}

	if (weights_1_array.size() != WEIGHTS_1_COUNT)
	{
		LOGE("MLP data layer 1 weights count is {}, rather than {}", weights_1_array.size(), WEIGHTS_1_COUNT);
	}

	auto bias_1_array = data["1_bias"].get<std::vector<float>>();

	if (bias_1_array.size() != BIAS_1_COUNT)
	{
		LOGE("MLP data layer 1 bias count is {}, rather than {}", bias_1_array.size(), BIAS_1_COUNT);
	}

	auto weights_2_array_raw = data["2_weights"].get<std::vector<std::vector<float>>>();

	std::vector<float> weights_2_array;

	for (auto ii = weights_2_array_raw.begin(); ii != weights_2_array_raw.end(); ii++)
	{
		weights_2_array.insert(weights_2_array.end(), (*ii).begin(), (*ii).end());
	}

	// We need to pad the layer 2's weights with 16 zeros
	if (weights_2_array.size() != WEIGHTS_2_COUNT - 16)
	{
		LOGE("MLP data layer 2 weights count is {}, rather than {}", weights_2_array.size(), WEIGHTS_2_COUNT);
	}

	auto bias_2_array = data["2_bias"].get<std::vector<float>>();

	if (bias_2_array.size() != BIAS_2_COUNT - 1)
	{
		LOGE("MLP data layer 2 bias count is {}, rather than {}", bias_2_array.size(), BIAS_2_COUNT);
	}

	// Each sub model will share the same mlp weights data
	MLP_Weights &model_mlp = mlp_weight_vector[model_index];

	for (int ii = 0; ii < WEIGHTS_0_COUNT; ii++)
	{
		model_mlp.data[ii] = weights_0_array[ii];
	}

	for (int ii = 0; ii < WEIGHTS_1_COUNT; ii++)
	{
		model_mlp.data[WEIGHTS_0_COUNT + ii] = weights_1_array[ii];
	}

	// We need to pad the layer 2's weights with zeros for every 3 weights to make it 16 bytes aligned
	int raw_weight_cnt = 0;
	for (int ii = 0; ii < WEIGHTS_2_COUNT; ii++)
	{
		if ((ii + 1) % 4 == 0)
		{
			model_mlp.data[WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + ii] = 0.0f;
		}
		else
		{
			model_mlp.data[WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + ii] = weights_2_array[raw_weight_cnt++];
		}
	}

	for (int ii = 0; ii < BIAS_0_COUNT; ii++)
	{
		model_mlp.data[WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT + ii] = bias_0_array[ii];
	}

	for (int ii = 0; ii < BIAS_1_COUNT; ii++)
	{
		model_mlp.data[WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT +
		               BIAS_0_COUNT + ii] = bias_1_array[ii];
	}

	// We need to pad the layer 2's bias with zeros for every 3 weights to make it 16 bytes aligned
	for (int ii = 0; ii < BIAS_2_COUNT; ii++)
	{
		if ((ii + 1) % 4 == 0)
		{
			model_mlp.data[WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT +
			               BIAS_0_COUNT + BIAS_1_COUNT + ii] = 0.0f;
		}
		else
		{
			model_mlp.data[WEIGHTS_0_COUNT + WEIGHTS_1_COUNT + WEIGHTS_2_COUNT +
			               BIAS_0_COUNT + BIAS_1_COUNT + ii] = bias_2_array[ii];
		}
	}

	// Update all sub model with the same mlp weight
	for (int i = 0; i < obj_num; i++)
	{
		models[first_sub_model + i].sub_model_num = obj_num;
	}
}

void MobileNerf::update_uniform_buffers()
{
	assert(uniform_buffers[0]);

	const float tan_half_fov = tan(0.5 * fov / 180.0f * 3.141592653589793f);

	global_uniform.proj            = camera.matrices.perspective;
	global_uniform.view            = camera.matrices.view;
	global_uniform.camera_position = camera.position;
	global_uniform.camera_side     = glm::vec3(camera.matrices.view[0][0], camera.matrices.view[1][0], camera.matrices.view[2][0]);
	global_uniform.camera_up       = glm::vec3(camera.matrices.view[0][1], camera.matrices.view[1][1], camera.matrices.view[2][1]);
	global_uniform.camera_lookat   = -glm::vec3(camera.matrices.view[0][2], camera.matrices.view[1][2], camera.matrices.view[2][2]);
	global_uniform.img_dim         = glm::vec2(width, height);
	global_uniform.tan_half_fov    = tan_half_fov;

	for (int i = 0; i < model_path.size(); i++)
	{
		// Note that this is a hard-coded scene setting for the lego_combo
		global_uniform.model = combo_mode ? combo_model_transform[i] : glm::translate(glm::vec3(0.0f));
		uniform_buffers[i]->update(&global_uniform, sizeof(global_uniform));
	}
}

void MobileNerf::update_weights_buffers()
{
	// No need to be updated for every frames
	for (int i = 0; i < model_path.size(); i++)
	{
		weights_buffers[i]->update(&(mlp_weight_vector[i].data[0]), sizeof(MLP_Weights));
	}
}

void MobileNerf::prepare_instance_data()
{
	auto &ii = instancing_info;

	std::vector<InstanceData> instance_data;
	instance_data.resize(ii.dim.x * ii.dim.y * ii.dim.z);

	const glm::vec3 corner_pos = -ii.interval * 0.5f * (glm::vec3(ii.dim - 1));
	int             idx        = 0;
	glm::vec3       offset;
	for (int x = 0; x < ii.dim.x; ++x)
	{
		offset.x = corner_pos.x + ii.interval.x * x;
		for (int y = 0; y < ii.dim.y; ++y)
		{
			offset.y = corner_pos.y + ii.interval.y * y;
			for (int z = 0; z < ii.dim.z; ++z)
			{
				offset.z                        = corner_pos.z + ii.interval.z * z;
				instance_data[idx++].pos_offset = offset;
			}
		}
	}

	auto instance_buffer_size = instance_data.size() * sizeof(InstanceData);

	instance_buffer = std::make_unique<vkb::core::BufferC>(
	    get_device(),
	    instance_buffer_size,
	    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY);

	// Copy over the data for each of the models
	auto staging_instance_buffer = std::make_unique<vkb::core::BufferC>(
	    get_device(),
	    instance_buffer_size,
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU);
	staging_instance_buffer->update(instance_data);

	// now transfer over to the end buffer
	with_vkb_command_buffer([&](vkb::CommandBuffer &cmd) {
		cmd.copy_buffer(*staging_instance_buffer, *instance_buffer, staging_instance_buffer->get_size());
	});
}

void MobileNerf::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void MobileNerf::create_texture(int model_index, int sub_model_index, int models_entry)
{
	// Set up the input texture image

	// TODO(tomatkinson): should load different scenes's feature map from command line
	std::string feature_0_path = model_path[model_index] + "shape" + std::to_string(sub_model_index) + ".pngfeat0.png";
	std::string feature_1_path = model_path[model_index] + "shape" + std::to_string(sub_model_index) + ".pngfeat1.png";

	LOGI("Creating feature texture 0");
	create_texture_helper(feature_0_path, models[models_entry].texture_input_0);
	LOGI("Done Creating feature texture 0");

	LOGI("Creating feature texture 1");
	create_texture_helper(feature_1_path, models[models_entry].texture_input_1);
	LOGI("Done Creating feature texture 0");
}

void MobileNerf::create_texture_helper(std::string const &texturePath, Texture &texture_input)
{
	// Feature textures are in linear space instead of sRGB space
	texture_input = load_texture(texturePath, vkb::sg::Image::Other);
	vkDestroySampler(get_device().get_handle(), texture_input.sampler, nullptr);

	// Calculate valid filter
	VkFilter filter = using_original_nerf_models[0] ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	vkb::make_filters_valid(get_device().get_gpu().get_handle(), texture_input.image->get_format(), &filter);

	VkSamplerCreateInfo samplerCreateInfo     = {};
	samplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter               = filter;
	samplerCreateInfo.minFilter               = filter;
	samplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.minLod                  = 0.0f;
	samplerCreateInfo.maxLod                  = 16.0f;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	VK_CHECK(vkCreateSampler(get_device().get_handle(), &samplerCreateInfo, 0, &texture_input.sampler));
}

void MobileNerf::update_render_pass_nerf_forward()
{
	// For merged shaders, we need 2 attachments (as opposed to 5)
	// 0: Depth attachment
	// 1: Swapchain attachment
	std::array<VkAttachmentDescription, 2> attachments = {};
	// Depth attachment
	attachments[0].format         = depth_format;
	attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// Swapchain  attachment
	attachments[1].format         = get_render_context().get_swapchain().get_format();
	attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment            = 0;
	depth_reference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference swapchain_reference = {};
	swapchain_reference.attachment            = 1;
	swapchain_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass    = {};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &swapchain_reference;
	subpass.pDepthStencilAttachment = &depth_reference;
	subpass.inputAttachmentCount    = 0;
	subpass.pInputAttachments       = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments    = nullptr;
	subpass.pResolveAttachments     = nullptr;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachments.size());
	render_pass_create_info.pAttachments           = attachments.data();
	render_pass_create_info.subpassCount           = 1;
	render_pass_create_info.pSubpasses             = &subpass;

	VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &render_pass_nerf));
}

void MobileNerf::update_render_pass_nerf_baseline()
{
	unsigned int attachment_idx = 0;

	// Color attachment 0 - feature 0 G-buffer
	color_attach_0_idx                          = attachment_idx++;
	VkAttachmentDescription color_description_0 = {};
	color_description_0.format                  = feature_map_format;
	color_description_0.samples                 = VK_SAMPLE_COUNT_1_BIT;
	color_description_0.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_description_0.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_description_0.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_description_0.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_description_0.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	color_description_0.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Color attachment 1 - feature 1 G-buffer
	color_attach_1_idx                          = attachment_idx++;
	VkAttachmentDescription color_description_1 = {};
	color_description_1.format                  = feature_map_format;
	color_description_1.samples                 = VK_SAMPLE_COUNT_1_BIT;
	color_description_1.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_description_1.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_description_1.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_description_1.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_description_1.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	color_description_1.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Color attachment 2 - ray direction G-buffer
	color_attach_2_idx                          = attachment_idx++;
	VkAttachmentDescription color_description_2 = {};
	color_description_2.format                  = VK_FORMAT_R16G16B16A16_SFLOAT;
	color_description_2.samples                 = VK_SAMPLE_COUNT_1_BIT;
	color_description_2.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_description_2.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_description_2.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_description_2.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_description_2.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	color_description_2.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Color attachment 3 - weight index G-buffer
	VkAttachmentDescription color_description_3 = {};
	color_attach_3_idx                          = 3;
	if (combo_mode)
	{
		color_attach_3_idx                 = attachment_idx++;
		color_description_3.format         = VK_FORMAT_R8_UINT;
		color_description_3.samples        = VK_SAMPLE_COUNT_1_BIT;
		color_description_3.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_description_3.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_description_3.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_description_3.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_description_3.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		color_description_3.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	// Depth attachment
	depth_attach_idx                          = attachment_idx++;
	VkAttachmentDescription depth_description = {};
	depth_description.format                  = depth_format;
	depth_description.samples                 = VK_SAMPLE_COUNT_1_BIT;
	depth_description.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_description.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_description.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_description.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_STORE;
	depth_description.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_description.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// Swapchain  attachment
	swapchain_attach_idx                          = attachment_idx++;
	VkAttachmentDescription swapchain_description = {};
	swapchain_description.format                  = get_render_context().get_swapchain().get_format();
	swapchain_description.samples                 = VK_SAMPLE_COUNT_1_BIT;
	swapchain_description.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	swapchain_description.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	swapchain_description.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	swapchain_description.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	swapchain_description.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	swapchain_description.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	std::vector<VkAttachmentDescription> attachments;
	attachments.push_back(color_description_0);
	attachments.push_back(color_description_1);
	attachments.push_back(color_description_2);
	if (combo_mode)
		attachments.push_back(color_description_3);
	attachments.push_back(depth_description);
	attachments.push_back(swapchain_description);

	VkAttachmentReference color_reference_0 = {};
	color_reference_0.attachment            = color_attach_0_idx;
	color_reference_0.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_reference_1 = {};
	color_reference_1.attachment            = color_attach_1_idx;
	color_reference_1.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_reference_2 = {};
	color_reference_2.attachment            = color_attach_2_idx;
	color_reference_2.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_reference_3 = {};
	color_reference_3.attachment            = color_attach_3_idx;
	color_reference_3.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment            = depth_attach_idx;
	depth_reference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference swapchain_reference = {};
	swapchain_reference.attachment            = swapchain_attach_idx;
	swapchain_reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array<VkSubpassDescription, 2> subpassDescriptions{};

	std::vector<VkAttachmentReference> color_references_feature_maps = {color_reference_0, color_reference_1, color_reference_2};
	if (combo_mode)
		color_references_feature_maps.push_back(color_reference_3);

	subpassDescriptions[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[0].colorAttachmentCount    = static_cast<uint32_t>(color_references_feature_maps.size());
	subpassDescriptions[0].pColorAttachments       = color_references_feature_maps.data();
	subpassDescriptions[0].pDepthStencilAttachment = &depth_reference;
	subpassDescriptions[0].inputAttachmentCount    = 0;
	subpassDescriptions[0].pInputAttachments       = nullptr;
	subpassDescriptions[0].preserveAttachmentCount = 0;
	subpassDescriptions[0].pPreserveAttachments    = nullptr;
	subpassDescriptions[0].pResolveAttachments     = nullptr;

	// Color attachments written to in first sub pass will be used as input attachments to be read in the fragment shader
	std::vector<VkAttachmentReference> inputReferences = {
	    {color_attach_0_idx, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},        // Color attachment 0 - feature 0 G-buffer
	    {color_attach_1_idx, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},        // Color attachment 1 - feature 1 G-buffer
	    {color_attach_2_idx, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}         // Color attachment 2 - ray direction G-buffer
	};
	if (combo_mode)
		inputReferences.push_back({color_attach_3_idx, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});        // Color attachment 3 - weight index G-buffer

	subpassDescriptions[1].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[1].colorAttachmentCount    = 1;
	subpassDescriptions[1].pColorAttachments       = &swapchain_reference;
	subpassDescriptions[1].pDepthStencilAttachment = nullptr;
	subpassDescriptions[1].inputAttachmentCount    = static_cast<uint32_t>(inputReferences.size());
	subpassDescriptions[1].pInputAttachments       = inputReferences.data();
	subpassDescriptions[1].preserveAttachmentCount = 0;
	subpassDescriptions[1].pPreserveAttachments    = nullptr;
	subpassDescriptions[1].pResolveAttachments     = nullptr;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 3> dependencies{};

	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = 0;
	dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask   = VK_ACCESS_NONE;
	dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass      = 0;
	dependencies[1].dstSubpass      = 1;
	dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[2].srcSubpass      = 1;
	dependencies[2].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[2].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[2].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[2].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[2].dstAccessMask   = VK_ACCESS_NONE;
	dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachments.size());
	render_pass_create_info.pAttachments           = attachments.data();
	render_pass_create_info.subpassCount           = static_cast<uint32_t>(subpassDescriptions.size());
	render_pass_create_info.pSubpasses             = subpassDescriptions.data();
	render_pass_create_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
	render_pass_create_info.pDependencies          = dependencies.data();

	VK_CHECK(vkCreateRenderPass(get_device().get_handle(), &render_pass_create_info, nullptr, &render_pass_nerf));
}

std::unique_ptr<vkb::VulkanSampleC> create_mobile_nerf()
{
	return std::make_unique<MobileNerf>();
}
