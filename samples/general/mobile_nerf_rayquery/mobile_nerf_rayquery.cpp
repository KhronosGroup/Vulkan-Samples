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

#include "mobile_nerf_rayquery.h"
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
	std::vector<T> operator()(std::unordered_map<std::string, vkb::core::Buffer> &buffers, const char *buffer_name)
	{
		auto iter = buffers.find(buffer_name);
		if (iter == buffers.cend())
		{
			return {};
		}
		auto          &buffer = iter->second;
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
}        // namespace

void camera_set_look_at(vkb::Camera &camera, const glm::vec3 pos, const glm::vec3 look, const glm::vec3 up)
{
	auto view_matrix = glm::lookAt(pos, look, up);

	glm::vec3 scale;
	glm::quat orientation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(view_matrix, scale, orientation, translation, skew, perspective);

	camera.set_rotation(glm::eulerAngles(orientation) * glm::pi<float>() / 180.f);
	camera.set_translation(translation);
}

MobileNerfRayQuery::MobileNerfRayQuery()
{
	title = "Mobile Nerf Ray Query";

	set_api_version(VK_API_VERSION_1_1);

	// Required by VK_KHR_acceleration_structure
	add_device_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	add_device_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	// Required for ray queries
	add_device_extension(VK_KHR_RAY_QUERY_EXTENSION_NAME);

	// Required by VK_KHR_spirv_1_4
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	// Use this extension for better storage buffers layout
	add_device_extension(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
}

MobileNerfRayQuery::~MobileNerfRayQuery()
{
	if (has_device())
	{
		if (render_pass_nerf)
		{
			vkDestroyRenderPass(get_device().get_handle(), render_pass_nerf, nullptr);
		}

		for (uint32_t i = 0; i < framebuffers_nerf.size(); i++)
		{
			if (framebuffers_nerf[i])
			{
				vkDestroyFramebuffer(get_device().get_handle(), framebuffers_nerf[i], nullptr);
			}
		}

		auto device_ptr = get_device().get_handle();

		for (auto &model : models)
		{
			model.vertex_buffer.reset();
			model.index_buffer.reset();

			vkDestroySampler(get_device().get_handle(), model.texture_input_0.sampler, nullptr);
			vkDestroySampler(get_device().get_handle(), model.texture_input_1.sampler, nullptr);
		}

		vkDestroyPipeline(device_ptr, pipeline, nullptr);
		vkDestroyPipelineLayout(device_ptr, pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(device_ptr, descriptor_set_layout_common, nullptr);
		vkDestroyDescriptorSetLayout(device_ptr, descriptor_set_layout_indices, nullptr);
		vkDestroyDescriptorSetLayout(device_ptr, descriptor_set_layout_vertices, nullptr);
		vkDestroyDescriptorSetLayout(device_ptr, descriptor_set_layout_feature1, nullptr);
		vkDestroyDescriptorSetLayout(device_ptr, descriptor_set_layout_feature2, nullptr);

		for (auto &weights_buffer : weights_buffers)
			weights_buffer.reset();

		uniform_buffer.reset();
	}
}

bool MobileNerfRayQuery::prepare(const vkb::ApplicationOptions &options)
{
	read_json_map();

	// Load the mlp for each model
	mlp_weight_vector.resize(num_models);
	for (int i = 0; i < num_models; i++)
	{
		initialize_mlp_uniform_buffers(i);
	}

	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	load_shaders();
	update_render_pass();
	setup_framebuffers();
	// Because we have our own customized render pass, the UI render pass need to be updated with load on load so it won't
	// clear out the written color attachment
	update_render_pass_flags(RenderPassCreateFlags::ColorAttachmentLoad);

	// Setup camera
	camera.type  = vkb::CameraType::LookAt;
	camera_pos.y = -camera_pos.y;        // flip y to keep consistency of the init pos between rayquery and rasterization
	camera_set_look_at(camera, camera_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.01f, 256.0f);

	// Each models may have submodels
	int models_entry = 0;
	for (int model_index = 0; model_index < num_models; model_index++)
	{
		int num_sub_model = models[models_entry].sub_model_num;

		for (int sub_model_index = 0; sub_model_index < num_sub_model; sub_model_index++)
		{
			load_scene(model_index, sub_model_index, models_entry);
			create_texture(model_index, sub_model_index, models_entry);
			create_static_object_buffers(models_entry);
			create_bottom_level_acceleration_structure(models_entry);
			models_entry++;
		}
	}

	create_top_level_acceleration_structure();
	create_uniforms();
	create_pipeline_layout();
	create_descriptor_pool();
	create_descriptor_sets();
	prepare_pipelines();
	build_command_buffers();

	prepared = true;
	LOGI("Prepare Done!");
	return true;
}

void MobileNerfRayQuery::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	RequestFeature(gpu)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES, &VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR, &VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR, &VkPhysicalDeviceRayQueryFeaturesKHR::rayQuery)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, &VkPhysicalDeviceDescriptorIndexingFeaturesEXT::shaderSampledImageArrayNonUniformIndexing)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, &VkPhysicalDeviceDescriptorIndexingFeaturesEXT::shaderStorageBufferArrayNonUniformIndexing)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, &VkPhysicalDeviceDescriptorIndexingFeaturesEXT::runtimeDescriptorArray)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, &VkPhysicalDeviceDescriptorIndexingFeaturesEXT::descriptorBindingVariableDescriptorCount)
	    .request(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT, &VkPhysicalDeviceScalarBlockLayoutFeaturesEXT::scalarBlockLayout);
}

