/* Copyright (c) 2023, Holochip Corporation
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
 * Demonstrate and showcase a sample application using mesh shader rendering pipeline.
 */

#include "mesh_shader.h"
#include "benchmark_mode/benchmark_mode.h"

// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void *aligned_alloc(size_t size, size_t alignment)
{
	void *data;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void aligned_free(void *data)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else
	free(data);
#endif
}

MeshShader::MeshShader()
{
	title = "Mesh shader";

	// API version
	set_api_version(VK_API_VERSION_1_3);

	// Add instance and device extensions
	add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME);

	vkb::GLSLCompiler::set_target_environment(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);

	// Initialize the cube
	init_cube();
	// Initialize the cube meshlet information
	init_cube_meshlets();
}

MeshShader ::~MeshShader()
{
	if (device)
	{
		if (ubo_data_dynamic.model)
		{
			aligned_free(ubo_data_dynamic.model);
		}

		vkDestroyPipeline(get_device().get_handle(), pipeline, nullptr);
		vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layout, nullptr);
		vkDestroyDescriptorSetLayout(get_device().get_handle(), descriptor_set_layout, nullptr);
	}
}

void MeshShader::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color        = default_clear_color;
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.offset.x      = 0;
	render_pass_begin_info.renderArea.offset.y      = 0;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = static_cast<uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues             = clear_values.data();

	for (int32_t i = 0; i < static_cast<int32_t>(draw_cmd_buffers.size()); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		if (is_mesh_shader)
		{
			// TODO @Jeremy: we may need a total descriptor set count as a CLASS VARIABLE:

			// The first set = 0, descriptor set count = 4... this means uniform buffer
			vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 4, &descriptor_set, 0, nullptr);

			vkCmdDrawMeshTasksEXT(draw_cmd_buffers[i], 6, 1, 1);        // That 2 draws on all directions each
		}
		else
		{
			VkDeviceSize offsets[1] = {0}; // Just to ensure that offsets gets out of the scope
			// Binding Vertex and Index buffers
			vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
			vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

			// Render multiple objects using different model matrices by dynamically offsetting into one uniform buffer
			for (uint32_t j = 0; j < OBJECT_INSTANCES; j++)
			{
				// One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
				uint32_t dynamic_offset = j * static_cast<uint32_t>(dynamic_alignment);
				// Bind the descriptor set for rendering a mesh using the dynamic offset

				// The first set = 0, descriptor set count = 1... this means uniform buffer
				// Dynamic offset is 1, and attached to the pointer to the dynamic offset data

				vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 1, &dynamic_offset);

				vkCmdDrawIndexed(draw_cmd_buffers[i], index_count, 1, 0, 0, 0);
			}
		}

		draw_ui(draw_cmd_buffers[i]);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}


}

void MeshShader::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void MeshShader::init_cube()
{
	// Setup vertices indices for a colored cube
	cube_vertices =
	    {
	        {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},         // vertex 0
	        {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},          // vertex 1
	        {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},           // vertex 2
	        {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}},          // vertex 3
	        {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},        // vertex 4
	        {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},         // vertex 5
	        {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},          // vertex 6
	        {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}}          // vertex 7
	    };

	// A simple reference for the vertices and associated corresponding indices

	//                 7----------------6
	//                /|               /|
	//               / |              / |
	//              /  |             /  |
	//             /   |            /   |
	//            3----------------2    |
	//            |    4-----------|----5
	//            |   /            |   /
	//            |  /             |  /
	//            | /              | /
	//            |/               |/
	//            0----------------1

	cube_indices =
	    {
	        0, 1, 2,        // front button
	        2, 3, 0,        // front top
	        1, 5, 6,        // right button
	        6, 2, 1,        // right top
	        7, 6, 5,        // rear top
	        5, 4, 7,        // rear button
	        4, 0, 3,        // left button
	        3, 7, 4,        // left top
	        4, 5, 1,        // button right
	        1, 0, 4,        // button left
	        3, 2, 6,        // top right
	        6, 7, 3         // top left
	    };
}

