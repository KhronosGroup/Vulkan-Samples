/* Copyright (c) 2019-2024, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "rendering/render_pipeline.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/camera.h"
#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

/**
 * @brief This structure will be pushed in its entirety if 256 bytes of push constants
 *        are supported by the physical device, otherwise it will be trimmed to 128 bytes
 *        (i.e. only "model" and "camera_view_proj" will be pushed)
 *
 *        The shaders will be compiled with a define to handle this difference.
 */
struct alignas(16) MVPUniform
{
	glm::mat4 model;

	glm::mat4 camera_view_proj;

	glm::mat4 scale;

	// This value is ignored by the shader and is just to increase bandwidth
	glm::mat4 padding;
};

/**
 * @brief Constant Data sample
 *
 * This sample is designed to show the different ways in which Vulkan can push constant data to the
 * shaders.
 *
 * The current ways that are supported are:
 *     - Push Constants
 *     - Descriptor Sets
 *     - Dynamic Descriptor Sets
 *     - Update-after-bind Descriptor Sets
 *     - Pre-allocated buffer array
 *
 * The sample also shows the performance implications that these different methods would have on your
 * application or game. These performance deltas may differ between platforms and vendors.
 *
 * The data structure used will be pushed in its entirety if 256 bytes of push constants
 * are supported by the physical device, otherwise it will be trimmed to 128 bytes
 * (i.e. only "model" and "camera_view_proj" will be pushed)
 *
 * The shaders will be compiled with a definition to handle this difference.
 */
class ConstantData : public vkb::VulkanSample<vkb::BindingType::C>
{
  public:
	/**
	 * @brief The sample supported methods of using constant data in shaders
	 */
	enum Method
	{
		PushConstants,
		DescriptorSets,
		DynamicDescriptorSets,
		UpdateAfterBindDescriptorSets,        // May be disabled if the device doesn't support
		BufferArray,
		Undefined
	};

	/**
	 * @brief Describes the properties of a method
	 */
	struct MethodProperties
	{
		const char *description;

		bool supported{true};
	};

	ConstantData();

	virtual ~ConstantData() = default;

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	/**
	 * @brief The base subpass to help prepare the shader variants and store the push constant limit
	 */
	class ConstantDataSubpass : public vkb::ForwardSubpass
	{
	  public:
		ConstantDataSubpass(vkb::RenderContext &render_context, vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader, vkb::sg::Scene &scene, vkb::sg::Camera &camera) :
		    vkb::ForwardSubpass(render_context, std::move(vertex_shader), std::move(fragment_shader), scene, camera)
		{}

		virtual void prepare() override;

		uint32_t struct_size{128};
	};

	/**
	 * @brief A custom forward subpass to isolate just the use of push constants
	 *
	 * This subpass is intentionally set up with custom shaders that possess just a single push constant structure
	 */
	class PushConstantSubpass : public ConstantDataSubpass
	{
	  public:
		PushConstantSubpass(vkb::RenderContext &render_context, vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader, vkb::sg::Scene &scene, vkb::sg::Camera &camera) :
		    ConstantDataSubpass(render_context, std::move(vertex_shader), std::move(fragment_shader), scene, camera)
		{}

		/**
		 * @brief Updates the MVP uniform member variable to then be pushed into the shader
		 */
		virtual void update_uniform(vkb::CommandBuffer &command_buffer, vkb::sg::Node &node, size_t thread_index) override;

		/**
		 * @brief Overridden to intentionally disable any dynamic shader module updates
		 */
		virtual vkb::PipelineLayout &prepare_pipeline_layout(vkb::CommandBuffer &command_buffer, const std::vector<vkb::ShaderModule *> &shader_modules) override;

		/**
		 * @brief Overridden to push a custom data structure to the shader
		 */
		virtual void prepare_push_constants(vkb::CommandBuffer &command_buffer, vkb::sg::SubMesh &sub_mesh) override;

		// The MVP uniform data structure
		MVPUniform mvp_uniform;
	};

	/**
	 * @brief A custom forward subpass to isolate just the use of uniform buffer objects
	 *
	 * This subpass is intentionally set up with custom shaders that possess just a single UBO binding
	 * The subpass will use the right UBO method (Static, Dynamic or Update-after-bind) based on its setting as set by the sample
	 */
	class DescriptorSetSubpass : public ConstantDataSubpass
	{
	  public:
		DescriptorSetSubpass(vkb::RenderContext &render_context, vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader, vkb::sg::Scene &scene, vkb::sg::Camera &camera) :
		    ConstantDataSubpass(render_context, std::move(vertex_shader), std::move(fragment_shader), scene, camera)
		{}

		/**
		 * @brief Creates a buffer filled with the mvp data and binds it
		 */
		virtual void update_uniform(vkb::CommandBuffer &command_buffer, vkb::sg::Node &node, size_t thread_index) override;