void MobileNerfRayQuery::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}

	draw();

	update_uniform_buffer();
}

void MobileNerfRayQuery::read_json_map()
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

            "rotation": false,

            "lego_ball":{
                "path": "scenes/morpheus_team/lego_ball_phone/",
                "num_sub_model": 1,
                "camera": [-1, 1, 1],
                "instancing":{
                    "dim": [1, 1, 1],
                    "interval": [2.0, 2.0, 2.0]
                }
            },

            "lego_boba_fett":{
                "path": "scenes/morpheus_team/lego_boba_fett_phone/",
                "num_sub_model": 1,
                "camera": [-1, 1, 1],
                "instancing":{
                    "dim": [1, 1, 1],
                    "interval": [2.0, 2.0, 2.0]
                }
            },

            "lego_monster_truck":{
                "path": "scenes/morpheus_team/lego_monster_truck_phone/",
                "num_sub_model": 1,
                "camera": [-1, 1, 1],
                "instancing":{
                    "dim": [1, 1, 1],
                    "interval": [2.0, 2.0, 2.0]
                }
            },

            "lego_tractor":{
                "path": "scenes/morpheus_team/lego_tractor_phone/",
                "num_sub_model": 1,
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

	// Load combo models or a single model. In combo mode, we have multiple sets of weights.
	if (!asset_map["combo"].is_null())
		combo_mode = asset_map["combo"].get<bool>();
	else
		combo_mode = false;

	if (combo_mode)
	{
		model_path.resize(asset_map["models"].size());
		for (int i = 0; i < model_path.size(); i++)
		{
			model_path[i] = asset_map["models"][i].get<std::string>();
			LOGI("Target model: {}, asset path: {}", target_model, model_path[i]);
		}
	}
	else
	{
		model_path.resize(1);
		model_path[0] = asset_map["path"].get<std::string>();
		LOGI("Target model: {}, asset path: {}", target_model, model_path[0]);
	}
	num_models = static_cast<int>(model_path.size());

	// Read Texture Format
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

	// Rotation mode
	do_rotation = raw_asset_map["rotation"].get<bool>();

	// Read view port size. Use defualt setting (1280x720) if size is 0.
	view_port_width  = raw_asset_map["width"].get<int>();
	view_port_height = raw_asset_map["height"].get<int>();

	if (view_port_width == 0 || view_port_height == 0)
	{
		view_port_width        = width;
		view_port_height       = height;
		use_native_screen_size = true;
	}

	// Read camera position
	if (asset_map["camera"].is_array() && asset_map["camera"].size() == 3)
	{
		camera_pos = glm::vec3(asset_map["camera"][0].get<float>(), asset_map["camera"][1].get<float>(), asset_map["camera"][2].get<float>());
	}
	else
	{
		LOGW("Fail to read camera position. Use defualt value.");
	}

	// Read instancing rendering settings.
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

void MobileNerfRayQuery::initialize_mlp_uniform_buffers(int model_index)
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
	const auto first_sub_model = models.size();
	int        obj_num         = data["obj_num"].get<int>();

	// Here we know the actual number of sub models
	const auto next_sub_model_index = models.size();
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

void MobileNerfRayQuery::load_shaders()
{
	shader_stages[0] = load_shader("mobile_nerf_rayquery/quad.vert", VK_SHADER_STAGE_VERTEX_BIT);
	shader_stages[1] = load_shader(
	    combo_mode ?
	        "mobile_nerf_rayquery/rayquery_morpheus_combo.frag" :
	        "mobile_nerf_rayquery/rayquery_morpheus.frag",
	    VK_SHADER_STAGE_FRAGMENT_BIT);
}

void MobileNerfRayQuery::update_render_pass()
{
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

void MobileNerfRayQuery::setup_framebuffers()
{
	// Delete existing frame buffers
	if (framebuffers_nerf.size() > 0)
	{
		for (uint32_t i = 0; i < framebuffers_nerf.size(); i++)
		{
			if (framebuffers_nerf[i] != VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(get_device().get_handle(), framebuffers_nerf[i], nullptr);
			}
		}
	}

	std::vector<VkImageView> views;
	views.resize(2);
	views[0] = depth_stencil.view;

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

	framebuffers_nerf.resize(swapchain_buffers.size());

	for (uint32_t i = 0; i < framebuffers_nerf.size(); i++)
	{
		views[1] = swapchain_buffers[i].view;
		VK_CHECK(vkCreateFramebuffer(get_device().get_handle(), &framebuffer_create_info, nullptr, &framebuffers_nerf[i]));
	}
}

void MobileNerfRayQuery::load_scene(int model_index, int sub_model_index, int models_entry)
{
	Model &model = models[models_entry];

	vkb::GLTFLoader loader{get_device()};
	int             total_sub_sub_model = 1;

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

void MobileNerfRayQuery::create_texture(int model_index, int sub_model_index, int models_entry)
{
	std::string feature_0_path = model_path[model_index] + "shape" + std::to_string(sub_model_index) + ".pngfeat0.png";
	std::string feature_1_path = model_path[model_index] + "shape" + std::to_string(sub_model_index) + ".pngfeat1.png";

	LOGI("Creating feature texture 0");
	create_texture_helper(feature_0_path, models[models_entry].texture_input_0);
	LOGI("Done Creating feature texture 0");

	LOGI("Creating feature texture 1");
	create_texture_helper(feature_1_path, models[models_entry].texture_input_1);
	LOGI("Done Creating feature texture 0");
}

void MobileNerfRayQuery::create_texture_helper(std::string const &texturePath, Texture &texture_input)
{
	// Feature textures are in linear space instead of sRGB space
	texture_input = load_texture(texturePath, vkb::sg::Image::Other);
	vkDestroySampler(get_device().get_handle(), texture_input.sampler, nullptr);

	// Calculate valid filter
	VkFilter filter = VK_FILTER_LINEAR;
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

void MobileNerfRayQuery::create_static_object_buffers(int models_entry)
{
	LOGI("Creating static object buffers");
	Model &model              = models[models_entry];
	auto   vertex_buffer_size = model.vertices.size() * sizeof(Vertex);
	auto   index_buffer_size  = model.indices.size() * sizeof(model.indices[0]);

	// Note that in contrast to a typical pipeline, our vertex/index buffer requires the acceleration structure build flag in rayquery
	// Create a staging buffer
	const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	const VkBufferUsageFlags staging_flags      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	// Create destination buffers
	model.vertex_buffer = std::make_unique<vkb::core::Buffer>(
	    get_device(),
	    vertex_buffer_size,
	    buffer_usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY);
	model.vertex_buffer->set_debug_name(fmt::format("Model #{} vertices", models_entry));
	model.index_buffer = std::make_unique<vkb::core::Buffer>(
	    get_device(),
	    index_buffer_size,
	    buffer_usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY);
	model.index_buffer->set_debug_name(fmt::format("Model #{} indices", models_entry));

	// Create staging buffers
	std::unique_ptr<vkb::core::Buffer> staging_vertex_buffer = std::make_unique<vkb::core::Buffer>(
	    get_device(),
	    vertex_buffer_size,
	    staging_flags,
	    VMA_MEMORY_USAGE_CPU_TO_GPU);
	staging_vertex_buffer->update(model.vertices);

	std::unique_ptr<vkb::core::Buffer> staging_index_buffer = std::make_unique<vkb::core::Buffer>(
	    get_device(),
	    index_buffer_size,
	    staging_flags,
	    VMA_MEMORY_USAGE_CPU_TO_GPU);
	staging_index_buffer->update(model.indices);

	// Copy over the data for each of the models
	with_vkb_command_buffer([&](vkb::CommandBuffer &cmd) {
		cmd.copy_buffer(*staging_vertex_buffer, *model.vertex_buffer, staging_vertex_buffer->get_size());
		cmd.copy_buffer(*staging_index_buffer, *model.index_buffer, staging_index_buffer->get_size());
	});

	LOGI("Done Creating static object buffers");
}

void MobileNerfRayQuery::create_uniforms()
{
	weights_buffers.resize(num_models);

	LOGI("Creating camera view uniform buffer");
	uniform_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
	                                                     sizeof(global_uniform),
	                                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);

	for (int i = 0; i < num_models; i++)
	{
		LOGI("Creating mlp weights uniform buffer for model {}", i);
		weights_buffers[i] = std::make_unique<vkb::core::Buffer>(get_device(),
		                                                         sizeof(MLP_Weights),
		                                                         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	update_uniform_buffer();
	update_weights_buffers();
}

void MobileNerfRayQuery::update_uniform_buffer()
{
	assert(uniform_buffer);

	camera.set_perspective(fov, (float) width / (float) height, 0.01f, 200.0f);
	const float tan_half_fov = tan(0.5 * fov / 180.0f * 3.141592653589793f);

	global_uniform.camera_position = camera.position;
	global_uniform.camera_side     = glm::vec3(camera.matrices.view[0][0], camera.matrices.view[1][0], camera.matrices.view[2][0]);
	global_uniform.camera_up       = glm::vec3(camera.matrices.view[0][1], camera.matrices.view[1][1], camera.matrices.view[2][1]);
	global_uniform.camera_lookat   = glm::vec3(camera.matrices.view[0][2], camera.matrices.view[1][2], camera.matrices.view[2][2]);
	global_uniform.img_dim         = glm::vec2(width, height);
	global_uniform.tan_half_fov    = tan_half_fov;

	uniform_buffer->update(&global_uniform, sizeof(GlobalUniform));
}

// No need to be updated for every frames
void MobileNerfRayQuery::update_weights_buffers()
{
	for (int i = 0; i < num_models; i++)
	{
		weights_buffers[i]->update(&(mlp_weight_vector[i].data[0]), sizeof(MLP_Weights));
	}
}

uint64_t MobileNerfRayQuery::get_buffer_device_address(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
	buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(get_device().get_handle(), &buffer_device_address_info);
}

void MobileNerfRayQuery::create_top_level_acceleration_structure()
{
	std::vector<VkAccelerationStructureInstanceKHR> acceleration_structure_instances;
	auto                                            add_instance = [&](Model &model, const VkTransformMatrixKHR &transform_matrix, uint32_t instance_index) {
        VkAccelerationStructureInstanceKHR acceleration_structure_instance{};
        acceleration_structure_instance.transform           = transform_matrix;
        acceleration_structure_instance.instanceCustomIndex = instance_index;        // this is the model index instead of the instance index in instancing rendering.
                                                                                     // need this to index correct weights and vertex & index buffer in shader.
        acceleration_structure_instance.mask                                   = 0xFF;
        acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;
        acceleration_structure_instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        acceleration_structure_instance.accelerationStructureReference         = model.bottom_level_acceleration_structure->get_device_address();
        acceleration_structure_instances.emplace_back(acceleration_structure_instance);
	};

	auto     &ii = instancing_info;
	glm::vec3 offset;
	glm::vec3 corner_pos = -ii.interval * 0.5f * (glm::vec3(ii.dim - 1));
	for (int x = 0; x < ii.dim.x; ++x)
	{
		offset.x = corner_pos.x + ii.interval.x * x;
		for (int y = 0; y < ii.dim.y; ++y)
		{
			offset.y = corner_pos.y + ii.interval.y * y;
			for (int z = 0; z < ii.dim.z; ++z)
			{
				offset.z                              = corner_pos.z + ii.interval.z * z;
				VkTransformMatrixKHR transform_matrix = {
				    1.0f,
				    0.0f,
				    0.0f,
				    offset.x,
				    0.0f,
				    1.0f,
				    0.0f,
				    offset.y,
				    0.0f,
				    0.0f,
				    1.0f,
				    offset.z,
				};
				for (size_t i = 0; i < models.size(); ++i)
				{
					add_instance(models[i], transform_matrix, i);
				}
			}
		}
	}

	LOGI("model num: {}", models.size());

	const size_t                       instancesDataSize = sizeof(VkAccelerationStructureInstanceKHR) * acceleration_structure_instances.size();
	std::unique_ptr<vkb::core::Buffer> instances_buffer  = std::make_unique<vkb::core::Buffer>(get_device(),
                                                                                              instancesDataSize,
                                                                                              VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                                                              VMA_MEMORY_USAGE_CPU_TO_GPU);
	instances_buffer->update(acceleration_structure_instances.data(), instancesDataSize);

	top_level_acceleration_structure = std::make_unique<vkb::core::AccelerationStructure>(get_device(), VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
	top_level_acceleration_structure->add_instance_geometry(instances_buffer, acceleration_structure_instances.size());
	top_level_acceleration_structure->build(queue);
}

void MobileNerfRayQuery::create_bottom_level_acceleration_structure(int model_entry)
{
	Model &model = models[model_entry];

	// Create buffers for the bottom level geometry
	// Note that the buffer usage flags for buffers consumed by the bottom level acceleration structure require special flags
	const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	// Set up a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
	VkTransformMatrixKHR transform_matrix = {
	    1.0f, 0.0f, 0.0f, 0.0f,
	    0.0f, 1.0f, 0.0f, 0.0f,
	    0.0f, 0.0f, 1.0f, 0.0f};
	if (combo_mode)
	{
		// Use hard-coded transformation under combo mode
		glm::mat4x4 &M   = combo_model_transform[model_entry];
		transform_matrix = {
		    M[0][0], M[1][0], M[2][0], M[3][0],
		    M[0][1], M[1][1], M[2][1], -M[3][1],
		    M[0][2], M[1][2], M[2][2], M[3][2]};
	}
	std::unique_ptr<vkb::core::Buffer> transform_matrix_buffer = std::make_unique<vkb::core::Buffer>(get_device(), sizeof(transform_matrix), buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);
	transform_matrix_buffer->update(&transform_matrix, sizeof(transform_matrix));

	if (model.bottom_level_acceleration_structure == nullptr)
	{
		model.bottom_level_acceleration_structure = std::make_unique<vkb::core::AccelerationStructure>(
		    get_device(), VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
		model.bottom_level_acceleration_structure->add_triangle_geometry(
		    model.vertex_buffer,
		    model.index_buffer,
		    transform_matrix_buffer,
		    model.indices.size(),
		    model.vertices.size(),
		    sizeof(Vertex),
		    0, VK_FORMAT_R32G32B32_SFLOAT, VK_GEOMETRY_OPAQUE_BIT_KHR,
		    get_buffer_device_address(model.vertex_buffer->get_handle()),
		    get_buffer_device_address(model.index_buffer->get_handle()));
	}
	model.bottom_level_acceleration_structure->build(queue, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR);
}

void MobileNerfRayQuery::create_pipeline_layout()
{
	// Use multiple descriptor sets due to the limitation of using variable size resource array
	// see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_descriptor_indexing.html

	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings_common = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};

	// Add an array of weights sets into shader
	if (combo_mode)
	{
		set_layout_bindings_common.push_back(vkb::initializers::descriptor_set_layout_binding(
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, num_models));
	}
	else
	{
		set_layout_bindings_common.push_back(vkb::initializers::descriptor_set_layout_binding(
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2));
	}
	VkDescriptorSetLayoutCreateInfo descriptor_layout_bounded = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings_common.data(), static_cast<uint32_t>(set_layout_bindings_common.size()));
	if (combo_mode)
	{
		VkDescriptorBindingFlagsEXT                    flags[3] = {0, 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT};
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
		setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		setLayoutBindingFlags.bindingCount  = 3;
		setLayoutBindingFlags.pBindingFlags = flags;
		descriptor_layout_bounded.pNext     = &setLayoutBindingFlags;
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_bounded, nullptr, &descriptor_set_layout_common));
	}
	else
	{
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_bounded, nullptr, &descriptor_set_layout_common));
	}

	auto create_unbounded_descriptor_set_layout = [&](VkDescriptorSetLayout &layout_handle, VkDescriptorSetLayoutBinding &binding) {
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
		setLayoutBindingFlags.sType                        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		setLayoutBindingFlags.bindingCount                 = 1;
		VkDescriptorBindingFlagsEXT descriptorBindingFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
		setLayoutBindingFlags.pBindingFlags                = &descriptorBindingFlags;
		VkDescriptorSetLayoutCreateInfo descriptor_layout  = vkb::initializers::descriptor_set_layout_create_info(&binding, 1);
		descriptor_layout.pNext                            = &setLayoutBindingFlags;
		VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &layout_handle));
	};

	VkDescriptorSetLayoutBinding set_layout_binding_vertices = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, models.size());
	create_unbounded_descriptor_set_layout(descriptor_set_layout_vertices, set_layout_binding_vertices);

	VkDescriptorSetLayoutBinding set_layout_binding_indices = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, models.size());
	create_unbounded_descriptor_set_layout(descriptor_set_layout_indices, set_layout_binding_indices);

	VkDescriptorSetLayoutBinding set_layout_binding_feature1 = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, models.size());
	create_unbounded_descriptor_set_layout(descriptor_set_layout_feature1, set_layout_binding_feature1);

	VkDescriptorSetLayoutBinding set_layout_binding_feature2 = vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, models.size());
	create_unbounded_descriptor_set_layout(descriptor_set_layout_feature2, set_layout_binding_feature2);

	std::vector<VkDescriptorSetLayout> descriptor_set_layouts = {
	    descriptor_set_layout_common,
	    descriptor_set_layout_vertices,
	    descriptor_set_layout_indices,
	    descriptor_set_layout_feature1,
	    descriptor_set_layout_feature2};

	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(
	        descriptor_set_layouts.data(),
	        static_cast<uint32_t>(descriptor_set_layouts.size()));

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}

