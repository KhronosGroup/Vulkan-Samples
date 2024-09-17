/* Copyright (c) 2024, Arm Limited and Contributors
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

#include "hack_base.h"

#include "benchmark_mode/benchmark_mode.h"


 // Wrapper functions for aligned memory allocation
 // There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void* hack_base::aligned_alloc(size_t size, size_t alignment)
{
	void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
	{
		data = nullptr;
	}
#endif
	return data;
}

void hack_base::aligned_free(void* data)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else
	free(data);
#endif
}

hack_base::hack_base()
{
	// Pipeline defaults
	input_assembly_state =
		vkb::initializers::pipeline_input_assembly_state_create_info(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			0,
			VK_FALSE);

	rasterization_state =
		vkb::initializers::pipeline_rasterization_state_create_info(
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_NONE,
			VK_FRONT_FACE_COUNTER_CLOCKWISE,
			0);

	blend_attachment_state =
		vkb::initializers::pipeline_color_blend_attachment_state(
			0xf,
			VK_FALSE);

	color_blend_state =
		vkb::initializers::pipeline_color_blend_state_create_info(
			1,
			&blend_attachment_state);

	// Note: Using reversed depth-buffer for increased precision, so Greater depth values are kept
	depth_stencil_state =
		vkb::initializers::pipeline_depth_stencil_state_create_info(
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_GREATER);

	viewport_state =
		vkb::initializers::pipeline_viewport_state_create_info(1, 1, 0);

	multisample_state =
		vkb::initializers::pipeline_multisample_state_create_info(
			VK_SAMPLE_COUNT_1_BIT,
			0);

	dynamic_state_enables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR };
	dynamic_state =
		vkb::initializers::pipeline_dynamic_state_create_info(
			dynamic_state_enables.data(),
			static_cast<uint32_t>(dynamic_state_enables.size()),
			0);

	// Vertex bindings and attributes
	vertex_input_bindings = {
			vkb::initializers::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	vertex_input_attributes = {
			vkb::initializers::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),          // Location 0 : Position
			vkb::initializers::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)),        // Location 1 : Color
	};
	vertex_input_state = vkb::initializers::pipeline_vertex_input_state_create_info();
	vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_input_bindings.size());
	vertex_input_state.pVertexBindingDescriptions = vertex_input_bindings.data();
	vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
	vertex_input_state.pVertexAttributeDescriptions = vertex_input_attributes.data();
}

hack_base::~hack_base()
{
	if (aligned_cubes)
	{
		aligned_free(aligned_cubes);
	}
}


void hack_base::generate_cube()
{
	// Setup vertices indices for a colored cube
	std::vector<Vertex> vertices = {
			{{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
			{{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
			{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
			{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}},
			{{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
			{{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
			{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
			{{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}},
	};

	std::vector<uint32_t> indices = {
			0,
			1,
			2,
			2,
			3,
			0,
			1,
			5,
			6,
			6,
			2,
			1,
			7,
			6,
			5,
			5,
			4,
			7,
			4,
			0,
			3,
			3,
			7,
			4,
			4,
			5,
			1,
			1,
			0,
			4,
			3,
			2,
			6,
			6,
			7,
			3,
	};

	index_count = static_cast<uint32_t>(indices.size());

	auto vertex_buffer_size = vertices.size() * sizeof(Vertex);
	auto index_buffer_size = indices.size() * sizeof(uint32_t);

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	vertex_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
		vertex_buffer_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	vertex_buffer->update(vertices.data(), vertex_buffer_size);

	index_buffer = std::make_unique<vkb::core::BufferC>(get_device(),
		index_buffer_size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	index_buffer->update(indices.data(), index_buffer_size);
}

void hack_base::generate_rotations()
{
	// Prepare per-object matrices with offsets and random rotations
	std::default_random_engine      rnd_engine(lock_simulation_speed ? 0 : static_cast<unsigned>(time(nullptr)));
	std::normal_distribution<float> rnd_dist(-1.0f, 1.0f);
	for (uint32_t i = 0; i < OBJECT_INSTANCES; i++)
	{
		rotations[i] = glm::vec3(rnd_dist(rnd_engine), rnd_dist(rnd_engine), rnd_dist(rnd_engine)) * 2.0f * glm::pi<float>();
		rotation_speeds[i] = glm::vec3(rnd_dist(rnd_engine), rnd_dist(rnd_engine), rnd_dist(rnd_engine));
	}
}

void hack_base::update_rotation(float delta_time)
{
	// Fixed timestep for profiling purposes.
	animation_timer = 1.0f / 60;

	// Dynamic ubo with per-object model matrices indexed by offsets in the command buffer
	auto      dim = static_cast<uint32_t>(pow(OBJECT_INSTANCES, (1.0f / 3.0f)));
	auto      fdim = static_cast<float>(dim);
	glm::vec3 offset(5.0f);

	for (uint32_t x = 0; x < dim; x++)
	{
		auto fx = static_cast<float>(x);
		for (uint32_t y = 0; y < dim; y++)
		{
			auto fy = static_cast<float>(y);
			for (uint32_t z = 0; z < dim; z++)
			{
				auto fz = static_cast<float>(z);
				auto index = x * dim * dim + y * dim + z;


				// Update rotations
				rotations[index] += animation_timer * rotation_speeds[index];

				// Update matrices
				glm::vec3 pos(-((fdim * offset.x) / 2.0f) + offset.x / 2.0f + fx * offset.x,
					-((fdim * offset.y) / 2.0f) + offset.y / 2.0f + fy * offset.y,
					-((fdim * offset.z) / 2.0f) + offset.z / 2.0f + fz * offset.z);

				glm::mat4 *model_mat = (glm::mat4 *) (((size_t) aligned_cubes + (index * alignment)));
				*model_mat = glm::translate(glm::mat4(1.0f), pos);
				*model_mat = glm::rotate(*model_mat, rotations[index].x, glm::vec3(1.0f, 1.0f, 0.0f));
				*model_mat = glm::rotate(*model_mat, rotations[index].y, glm::vec3(0.0f, 1.0f, 0.0f));
				*model_mat = glm::rotate(*model_mat, rotations[index].z, glm::vec3(0.0f, 0.0f, 1.0f));
			}
		}
	}
}

void hack_base::draw()
{
	ApiVulkanSample::prepare_frame();

	// Command buffer to be submitted to the queue
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &draw_cmd_buffers[current_buffer];

	// Submit to queue
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	ApiVulkanSample::submit_frame();
}

void hack_base::prepare_view_uniform_buffer()
{
	// Static shared uniform buffer object with projection and view matrix
	view_uniform_buffer.view = std::make_unique<vkb::core::BufferC>(get_device(),
		sizeof(ubo_vs),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);

	update_view_uniform_buffer();
}

void hack_base::update_view_uniform_buffer()
{
	// Fixed ubo with projection and view matrices
	ubo_vs.projection = camera.matrices.perspective;
	ubo_vs.view = camera.matrices.view;

	view_uniform_buffer.view->convert_and_update(ubo_vs);
}

bool hack_base::prepare(const vkb::ApplicationOptions& options)
{
	if (!ApiVulkanSample::prepare(options))
	{
		return false;
	}

	camera.type = vkb::CameraType::LookAt;
	camera.set_position(glm::vec3(0.0f, 0.0f, -30.0f));
	camera.set_rotation(glm::vec3(0.0f));

	// Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 256.0f, 0.1f);

	generate_cube();
	generate_rotations();
	prepare_view_uniform_buffer();

	hack_prepare();

	prepared = true;
	return true;
}

void hack_base::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (!paused)
	{
		update_rotation(delta_time);
		hack_render(delta_time);
	}
	if (camera.updated)
	{
		update_view_uniform_buffer();
	}
}

bool hack_base::resize(const uint32_t width, const uint32_t height)
{
	ApiVulkanSample::resize(width, height);
	update_view_uniform_buffer();
	return true;
}

void hack_base::prepare_aligned_cubes(size_t alignment, size_t *out_buffer_size)
{
	// In case we change the alignment
	if (aligned_cubes)
	{
		aligned_free(aligned_cubes);
	}

	this->alignment = alignment;
	size_t buffer_size = OBJECT_INSTANCES * alignment;

	aligned_cubes = static_cast<glm::mat4 *>(aligned_alloc(buffer_size, alignment));
	assert(aligned_cubes);

	if (out_buffer_size)
	{
		*out_buffer_size = buffer_size;
	}
}

///
std::unique_ptr<vkb::VulkanSampleC> create_hack_base()
{
	return std::make_unique<hack_base>();
}