void MeshShader::init_cube_meshlets()
{
	// Vertex and Indices Decomposition:

	// In order to minimize the duplicated vertex indices, one could choose the following pass to de-composite the cube:

	//                 7------------>---6
	//                /|               /|
	//               / |              / |
	//              ^  |             /  |
	//             /   |            /   |
	//            3--<-------------2    |
	//            |    4---<-------|----5
	//            |   /            ^   /
	//            |  /             |  /
	//            | /              | /
	//            |/               |/
	//    (start) 0----------->----1

	//  front surface: start from 0 -> 1 ->2 ->3;  vertices covered: {0, 1, 2, 3}; no duplications
	//    top surface: start from 3 -> 7 ->6;      vertices covered: {2, 3, 7, 6}; no duplications
	//   rear surface: start from 6 -> 5 ->4;      vertices covered: {7, 6, 5, 4}; no duplications
	// button surface: start from 4 -> 0 ->1;      vertices covered: {5, 4, 0, 1}; vertices duplicated: {0, 1}
	//  right surface: start from 1 -> 5 ->6 -> 2; vertices covered: {1, 5, 6, 2}; vertices duplicated: {5, 6, 2}
	//   left surface: start from 3 -> 0 ->4 -> 7; vertices covered: {3, 0, 4, 7}; vertices duplicated: {3, 0, 4, 7}

	// Hence, meshlet_vertex_indices can be defined as follows:
	meshlet_vertex_indices =
	    {
	        0, 1,             // working group 1 only
	        2, 3,             // shared by working group 1 and 2
	        7, 6,             // shared by working group 2 and 3
	        5, 4,             // shared by working group 3 and 4
	        0,                // working group 4 only
	        1,                // shared by working group 4 and 5
	        5, 6, 2,          // working group 5 only
	        3, 0, 4, 7        // working group 6 only
	    };

	// Working group 1: front surface

	//              vertex indices		         local index
	//				{0, 1, 2, 3}                 {0, 1, 2, 3}
	//            7----------------6          3----------------2
	//            |              . |          |              . |
	//            |   t2      .    |          |   t2      .    |
	//            |        .       |          |        .       |
	//            |     .    t1    |          |     .    t1    |
	//            |  .             |          |  .             |
	//            0----------------1          0----------------1

	//                              FRONT SURFACE
	//                            (normal arrows out)

	// cube vertex indices {0, 1, 2, 3}
	// local primitive indices making the surface from triangle t1 to triangle t2: {0,1,2, 2,3,0}

	// Working group 2: top surface

	//              vertex indices		         local index
	//				{2, 3, 7, 6}                 {0, 1, 2, 3}
	//            7----------------6          2----------------3
	//            |              . |          |              . |
	//            |   t2      .    |          |   t2      .    |
	//            |        .       |          |        .       |
	//            |     .    t1    |          |     .    t1    |
	//            |  .             |          |  .             |
	//            3----------------2          1----------------0

	//                              TOP SURFACE
	//                           (normal arrows out)

	// cube vertex indices {2, 3, 7, 6}
	// local primitive indices making the surface from triangle t1 to triangle t2: {1,0,3, 3,2,1}

	// Working group 3: rear surface

	//              vertex indices		         local index
	//				{7, 6, 5, 4}                 {0, 1, 2, 3}
	//            6----------------7          1----------------0
	//            |              . |          |              . |
	//            |   t2      .    |          |   t2      .    |
	//            |        .       |          |        .       |
	//            |     .    t1    |          |     .    t1    |
	//            |  .             |          |  .             |
	//            5----------------4          2----------------3

	//                             REAR SURFACE
	//                          (normal arrows out)

	// cube vertex indices {7, 6, 5, 4}
	// local primitive indices making the surface from triangle t1 to triangle t2: {2,3,0, 0,1,2}

	// Working group 4: button surface

	//              vertex indices		         local index
	//				{5, 4, 0, 1}                 {0, 1, 2, 3}
	//            5----------------4          0----------------1
	//            |              . |          |              . |
	//            |   t2      .    |          |   t2      .    |
	//            |        .       |          |        .       |
	//            |     .    t1    |          |     .    t1    |
	//            |  .             |          |  .             |
	//            1----------------0          3----------------2

	//                            BUTTON SURFACE
	//                          (normal arrows out)

	// cube vertex indices {5, 4, 0, 1}
	// local primitive indices making the surface from triangle t1 to triangle t2: {3,2,1, 1,0,3}

	// Working group 5: right surface

	//              vertex indices		         local index
	//				{1, 5, 6, 2}                 {0, 1, 2, 3}
	//            2----------------6          3----------------2
	//            |              . |          |              . |
	//            |   t2      .    |          |   t2      .    |
	//            |        .       |          |        .       |
	//            |     .    t1    |          |     .    t1    |
	//            |  .             |          |  .             |
	//            1----------------5          0----------------1

	//                            RIGHT SURFACE
	//                          (normal arrows out)

	// cube vertex indices {1, 5, 6, 2}
	// local primitive indices making the surface from triangle t1 to triangle t2: {0,1,2, 2,3,0}

	// Working group 6: left surface

	//              vertex indices		         local index
	//				{3, 0, 4, 7}                 {0, 1, 2, 3}
	//            7----------------3          3----------------0
	//            |              . |          |              . |
	//            |   t2      .    |          |   t2      .    |
	//            |        .       |          |        .       |
	//            |     .    t1    |          |     .    t1    |
	//            |  .             |          |  .             |
	//            4----------------0          2----------------1

	//                             LEFT SURFACE
	//                          (normal arrows out)

	// cube vertex indices {3, 0, 4, 7}
	// local primitive indices making the surface from triangle t1 to triangle t2: {2,1,0, 0,3,2}

	// Hence, meshlet_primitive_indices can be defined as follows:
	meshlet_primitive_indices =
	    {
	        0, 1, 2, 2, 3, 0,        // working group 1
	        1, 0, 3, 3, 2, 1,        // working group 2
	        2, 3, 0, 0, 1, 2,        // working group 3
	        3, 2, 1, 1, 0, 3,        // working group 4
	        0, 1, 2, 2, 3, 0,        // working group 5
	        2, 1, 0, 0, 3, 2         // working group 6
	    };

	// By referring to the following list, one shall have easily figured out meshlet_info for each working groups
	/*
	meshlet_vertex_indices =
	    {
	        0, 1,             // working group 1 only
	        2, 3,             // shared by working group 1 and 2
	        7, 6,             // shared by working group 2 and 3
	        5, 4,             // shared by working group 3 and 4
	        0,                // working group 4 only
	        1,                // shared by working group 4 and 5
	        5, 6, 2,          // working group 5 only
	        3, 0, 4, 7        // working group 6 only
	    };
	meshlet_primitive_indices =
	    {
	        0, 1, 2, 2, 3, 0,        // working group 1
	        1, 0, 3, 3, 2, 1,        // working group 2
	        2, 3, 0, 0, 1, 2,        // working group 3
	        3, 2, 1, 1, 0, 3,        // working group 4
	        0, 1, 2, 2, 3, 0,        // working group 5
	        2, 1, 0, 0, 3, 2         // working group 6
	    };
	*/
	// vertex_count for all working groups: 4
	// primitive_count for all working groups: 4
	// primitive_begin_index for each working group: 6 * working group index

	// Hence, meshlet_info can be further initialized as follows:
	MeshletInfo meshlet_info{};

	meshlet_info.vertex_count    = 4;
	meshlet_info.primitive_count = 4;

	// Working group 1:
	meshlet_info.vertex_begin_index    = 0;
	meshlet_info.primitive_begin_index = 0;
	meshlet_infos.push_back(meshlet_info);
	// Working group 2:
	meshlet_info.vertex_begin_index = 2;
	meshlet_info.primitive_begin_index += 6;
	meshlet_infos.push_back(meshlet_info);
	// Working group 3:
	meshlet_info.vertex_begin_index = 4;
	meshlet_info.primitive_begin_index += 6;
	meshlet_infos.push_back(meshlet_info);
	// Working group 4:
	meshlet_info.vertex_begin_index = 6;
	meshlet_info.primitive_begin_index += 6;
	meshlet_infos.push_back(meshlet_info);
	// Working group 5:
	meshlet_info.vertex_begin_index = 9;
	meshlet_info.primitive_begin_index += 6;
	meshlet_infos.push_back(meshlet_info);
	// Working group 6:
	meshlet_info.vertex_begin_index = 13;
	meshlet_info.primitive_begin_index += 6;
	meshlet_infos.push_back(meshlet_info);
}