void MobileNerfRayQuery::create_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
	    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * static_cast<uint32_t>(framebuffers.size())},
	    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 * static_cast<uint32_t>(framebuffers.size())},
	    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * static_cast<uint32_t>(framebuffers.size()) * static_cast<uint32_t>(num_models)},
	    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 * static_cast<uint32_t>(models.size()) * static_cast<uint32_t>(framebuffers.size())},
	    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * static_cast<uint32_t>(models.size()) * static_cast<uint32_t>(framebuffers.size())}};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 5 * static_cast<uint32_t>(framebuffers.size()));
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void MobileNerfRayQuery::create_descriptor_sets()
{
	const auto numDescriptorPerModel = framebuffers.size();
	descriptor_set_common.resize(numDescriptorPerModel);
	descriptor_set_vertices.resize(numDescriptorPerModel);
	descriptor_set_indices.resize(numDescriptorPerModel);
	descriptor_set_feature1.resize(numDescriptorPerModel);
	descriptor_set_feature2.resize(numDescriptorPerModel);

	auto allocate_unbounded_descriptor_set = [&](VkDescriptorSetLayout &descriptor_set_layout, VkDescriptorSet &descriptor_set) {
		uint32_t counts[1];
		counts[0] = static_cast<uint32_t>(models.size());

		VkDescriptorSetVariableDescriptorCountAllocateInfo set_counts = {};
		set_counts.sType                                              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		set_counts.descriptorSetCount                                 = 1;
		set_counts.pDescriptorCounts                                  = counts;

		VkDescriptorSetAllocateInfo descriptor_set_allocate_info =
		    vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
		descriptor_set_allocate_info.pNext = &set_counts;
		VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_set));
	};

	for (int i = 0; i < numDescriptorPerModel; i++)
	{
		if (combo_mode)
		{
			allocate_unbounded_descriptor_set(descriptor_set_layout_common, descriptor_set_common[i]);
		}
		else
		{
			VkDescriptorSetAllocateInfo descriptor_set_allocate_info_common =
			    vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout_common, 1);
			VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info_common, &descriptor_set_common[i]));
		}

		allocate_unbounded_descriptor_set(descriptor_set_layout_vertices, descriptor_set_vertices[i]);
		allocate_unbounded_descriptor_set(descriptor_set_layout_indices, descriptor_set_indices[i]);
		allocate_unbounded_descriptor_set(descriptor_set_layout_feature1, descriptor_set_feature1[i]);
		allocate_unbounded_descriptor_set(descriptor_set_layout_feature2, descriptor_set_feature2[i]);

		uint32_t num_total_submodels = static_cast<uint32_t>(models.size());

		VkDescriptorBufferInfo uniform_buffer_descriptor = create_descriptor(*uniform_buffer);

		std::vector<VkDescriptorBufferInfo> vertex_buffer_descriptors;
		vertex_buffer_descriptors.reserve(num_total_submodels);
		std::vector<VkDescriptorBufferInfo> index_buffer_descriptors;
		index_buffer_descriptors.reserve(num_total_submodels);
		std::vector<VkDescriptorImageInfo> texture_input_1_descriptors;
		texture_input_1_descriptors.reserve(num_total_submodels);
		std::vector<VkDescriptorImageInfo> texture_input_2_descriptors;
		texture_input_2_descriptors.reserve(num_total_submodels);

		for (Model &model : models)
		{
			vertex_buffer_descriptors.emplace_back(create_descriptor(*model.vertex_buffer));
			index_buffer_descriptors.emplace_back(create_descriptor(*model.index_buffer));

			VkDescriptorImageInfo texture_input_1_descriptor{};
			texture_input_1_descriptor.sampler     = model.texture_input_0.sampler;
			texture_input_1_descriptor.imageView   = model.texture_input_0.image->get_vk_image_view().get_handle();
			texture_input_1_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			texture_input_1_descriptors.emplace_back(texture_input_1_descriptor);

			VkDescriptorImageInfo texture_input_2_descriptor{};
			texture_input_2_descriptor.sampler     = model.texture_input_1.sampler;
			texture_input_2_descriptor.imageView   = model.texture_input_1.image->get_vk_image_view().get_handle();
			texture_input_2_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			texture_input_2_descriptors.emplace_back(texture_input_2_descriptor);
		}

		VkWriteDescriptorSet uniform_buffer_write  = vkb::initializers::write_descriptor_set(descriptor_set_common[i],
		                                                                                     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniform_buffer_descriptor);
		VkWriteDescriptorSet vertex_buffer_write   = vkb::initializers::write_descriptor_set(descriptor_set_vertices[i],
		                                                                                     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, vertex_buffer_descriptors.data(), static_cast<uint32_t>(vertex_buffer_descriptors.size()));
		VkWriteDescriptorSet index_buffer_write    = vkb::initializers::write_descriptor_set(descriptor_set_indices[i],
		                                                                                     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, index_buffer_descriptors.data(), static_cast<uint32_t>(index_buffer_descriptors.size()));
		VkWriteDescriptorSet texture_input_write_0 = vkb::initializers::write_descriptor_set(descriptor_set_feature1[i],
		                                                                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, texture_input_1_descriptors.data(), static_cast<uint32_t>(texture_input_1_descriptors.size()));
		VkWriteDescriptorSet texture_input_write_1 = vkb::initializers::write_descriptor_set(descriptor_set_feature2[i],
		                                                                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, texture_input_2_descriptors.data(), static_cast<uint32_t>(texture_input_2_descriptors.size()));

		// Set up the descriptor for binding our top level acceleration structure to the ray tracing shaders
		VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
		descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptor_acceleration_structure_info.accelerationStructureCount = 1;
		auto rhs                                                          = top_level_acceleration_structure->get_handle();
		descriptor_acceleration_structure_info.pAccelerationStructures    = &rhs;

		VkWriteDescriptorSet acceleration_structure_write{};
		acceleration_structure_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		acceleration_structure_write.dstSet          = descriptor_set_common[i];
		acceleration_structure_write.dstBinding      = 1;
		acceleration_structure_write.descriptorCount = 1;
		acceleration_structure_write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		// The acceleration structure descriptor has to be chained via pNext
		acceleration_structure_write.pNext = &descriptor_acceleration_structure_info;

		std::vector<VkWriteDescriptorSet>   write_descriptor_sets;
		VkWriteDescriptorSet                weights_buffer_write;
		std::vector<VkDescriptorBufferInfo> weights_buffer_descriptors;
		VkDescriptorBufferInfo              weights_buffer_descriptor;

		if (combo_mode)
		{
			weights_buffer_descriptors.reserve(mlp_weight_vector.size());
			for (auto &weight_buffer : weights_buffers)
			{
				weights_buffer_descriptors.emplace_back(create_descriptor(*weight_buffer));
			}
			weights_buffer_write = vkb::initializers::write_descriptor_set(descriptor_set_common[i],
			                                                               VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, weights_buffer_descriptors.data(), static_cast<uint32_t>(weights_buffer_descriptors.size()));
		}
		else
		{
			weights_buffer_descriptor = create_descriptor(*weights_buffers[0]);
			weights_buffer_write      = vkb::initializers::write_descriptor_set(descriptor_set_common[i],
			                                                                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &weights_buffer_descriptor);
		}
		write_descriptor_sets = std::vector<VkWriteDescriptorSet>{
		    uniform_buffer_write,
		    acceleration_structure_write,
		    weights_buffer_write,
		    vertex_buffer_write,
		    index_buffer_write,
		    texture_input_write_0,
		    texture_input_write_1};

		vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
	}
}