		/**
		 * @brief Dynamically retrieves the correct pipeline layout depending on the method of UBO
		 */
		virtual vkb::PipelineLayout &prepare_pipeline_layout(vkb::CommandBuffer &command_buffer, const std::vector<vkb::ShaderModule *> &shader_modules) override;

		/**
		 * @brief Overridden to intentionally disable any push constants
		 */
		virtual void prepare_push_constants(vkb::CommandBuffer &command_buffer, vkb::sg::SubMesh &sub_mesh) override;

		// The method by which the UBO subpass will operate
		Method method;
	};

	/**
	 * @brief A custom forward subpass to isolate the use of a shader storage buffer object
	 *
	 * This subpass is intentionally set up with custom shaders that own just a buffer binding holding an array of mvp data
	 * The subpass will use instancing to index into the UBO array
	 */
	class BufferArraySubpass : public ConstantDataSubpass
	{
	  public:
		BufferArraySubpass(vkb::RenderContext &render_context, vkb::ShaderSource &&vertex_shader, vkb::ShaderSource &&fragment_shader, vkb::sg::Scene &scene, vkb::sg::Camera &camera) :
		    ConstantDataSubpass(render_context, std::move(vertex_shader), std::move(fragment_shader), scene, camera)
		{}

		virtual void draw(vkb::CommandBuffer &command_buffer) override;

		/**
		 * @brief No-op, uniform data is sent upfront before the draw call
		 */
		virtual void update_uniform(vkb::CommandBuffer &command_buffer, vkb::sg::Node &node, size_t thread_index) override;

		/**
		 * @brief Returns a default pipeline layout
		 */
		virtual vkb::PipelineLayout &prepare_pipeline_layout(vkb::CommandBuffer &command_buffer, const std::vector<vkb::ShaderModule *> &shader_modules) override;

		/**
		 * @brief Overridden to intentionally disable any push constants
		 */
		virtual void prepare_push_constants(vkb::CommandBuffer &command_buffer, vkb::sg::SubMesh &sub_mesh) override;

		/**
		 * @brief Overridden to send an index
		 */
		virtual void draw_submesh_command(vkb::CommandBuffer &command_buffer, vkb::sg::SubMesh &sub_mesh) override;

		uint32_t instance_index{0};
	};

	template <typename T>
	std::unique_ptr<vkb::RenderPipeline> create_render_pipeline(const std::string &vertex_shader, const std::string &fragment_shader)
	{
		static_assert(std::is_base_of<ConstantDataSubpass, T>::value, "T is an invalid type. Must be a derived class from ConstantDataSubpass");

		vkb::ShaderSource vert_shader(vertex_shader);
		vkb::ShaderSource frag_shader(fragment_shader);

		auto subpass = std::make_unique<T>(get_render_context(), std::move(vert_shader), std::move(frag_shader), get_scene(), *camera);

		// We want to check if the push constants limit can support the full 256 bytes
		auto push_constant_limit = get_device().get_gpu().get_properties().limits.maxPushConstantsSize;
		if (push_constant_limit >= 256)
		{
			subpass->struct_size = 256;
		}

		std::vector<std::unique_ptr<vkb::Subpass>> subpasses{};
		subpasses.push_back(std::move(subpass));
		return std::make_unique<vkb::RenderPipeline>(std::move(subpasses));
	}

  private:
	virtual void draw_gui() override;

	virtual void draw_renderpass(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target) override;

	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;

	/**
	 * @brief Helper function to determine the constant data method that is selected and supported by the sample
	 * @returns The method that is selected, otherwise push constants
	 */
	inline Method get_active_method();

	vkb::sg::PerspectiveCamera *camera{};

	// The render pipeline designed for using push constants
	std::unique_ptr<vkb::RenderPipeline> push_constant_render_pipeline{nullptr};

	// The render pipeline designed for using Descriptor Sets, Dynamic Descriptor Sets and Update-after-bind Descriptor Sets
	std::unique_ptr<vkb::RenderPipeline> descriptor_set_render_pipeline{nullptr};

	// The render pipeline designed for using a large shader storage buffer object that is instanced into to get the relevant MVP data
	std::unique_ptr<vkb::RenderPipeline> buffer_array_render_pipeline{nullptr};

	// The samples constant data methods and their properties
	std::unordered_map<Method, MethodProperties> methods = {
	    {Method::PushConstants, {"Push Constants"}},
	    {Method::DescriptorSets, {"Descriptor Sets"}},
	    {Method::DynamicDescriptorSets, {"Dynamic Descriptor Sets"}},
	    {Method::UpdateAfterBindDescriptorSets, {"Update-after-bind Descriptor Sets", false}},
	    {Method::BufferArray, {"Single Pre-allocated Buffer Array"}}};

	int gui_method_value{static_cast<int>(Method::PushConstants)};

	int last_gui_method_value{static_cast<int>(Method::PushConstants)};
};

std::unique_ptr<vkb::VulkanSample<vkb::BindingType::C>> create_constant_data();