void MeshShader::prepare_buffers()
{
	if (is_mesh_shader)
	{
		auto meshlet_vertex_array_buffer_size    = cube_vertices.size() * sizeof(Vertex);        // total number of vertices multiplies by the data size of a Vertex structure
		auto meshlet_vertex_index_buffer_size    = meshlet_vertex_indices.size() * sizeof(uint32_t);
		auto meshlet_info_object_buffer_size     = meshlet_infos.size() * sizeof(MeshletInfo);        // total number of meshlet info multiplies by the data size of a MeshletInfo structure
		auto meshlet_primitive_index_buffer_size = meshlet_primitive_indices.size() * sizeof(uint8_t);

		// Updates: meshlet vertex array buffer
		meshlet_vertex_array_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
		                                                                  meshlet_vertex_array_buffer_size,
		                                                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,        // flag: storage buffer units
		                                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);              // memory flag: from CPU stores to GPU
		meshlet_vertex_array_buffer->update(cube_vertices.data(), meshlet_vertex_index_buffer_size);                 // data source: cube_vertices vector

		// Update: meshlet vertex index buffer
		meshlet_vertex_index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
		                                                                  meshlet_vertex_index_buffer_size,
		                                                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,        // flag: storage buffer units
		                                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);              // memory flag: from CPU stores to GPU
		meshlet_vertex_index_buffer->update(meshlet_vertex_indices.data(), meshlet_vertex_index_buffer_size);        // data source: meshlet_vertex_indices vector

		// Update: meshlet info buffer
		meshlet_info_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
		                                                          meshlet_info_object_buffer_size,
		                                                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,        // flag: storage buffer units
		                                                          VMA_MEMORY_USAGE_CPU_TO_GPU);              // memory flag: from CPU stores to GPU
		meshlet_info_buffer->update(meshlet_infos.data(), meshlet_info_object_buffer_size);                  // data source: meshlet_infos vector

		// Updates: meshlet primitive index buffer
		meshlet_primitive_index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
		                                                                     meshlet_primitive_index_buffer_size,
		                                                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,              // flag: storage buffer units
		                                                                     VMA_MEMORY_USAGE_CPU_TO_GPU);                    // memory flag: from CPU stores to GPU
		meshlet_primitive_index_buffer->update(meshlet_primitive_indices.data(), meshlet_primitive_index_buffer_size);        // data source: meshlet_primitive_indices vector
	}
	else
	{
		auto vertex_buffer_size = cube_vertices.size() * sizeof(Vertex);
		auto index_buffer_size  = cube_indices.size() * sizeof(uint32_t);

		// Vertex buffer
		vertex_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
		                                                    vertex_buffer_size,
		                                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		                                                    VMA_MEMORY_USAGE_GPU_TO_CPU);
		vertex_buffer->update(cube_vertices.data(), vertex_buffer_size);

		// Index buffer
		index_buffer = std::make_unique<vkb::core::Buffer>(get_device(),
		                                                   index_buffer_size,
		                                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		                                                   VMA_MEMORY_USAGE_GPU_TO_CPU);
		index_buffer->update(cube_indices.data(), index_buffer_size);

		index_count = static_cast<uint32_t>(cube_indices.size());
	}
}