void MobileNerfRayQuery::prepare_pipelines()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, /*VK_CULL_MODE_BACK_BIT*/ VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE /*VK_FRONT_FACE_CLOCKWISE*/, 0);

	std::vector<VkPipelineColorBlendAttachmentState> blend_attachment_states;
	blend_attachment_states.push_back(vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE));

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

	// No need for Vertex bindings and attributes
	VkPipelineVertexInputStateCreateInfo vertex_input_state{};
	vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkGraphicsPipelineCreateInfo pipeline_create_info = vkb::initializers::pipeline_create_info(pipeline_layout, render_pass_nerf, 0);
	pipeline_create_info.pVertexInputState            = &vertex_input_state;
	pipeline_create_info.pInputAssemblyState          = &input_assembly_state;
	pipeline_create_info.pRasterizationState          = &rasterization_state;
	pipeline_create_info.pColorBlendState             = &color_blend_state;
	pipeline_create_info.pMultisampleState            = &multisample_state;
	pipeline_create_info.pViewportState               = &viewport_state;
	pipeline_create_info.pDepthStencilState           = &depth_stencil_state;
	pipeline_create_info.pDynamicState                = &dynamic_state;
	pipeline_create_info.subpass                      = 0;
	pipeline_create_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages                      = shader_stages.data();

	// Only need one pipeline in rayquery
	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

