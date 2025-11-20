/* Copyright (c) 2019-2025, Arm Limited and Contributors
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

#pragma once

#include "buffer_pool.h"
#include "common/utils.h"
#include "rendering/render_pipeline.h"
#include "rendering/subpasses/forward_subpass.h"
#include "scene_graph/components/material.h"
#include "scene_graph/components/mesh.h"
#include "scene_graph/components/perspective_camera.h"
#include "vulkan_sample.h"

class ThreadPool
{
  public:
	explicit ThreadPool() :
	    stop_flag(false)
	{}

	~ThreadPool()
	{
		shutdown();
	}

	template <class F, class... Args>
	auto push(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args..., size_t>>
	{
		using return_type                 = std::invoke_result_t<F, Args..., size_t>;
		auto                     task_ptr = std::make_shared<std::packaged_task<return_type(size_t)>>(std::bind(std::forward<F>(f), std::forward<Args>(args)..., std::placeholders::_1));
		std::future<return_type> res      = task_ptr->get_future();
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			tasks.emplace([task_ptr](size_t thread_index) { (*task_ptr)(thread_index); });
		}
		condition.notify_one();
		return res;
	}

	void resize(size_t thread_count)
	{
		if (thread_count != workers.size())
		{
			shutdown();

			for (size_t i = 0; i < thread_count; ++i)
			{
				workers.emplace_back([this, i] {
					size_t thread_index = i;
					while (true)
					{
						std::function<void(size_t)> task;
						{
							std::unique_lock<std::mutex> lock(queue_mutex);
							condition.wait(lock, [this] { return stop_flag || !tasks.empty(); });
							if (stop_flag && tasks.empty())
								return;
							task = std::move(tasks.front());
							tasks.pop();
						}
						task(thread_index);
					}
				});
			}
		}
	}

	void shutdown()
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			stop_flag = true;
		}
		condition.notify_all();
		for (auto &worker : workers)
			worker.join();
		workers.clear();
		stop_flag = false;
	}

	size_t size() const
	{
		return workers.size();
	}

  private:
	std::vector<std::thread>                workers;
	std::queue<std::function<void(size_t)>> tasks;
	std::mutex                              queue_mutex;
	std::condition_variable                 condition;
	std::atomic<bool>                       stop_flag;
};

/**
 * @brief Sample showing the use of secondary command buffers for
 *        multi-threaded recording, as well as the different
 *        strategies for recycling command buffers every frame
 */
class CommandBufferUsage : public vkb::VulkanSampleC
{
  public:
	CommandBufferUsage();

	virtual ~CommandBufferUsage() = default;

	virtual bool prepare(const vkb::ApplicationOptions &options) override;

	virtual void update(float delta_time) override;

	/**
	 * @brief Helper structure used to set subpass state
	 */
	struct ForwardSubpassSecondaryState
	{
		uint32_t secondary_cmd_buf_count = 0;

		vkb::CommandBufferResetMode command_buffer_reset_mode = vkb::CommandBufferResetMode::ResetPool;

		bool multi_threading = false;

		uint32_t thread_count = 0;
	};

	/**
	 * @brief Overrides the draw method to allow for dividing draw calls
	 *        into multiple secondary command buffers, optionally
	 *        in different threads
	 */
	class ForwardSubpassSecondary : public vkb::ForwardSubpass
	{
	  public:
		ForwardSubpassSecondary(vkb::rendering::RenderContextC &render_context,
		                        vkb::ShaderSource             &&vertex_source,
		                        vkb::ShaderSource             &&fragment_source,
		                        vkb::sg::Scene                 &scene,
		                        vkb::sg::Camera                &camera);

		void draw(vkb::core::CommandBufferC &primary_command_buffer) override;

		void set_viewport(VkViewport &viewport);

		void set_scissor(VkRect2D &scissor);

		float get_avg_draws_per_buffer() const;

		ForwardSubpassSecondaryState &get_state();

	  private:
		/**
		 * @brief Records the necessary commands to draw the specified range of scene meshes
		 * @param command_buffer The primary command buffer to record
		 * @param nodes The meshes to draw
		 * @param mesh_start Index to the first mesh to draw
		 * @param mesh_end Index to the mesh where recording will stop (not included)
		 * @param thread_index Identifies the resources allocated for this thread
		 */
		void record_draw(vkb::core::CommandBufferC                                                   &command_buffer,
		                 const std::vector<std::pair<vkb::scene_graph::NodeC *, vkb::sg::SubMesh *>> &nodes,
		                 uint32_t                                                                     mesh_start,
		                 uint32_t                                                                     mesh_end,
		                 size_t                                                                       thread_index = 0);

		/**
		 * @brief Records the necessary commands to draw the specified range of scene meshes
		 *        The primary command buffer provided is used to initialize, record, end and return a
		 *        pointer to a new secondary command buffer.
		 * @param primary_command_buffer The primary command buffer used to inherit a secondary
		 * @param nodes The meshes to draw
		 * @param mesh_start Index to the first mesh to draw
		 * @param mesh_end Index to the mesh where recording will stop (not included)
		 * @param thread_index Identifies the resources allocated for this thread
		 * @return a pointer to the recorded secondary command buffer
		 */
		std::shared_ptr<vkb::core::CommandBufferC> record_draw_secondary(vkb::core::CommandBufferC                                                   &primary_command_buffer,
		                                                                 const std::vector<std::pair<vkb::scene_graph::NodeC *, vkb::sg::SubMesh *>> &nodes,
		                                                                 uint32_t                                                                     mesh_start,
		                                                                 uint32_t                                                                     mesh_end,
		                                                                 size_t                                                                       thread_index = 0);

		VkViewport viewport{};

		VkRect2D scissor{};

		vkb::ColorBlendAttachmentState color_blend_attachment{};

		vkb::ColorBlendState color_blend_state{};

		ForwardSubpassSecondaryState state{};

		float avg_draws_per_buffer{0};

		ThreadPool thread_pool;
	};

  private:
	virtual void prepare_render_context() override;

	vkb::sg::PerspectiveCamera *camera{nullptr};

	void render(vkb::core::CommandBufferC &command_buffer) override;

	void draw_renderpass(vkb::core::CommandBufferC &primary_command_buffer, vkb::RenderTarget &render_target) override;

	void draw_gui() override;

	int gui_secondary_cmd_buf_count{0};

	uint32_t max_secondary_command_buffer_count{100};

	bool use_secondary_command_buffers{false};

	int gui_command_buffer_reset_mode{0};

	bool gui_multi_threading{false};

	const uint32_t MIN_THREAD_COUNT{4};

	uint32_t max_thread_count{0};
};

std::unique_ptr<vkb::VulkanSampleC> create_command_buffer_usage();