void MeshShader::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes{};

	// TODO @Jeremy: check this out, it might be good to set it as a class variable
	uint32_t descriptor_max_sets = 2;

	if (is_mesh_shader)
	{
		// Storage buffer needed: 4
		pool_sizes.push_back(vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4));
		// Therefore, 4 descriptor sets needed
		descriptor_max_sets = 4;
	}
	else
	{
		// Uniform buffer needed: 1
		pool_sizes.push_back(vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
		// Dynamic uniform buffer needed: 1
		pool_sizes.push_back(vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1));
		// Therefore, 4 descriptor sets needed
		// descriptor_max_sets = 2;        // this line is never needed, but its there as a reminder
	}

	// There always needed this one same image sampler
	pool_sizes.push_back(vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));

	// Create descriptor pool
	VkDescriptorPoolCreateInfo descriptor_pool_create_info =
	    vkb::initializers::descriptor_pool_create_info(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), descriptor_max_sets);

	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void MeshShader::setup_descriptor_set_layout()
{
	// Specifies the targeted shaders, and defines layer bindings for the specified shaders
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings{};

	if (is_mesh_shader)
	{
		// Task shader binding 0: vertex array buffer
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT, 0));
		// Task shader binding 1: vertex index buffer
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT, 1));
		// Task shader binding 2: meshlet information buffer
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT, 2));
		// Task shader binding 3: primitive index buffer
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT, 3));
		// Fragment shader binding 4: just a traditional binding for color output
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4));
	}
	else
	{
		// Vertex shader binding 0: (static/traditional) uniform buffer
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0));
		// Vertex shader binding 1: dynamic uniform buffer
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1));
		// Fragment shader binding 2: just a traditional binding for color output
		set_layout_bindings.push_back(vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2));
	}

	// Creates descriptor set layout
	VkDescriptorSetLayoutCreateInfo descriptor_layout =
	    vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));        // writes to descriptor_set_layout

	// Creates pipeline layout based on the descriptor set layout information
	VkPipelineLayoutCreateInfo pipeline_layout_create_info =
	    vkb::initializers::pipeline_layout_create_info(&descriptor_set_layout, 1);        // e.g., layout = 0,  binding = 1, 2, 3, ...

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));        // writes to pipeline_layout
}

