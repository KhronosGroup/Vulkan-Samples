/*
 * Copyright 2023 Nintendo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include "api_vulkan_sample.h"
#include <vector>

class ShaderObject : public ApiVulkanSample
{
  public:
	class Shader
	{
		VkShaderStageFlagBits stage;
		VkShaderStageFlags    next_stage;
		VkShaderEXT           shader      = VK_NULL_HANDLE;
		std::string           shader_name = "shader";
		VkShaderCreateInfoEXT vk_shader_create_info;
		std::vector<uint32_t> spirv;

	  public:
		Shader(){};
		Shader(VkShaderStageFlagBits        stage,
		       VkShaderStageFlags           next_stage,
		       std::string                  name,
		       const std::vector<uint8_t>  &vert_glsl_source,
		       const VkDescriptorSetLayout *pSetLayouts,
		       const VkPushConstantRange   *pPushConstantRange);

		std::string get_name()
		{
			return shader_name;
		}

		VkShaderCreateInfoEXT get_create_info()
		{
			return vk_shader_create_info;
		}

		const VkShaderEXT *get_shader()
		{
			return &shader;
		}

		const VkShaderStageFlagBits *get_stage()
		{
			return &stage;
		}

		const VkShaderStageFlags *get_next_stage()
		{
			return &next_stage;
		}

		void set_shader(VkShaderEXT _shader)
		{
			shader = _shader;
		}

		void destroy(VkDevice device);
	};

	struct Format
	{
		VkFormat    format;
		std::string name;
	};

	struct Image
	{
		VkImage        image{};
		VkImageView    image_view{};
		VkDeviceMemory memory{};
	};

	struct Sampler
	{
		Image     image;
		VkSampler sampler;
	};

	enum ShaderType
	{
		ShaderTypeBasic,
		ShaderTypeMaterial,
		ShaderTypePostProcess,
		ShaderTypeCOUNT
	};

	struct BasicPushConstant
	{
		glm::mat4 model = glm::mat4(1.0f);
	};

	struct MaterialPushConstant
	{
		glm::mat4 model            = glm::mat4(1.0f);
		glm::vec3 camera_pos       = glm::vec3(0);
		float     elapsed_time     = 0;
		float     material_diffuse = 0.8f;
		float     material_spec    = 0.2f;
	};

	struct PostProcessPushConstant
	{
		float elapsed_time = 0;
	};

	struct CurrentShader
	{
		int vert = 0;
		int geo  = 0;
		int frag = 0;
	};

	ShaderObject();
	~ShaderObject() override;

	const std::unordered_map<const char *, bool> get_validation_layers() override;

	bool resize(const uint32_t width, const uint32_t height) override;
	bool prepare(const vkb::ApplicationOptions &options) override;
	void setup_framebuffer() override;
	void setup_render_pass() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void render(float delta_time) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;
	void build_linked_shaders(VkDevice device, Shader *vert, Shader *frag);
	void build_shader(VkDevice device, Shader *shader);
	void bind_shader(VkCommandBuffer cmd_buffer, ShaderObject::Shader *shader);

  private:
	void create_default_sampler();
	void load_assets();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void create_descriptor_pool();
	void setup_descriptor_set_layout();
	void create_descriptor_sets();
	void create_shaders();
	void create_images();
	void initialize_descriptor_sets();
	void generate_terrain();
	void update_descriptor_sets();
	void set_initial_state(VkCommandBuffer cmd);
	void bind_material_shader(VkCommandBuffer cmd_buffer, int shader_index);
	void bind_basic_shader(VkCommandBuffer cmd_buffer, int shader_index);
	void draw(float delta_time);
	void iterate_current();
	void randomize_current();
	void get_timestamp_results();

	VkDescriptorImageInfo create_image_descriptor(Sampler &texture, VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	Image                 create_output_image(VkFormat format, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

	Texture envmap_texture;
	Texture checkerboard_texture;
	Texture terrain_array_textures;
	Texture heightmap_texture;

	struct Terrain
	{
		std::unique_ptr<vkb::core::Buffer> vertices;
		std::unique_ptr<vkb::core::Buffer> indices;
		uint32_t                           index_count;
	} terrain;

	struct CameraMatsUBO
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 proj_view;
	} camera_mats_ubo;

	// Timestamp information for additional CPU frame time tracking
	int                     current_timestamp = 0;
	std::array<float, 2000> timestamp_values{0};

	std::chrono::steady_clock::time_point start_time;

	float elapsed_time           = 0;
	float elapsed_iteration_time = 0;
	float max_iteration_time     = 0.5f;

	std::unique_ptr<vkb::core::Buffer> camera_mats_ubo_buffer;

	std::unique_ptr<vkb::sg::SubMesh> skybox;
	std::unique_ptr<vkb::sg::SubMesh> torus;
	std::unique_ptr<vkb::sg::SubMesh> rock;
	std::unique_ptr<vkb::sg::SubMesh> cube;
	std::unique_ptr<vkb::sg::SubMesh> sphere;
	std::unique_ptr<vkb::sg::SubMesh> teapot;

	VkPushConstantRange   push_constant_ranges[ShaderTypeCOUNT];
	VkDescriptorSet       descriptor_sets[ShaderTypeCOUNT]{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layouts[ShaderTypeCOUNT]{VK_NULL_HANDLE};
	VkDescriptorPool      descriptor_pool{VK_NULL_HANDLE};
	VkPipelineLayout      pipeline_layout[ShaderTypeCOUNT]{VK_NULL_HANDLE};

	Shader *skybox_vert_shader;
	Shader *skybox_frag_shader;

	Shader *terrain_vert_shader;
	Shader *terrain_frag_shader;

	// Simple shaders like normal, etc
	std::vector<Shader *> basic_vert_shaders;
	std::vector<Shader *> basic_frag_shaders;

	// Optional Post Processing shaders that are 1 vert N frag unlinked until runtime
	Shader               *post_process_vert_shader;
	std::vector<Shader *> post_process_frag_shaders;

	// Materials show the flexibility of shader object where no shaders are linked and the
	// vert, geo, and frag are chosen at runtime
	std::vector<Shader *> material_vert_shaders;
	std::vector<Shader *> material_geo_shaders;
	std::vector<Shader *> material_frag_shaders;
	std::vector<Shader *> shader_handles;

	// List of formats that we want to query if the device supports
	std::vector<Format> possible_output_formats{
	    {VK_FORMAT_R8G8B8A8_UNORM, "VK_FORMAT_R8G8B8A8_UNORM"},
	    {VK_FORMAT_R8G8B8A8_SRGB, "VK_FORMAT_R8G8B8A8_SRGB"},
	    {VK_FORMAT_B8G8R8A8_SRGB, "VK_FORMAT_B8G8R8A8_SRGB"},
	    {VK_FORMAT_R16G16B16_UNORM, "VK_FORMAT_R16G16B16_UNORM"},
	    {VK_FORMAT_R16G16B16_SFLOAT, "VK_FORMAT_R16G16B16_SFLOAT"},
	    {VK_FORMAT_R16G16B16A16_UNORM, "VK_FORMAT_R16G16B16A16_UNORM"},
	    {VK_FORMAT_R16G16B16A16_SFLOAT, "VK_FORMAT_R16G16B16A16_SFLOAT"},
	    {VK_FORMAT_R32G32B32A32_SFLOAT, "VK_FORMAT_R32G32B32A32_SFLOAT"},
	    {VK_FORMAT_B10G11R11_UFLOAT_PACK32, "VK_FORMAT_B10G11R11_UFLOAT_PACK32"},
	    {VK_FORMAT_A2R10G10B10_UNORM_PACK32, "VK_FORMAT_A2R10G10B10_UNORM_PACK32"},
	};

	std::vector<Format> possible_depth_formats{
	    {VK_FORMAT_D16_UNORM, "VK_FORMAT_D16_UNORM"},
	    {VK_FORMAT_D32_SFLOAT, "VK_FORMAT_D32_SFLOAT"},
	};

	std::vector<Format> supported_output_formats;
	std::vector<Format> supported_depth_formats;
	std::vector<Image>  output_images;
	std::vector<Image>  depth_images;

	constexpr static int num_basic_objects    = 5;
	constexpr static int num_material_objects = 6;
	int                  current_basic_linked_shaders[num_basic_objects]{};
	CurrentShader        current_material_shaders[num_material_objects]{};

	int current_post_process_shader = 0;
	int current_output_format       = 0;
	int current_depth_format        = 0;

	int selected_basic_object    = 0;
	int selected_material_object = 0;

	bool iterate_basic         = true;
	bool iterate_material_vert = true;
	bool iterate_material_geo  = true;
	bool iterate_material_frag = true;
	bool iterate_post_process  = true;
	bool iterate_output        = true;
	bool iterate_depth         = true;

	bool wireframe_enabled    = false;
	bool wireframe_mode       = false;
	bool post_processing      = true;
	bool iterate_permutations = true;
	bool enable_geometry_pass = true;

	Image     post_process_image;
	Sampler   post_process_input_sampler;
	VkSampler standard_sampler;

	std::default_random_engine rng;
};

std::unique_ptr<vkb::VulkanSample> create_shader_object();