void MobileNerfRayQuery::build_command_buffers()
{
	if (use_native_screen_size)
	{
		view_port_height = height;
		view_port_width  = width;
	}

	// In case the screen is resized, need to update the storage image size and descriptor set
	// Note that the texture_rendered image has already been recreated at this point
	if (!prepared)
	{
		setup_framebuffers();
	}

	VkCommandBufferBeginInfo  command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();
	std::vector<VkClearValue> clear_values;

	clear_values.resize(2);
	clear_values[0].depthStencil = {1.0f, 0};
	clear_values[1].color        = {{1.0f, 1.0f, 1.0f, 1.0f}};

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
		render_pass_begin_info.framebuffer = framebuffers_nerf[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport((float) view_port_width, (float) view_port_height, 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		// Use 5 descriptor sets due to the limitation of using variable size resource array
		// see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_descriptor_indexing.html
		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		std::vector<VkDescriptorSet> descriptor_sets_first_pass = {
		    descriptor_set_common[i],
		    descriptor_set_vertices[i],
		    descriptor_set_indices[i],
		    descriptor_set_feature1[i],
		    descriptor_set_feature2[i],
		};
		vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
		                        0, static_cast<int32_t>(descriptor_sets_first_pass.size()), descriptor_sets_first_pass.data(), 0, nullptr);
		VkDeviceSize offsets[1] = {0};
		vkCmdDraw(draw_cmd_buffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		// Render UI
		render_pass_begin_info_UI.framebuffer = framebuffers[i];

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info_UI, VK_SUBPASS_CONTENTS_INLINE);
		draw_ui(draw_cmd_buffers[i]);
		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void MobileNerfRayQuery::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

std::unique_ptr<vkb::VulkanSampleC> create_mobile_nerf_rayquery()
{
	return std::make_unique<MobileNerfRayQuery>();
}