void MeshShader::setup_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info =
	    vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_set));

	std::vector<VkWriteDescriptorSet> write_descriptor_sets{};

	if (is_mesh_shader)
	{
		// Create buffer descriptors:
		VkDescriptorBufferInfo meshlet_vertex_array_buffer_descriptor    = create_descriptor(*meshlet_vertex_array_buffer);           // vertex array buffer
		VkDescriptorBufferInfo meshlet_vertex_index_buffer_descriptor    = create_descriptor(*meshlet_vertex_index_buffer);           // vertex index buffer
		VkDescriptorBufferInfo meshlet_info_buffer_descriptor            = create_descriptor(*meshlet_info_buffer);                   // meshlet information buffer
		VkDescriptorBufferInfo meshlet_primitive_index_buffer_descriptor = create_descriptor(*meshlet_primitive_index_buffer);        // primitive index buffer

		// Pushes back to the write_descriptor_sets vector:
		write_descriptor_sets.push_back(vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &meshlet_vertex_array_buffer_descriptor));           // binding 0
		write_descriptor_sets.push_back(vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &meshlet_vertex_index_buffer_descriptor));           // binding 1
		write_descriptor_sets.push_back(vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, &meshlet_info_buffer_descriptor));                   // binding 2
		write_descriptor_sets.push_back(vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &meshlet_primitive_index_buffer_descriptor));        // binding 3
	}
	else
	{
		// Create buffer descriptors:
		VkDescriptorBufferInfo view_buffer_descriptor    = create_descriptor(*uniform_buffers.view);                              // uniform buffer
		VkDescriptorBufferInfo dynamic_buffer_descriptor = create_descriptor(*uniform_buffers.dynamic, dynamic_alignment);        // dynamic uniform buffer: passes the  actual dynamic alignment as the descriptor's size

		// Pushes back to the write_descriptor_sets vector:
		write_descriptor_sets.push_back(vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &view_buffer_descriptor));                   // binding: 0
		write_descriptor_sets.push_back(vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, &dynamic_buffer_descriptor));        // binding: 1
	}

	vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
}

