/* Copyright (c) 2020, Arm Limited and Contributors
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

#include "core/command_buffer.h"
#include "render_context.h"
#include "render_target.h"
#include <functional>

namespace vkb
{
class PostProcessingPipeline;

/**
* @brief The base of all types of passes in a vkb::PostProcessingPipeline.
*/
class PostProcessingPassBase
{
	friend class PostProcessingPipeline;

  public:
	PostProcessingPassBase(PostProcessingPipeline *parent);

	PostProcessingPassBase(const PostProcessingPassBase &to_copy) = delete;
	PostProcessingPassBase &operator=(const PostProcessingPassBase &to_copy) = delete;

	PostProcessingPassBase(PostProcessingPassBase &&to_move) = default;
	PostProcessingPassBase &operator=(PostProcessingPassBase &&to_move) = default;

	virtual ~PostProcessingPassBase() = default;

  protected:
	/**
	 * @brief Prepares this pass, recording commands into the given command buffer.
	 * @remarks Passes that that do not explicitly have a vkb::RenderTarget set will render
	 *          to default_render_target.
	 */
	virtual void prepare(CommandBuffer &command_buffer, RenderTarget &default_render_target)
	{
		prepared = true;
	}

	/**
	 * @brief Runs this pass, recording commands into the given command buffer.
	 * @remarks Passes that that do not explicitly have a vkb::RenderTarget set will render
	 *          to default_render_target.
	 */
	virtual void draw(CommandBuffer &command_buffer, RenderTarget &default_render_target)
	{}

	/**
	 * @brief A functor ran in the context of this renderpass.
	 * @see set_pre_draw_func(), set_post_draw_func()
	 */
	using HookFunc = std::function<void()>;

	// NOTE: Protected members are exposed via getters and setters in PostProcessingPass<>
	PostProcessingPipeline *parent{nullptr};
	bool                    prepared{false};

	std::string debug_name{};

	RenderTarget *                 render_target{nullptr};
	std::shared_ptr<core::Sampler> default_sampler{};

	HookFunc pre_draw{};
	HookFunc post_draw{};

	/**
	 * @brief Returns the parent's render context.
	 */
	RenderContext &get_render_context() const;

	/**
	* @brief Returns the parent's fullscreen triangle vertex shader source.
	*/
	ShaderSource &get_triangle_vs() const;

	struct BarrierInfo
	{
		VkPipelineStageFlags pipeline_stage;            // Pipeline stage of this pass' inputs/outputs
		VkAccessFlags        image_read_access;         // Access mask for images read from this pass
		VkAccessFlags        image_write_access;        // Access mask for images written to by this pass
	};

	/**
	* @brief Returns information that can be used to setup memory barriers of images
	*        that are produced (e.g. image stores, color attachment output) by this pass.
	*/
	virtual BarrierInfo get_src_barrier_info() const = 0;

	/**
	* @brief Returns information that can be used to setup memory barriers of images
	*        that are consumed (e.g. image loads, texture sampling) by this pass.
	*/
	virtual BarrierInfo get_dst_barrier_info() const = 0;

	/**
	 * @brief Convenience function that calls get_src_barrier_info() on the previous pass of the pipeline,
	 *        if any, or returns the specified default if this is the first pass in the pipeline.
	 */
	BarrierInfo get_predecessor_src_barrier_info(BarrierInfo fallback = {}) const;
};

/**
* @brief CRTP base of all types of passes in a vkb::PostProcessingPipeline.
*/
template <typename Self>
class PostProcessingPass : public PostProcessingPassBase
{
  public:
	using PostProcessingPassBase::PostProcessingPassBase;

	PostProcessingPass(const PostProcessingPass &to_copy) = delete;
	PostProcessingPass &operator=(const PostProcessingPass &to_copy) = delete;

	PostProcessingPass(PostProcessingPass &&to_move) = default;
	PostProcessingPass &operator=(PostProcessingPass &&to_move) = default;

	virtual ~PostProcessingPass() = default;

	/**
	 * @brief Sets a functor that, if non-null, will be invoked before draw()ing this pass.
	 * @remarks The function is invoked after ending the previous RenderPass, and before beginning this one.
	 */
	inline Self &set_pre_draw_func(HookFunc &&new_func)
	{
		pre_draw = std::move(new_func);

		return static_cast<Self &>(*this);
	}

	/**
	 * @brief Sets a functor that, if non-null, will be invoked after draw()ing this pass.
	 * @remarks The function after drawing the last subpass, and before ending this RenderPass.
	 */
	inline Self &set_post_draw_func(HookFunc &&new_func)
	{
		post_draw = std::move(new_func);

		return static_cast<Self &>(*this);
	}

	/**
	* @brief Render target to output to.
	*        If set, this pass will output to the given render target instead of the one passed to draw().
	*/
	inline RenderTarget *get_render_target() const
	{
		return render_target;
	}

	/**
	* @copydoc get_render_target()
	*/
	inline Self &set_render_target(RenderTarget *new_render_target)
	{
		render_target = new_render_target;

		return static_cast<Self &>(*this);
	}

	/**
	* @brief Returns the the debug name of this pass.
	*/
	inline const std::string &get_debug_name() const
	{
		return debug_name;
	}

	/**
	* @brief Sets the debug name of this pass.
	*/
	inline Self &set_debug_name(const std::string &new_debug_name)
	{
		debug_name = new_debug_name;

		return static_cast<Self &>(*this);
	}

	/**
	* @brief Returns the vkb::PostProcessingPipeline that is the parent of this pass.
	*/
	inline PostProcessingPipeline &get_parent() const
	{
		return *parent;
	}
};

}        // namespace vkb