void MeshShader::prepare_pipelines()
{
	//TODO: check
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
	    vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterization_state =
	    vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

	// TODO: check
	if (is_mesh_shader)
	{
		rasterization_state =
		    vkb::initializers::pipeline_rasterization_state_create_info({}, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	}

	VkPipelineColorBlendAttachmentState blend_attachment_state =
	    vkb::initializers::pipeline_color_blend_attachment_state(0xf, VK_FALSE);

	// TODO: check if its okay
	if (is_mesh_shader)
	{
		blend_attachment_state.colorWriteMask =
		    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo color_blend_state =
	    vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

	// Note: Using Reversed depth-buffer for increased precision, so Greater depth values are kept
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state =
	    vkb::initializers::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER);

	VkPipelineViewportStateCreateInfo viewport_state =
	    vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	// TODO: check
	VkPipelineMultisampleStateCreateInfo multisample_state =
	    vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);

	std::vector<VkDynamicState> dynamic_state_enables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamic_state =
	    vkb::initializers::pipeline_dynamic_state_create_info(dynamic_state_enables.data(), static_cast<uint32_t>(dynamic_state_enables.size()), 0);

	// Load shaders
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};        // create an empty vector for staging shaders

	if (is_mesh_shader)
	{
		shader_stages.push_back(load_shader("mesh_shader/mesh_shader_task_mesh.task", VK_SHADER_STAGE_TASK_BIT_EXT));
		shader_stages.push_back(load_shader("mesh_shader/mesh_shader_task_mesh.mesh", VK_SHADER_STAGE_MESH_BIT_EXT));
		shader_stages.push_back(load_shader("mesh_shader/mesh_shader_task_mesh.frag", VK_SHADER_STAGE_FRAGMENT_BIT));
	}
	else
	{
		shader_stages.push_back(load_shader("mesh_shader/mesh_shader_traditional.vert", VK_SHADER_STAGE_VERTEX_BIT));
		shader_stages.push_back(load_shader("mesh_shader/mesh_shader_traditional.frag", VK_SHADER_STAGE_FRAGMENT_BIT));
	}

	// Generate the graphic pipeline
	VkGraphicsPipelineCreateInfo pipeline_create_info =
	    vkb::initializers::pipeline_create_info(pipeline_layout, render_pass, 0);

	if (!is_mesh_shader)
	{
		// Vertex input to pipeline
		VkPipelineVertexInputStateCreateInfo vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();

		// Vertex bindings and attributes
		const std::vector<VkVertexInputBindingDescription> vertex_input_bindings =
		    {
		        vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
		    };
		const std::vector<VkVertexInputAttributeDescription> vertex_input_attributes =
		    {
		        vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),          // Location 0 : Position
		        vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)),        // Location 1 : Color
		    };

		vertex_input_state.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertex_input_bindings.size());
		vertex_input_state.pVertexBindingDescriptions      = vertex_input_bindings.data();
		vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
		vertex_input_state.pVertexAttributeDescriptions    = vertex_input_attributes.data();

		pipeline_create_info.pVertexInputState = &vertex_input_state;
	}

	pipeline_create_info.pInputAssemblyState = &input_assembly_state;
	pipeline_create_info.pRasterizationState = &rasterization_state;
	pipeline_create_info.pColorBlendState    = &color_blend_state;
	pipeline_create_info.pMultisampleState   = &multisample_state;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_state;

	// TODO: check
	pipeline_create_info.pDynamicState       = &dynamic_state;

	pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_create_info.pStages    = shader_stages.data();

	//TODO: ? not sure if VkPipelineRenderingCreateInfo is needed: however I do have the render pass...


	VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipeline_create_info, nullptr, &pipeline));
}

void MeshShader::prepare_uniform_buffers()
{
	// Allocate data for the dynamic uniform buffer object
	// We allocate this manually as the alignment of the offset differs between GPUs

	// Calculate required alignment based on minimum device offset alignment
	auto min_ubo_alignment = static_cast<size_t>(get_device().get_gpu().get_properties().limits.minUniformBufferOffsetAlignment);
	dynamic_alignment      = sizeof(glm::mat4);
	if (min_ubo_alignment > 0)
	{
		dynamic_alignment = (dynamic_alignment + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
	}

	size_t buffer_size = OBJECT_INSTANCES * dynamic_alignment;

	ubo_data_dynamic.model = (glm::mat4 *) aligned_alloc(buffer_size, dynamic_alignment);
	assert(ubo_data_dynamic.model);

	std::cout << "minUniformBufferOffsetAlignment = " << min_ubo_alignment << std::endl;
	std::cout << "dynamicAlignment = " << dynamic_alignment << std::endl;

	// Vertex shader uniform buffer block

	// Static shared uniform buffer object with projection and view matrix
	uniform_buffers.view =
	    std::make_unique<vkb::core::Buffer>(get_device(), sizeof(ubo_vs), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	uniform_buffers.dynamic =
	    std::make_unique<vkb::core::Buffer>(get_device(), buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Prepare per-object matrices with offsets and random rotations
	std::default_random_engine      rnd_engine(platform->using_plugin<::plugins::BenchmarkMode>() ? 0 : (unsigned) time(nullptr));
	std::normal_distribution<float> rnd_dist(-1.0f, 1.0f);
	for (uint32_t i = 0; i < OBJECT_INSTANCES; i++)
	{
		rotations[i]       = glm::vec3(rnd_dist(rnd_engine), rnd_dist(rnd_engine), rnd_dist(rnd_engine)) * 2.0f * glm::pi<float>();
		rotation_speeds[i] = glm::vec3(rnd_dist(rnd_engine), rnd_dist(rnd_engine), rnd_dist(rnd_engine));
	}

	update_uniform_buffers();
	update_dynamic_uniform_buffer(0.0f, true);
}

void MeshShader::update_uniform_buffers()
{
	// Fixed ubo with projection and view matrices
	ubo_vs.projection = camera.matrices.perspective;
	ubo_vs.view       = camera.matrices.view;

	uniform_buffers.view->convert_and_update(ubo_vs);
}

void MeshShader::update_dynamic_uniform_buffer(float delta_time, bool force)
{
	// Update at max. 60 fps
	animation_timer += delta_time;

	if ((animation_timer + 0.0025 < (1.0f / 60.0f)) && (!force))
	{
		return;
	}

	// Dynamic ubo with per-object model matrices indexed by offsets in the command buffer
	auto      dim       = static_cast<uint32_t>(pow(OBJECT_INSTANCES, (1.0f / 3.0f)));
	auto      float_dim = static_cast<float>(dim);
	glm::vec3 offset(5.0f);

	for (uint32_t x = 0; x < dim; x++)
	{
		auto fx = static_cast<float>(x);
		for (uint32_t y = 0; y < dim; y++)
		{
			auto fy = static_cast<float>(y);
			for (uint32_t z = 0; z < dim; z++)
			{
				auto fz    = static_cast<float>(z);
				auto index = x * dim * dim + y * dim + z;

				// Aligned offset
				auto model_mat = (glm::mat4 *) (((uint64_t) ubo_data_dynamic.model + (index * dynamic_alignment)));

				// Update rotations
				rotations[index] += animation_timer * rotation_speeds[index];

				// Update matrices
				glm::vec3 pos(-((float_dim * offset.x) / 2.0f) + offset.x / 2.0f + fx * offset.x,
				              -((float_dim * offset.y) / 2.0f) + offset.y / 2.0f + fy * offset.y,
				              -((float_dim * offset.z) / 2.0f) + offset.z / 2.0f + fz * offset.z);

				*model_mat = glm::translate(glm::mat4(1.0f), pos);
				*model_mat = glm::rotate(*model_mat, rotations[index].x, glm::vec3(1.0f, 1.0f, 0.0f));
				*model_mat = glm::rotate(*model_mat, rotations[index].y, glm::vec3(0.0f, 1.0f, 0.0f));
				*model_mat = glm::rotate(*model_mat, rotations[index].z, glm::vec3(0.0f, 0.0f, 1.0f));
			}
		}
	}

	animation_timer = 0.0f;

	uniform_buffers.dynamic->update(ubo_data_dynamic.model, static_cast<size_t>(uniform_buffers.dynamic->get_size()));

	// Flush to make changes visible to the device
	uniform_buffers.dynamic->flush();
}

bool MeshShader::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -30.0f));
	camera.set_rotation(glm::vec3(0.0f));
	// Note: Using reversed depth-buffer for increased precision, so Z-near and Z-far are flipped
	camera.set_perspective(60.0f, (float) width / (float) height, 256.0f, 0.1f);

	prepare_buffers();
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	prepare_pipelines();
	setup_descriptor_pool();
	setup_descriptor_set();
	build_command_buffers();
	prepared = true;
	return true;
}

bool MeshShader::resize(uint32_t width, uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_uniform_buffers();
	return true;
}

void MeshShader::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (!paused)
	{
		update_dynamic_uniform_buffer(delta_time);
	}
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

std::unique_ptr<vkb::Application> create_mesh_shader()
{
	return std::make_unique<MeshShader>();
}
