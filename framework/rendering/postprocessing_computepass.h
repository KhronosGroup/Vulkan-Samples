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

#include "common/glm_common.h"
#include "core/sampled_image.h"
#include "core/shader_module.h"
#include "postprocessing_pass.h"

namespace vkb
{
/**
 * @brief Maps in-shader binding names to the core::SampledImage to bind.
 */
using SampledImageMap = std::unordered_map<std::string, core::SampledImage>;

/**
* @brief A compute pass in a vkb::PostProcessingPipeline.
*/
class PostProcessingComputePass : public PostProcessingPass<PostProcessingComputePass>
{
  public:
	PostProcessingComputePass(PostProcessingPipeline *parent, const ShaderSource &cs_source, const ShaderVariant &cs_variant = {},
	                          std::shared_ptr<core::Sampler> &&default_sampler = {});

	PostProcessingComputePass(const PostProcessingComputePass &to_copy) = delete;
	PostProcessingComputePass &operator=(const PostProcessingComputePass &to_copy) = delete;

	PostProcessingComputePass(PostProcessingComputePass &&to_move) = default;
	PostProcessingComputePass &operator=(PostProcessingComputePass &&to_move) = default;

	void prepare(CommandBuffer &command_buffer, RenderTarget &default_render_target) override;
	void draw(CommandBuffer &command_buffer, RenderTarget &default_render_target) override;

	/**
	 * @brief Sets the number of workgroups to be dispatched each draw().
	 */
	inline PostProcessingComputePass &set_dispatch_size(glm::tvec3<uint32_t> new_size)
	{
		n_workgroups = new_size;
		return *this;
	}

	/**
	 * @brief Gets the number of workgroups that will be dispatched each draw().
	 */
	inline glm::tvec3<uint32_t> get_dispatch_size() const
	{
		return n_workgroups;
	}

	/**
	* @brief Maps the names of samplers in the shader to vkb::core::SampledImage.
	*        These are given as samplers to the subpass, at set 0; they are bound automatically according to their name.
	* @remarks PostProcessingPipeline::get_sampler() is used as the default sampler if none is specified.
	*          The RenderTarget for the current PostprocessingStep is used if none is specified for attachment images.
	*/
	inline const SampledImageMap &get_sampled_images() const
	{
		return sampled_images;
	}

	/**
	* @brief Maps the names of storage images in the shader to vkb::core::SampledImage.
	*        These are given as image2D / image2DArray / ... to the subpass, at set 0;
	*        they are bound automatically according to their name.
	*/
	inline const SampledImageMap &get_storage_images() const
	{
		return storage_images;
	}

	/**
	 * @brief Changes (or adds) the sampled image at name for this step.
	 * @remarks If no RenderTarget is specifically set for the core::SampledImage,
	 *          it will default to sample in the RenderTarget currently bound for drawing in the parent PostProcessingRenderpass.
	 * @remarks Images from RenderTarget attachments are automatically transitioned to SHADER_READ_ONLY_OPTIMAL layout if needed.
	 */
	PostProcessingComputePass &bind_sampled_image(const std::string &name, core::SampledImage &&new_image);

	/**
	 * @brief Changes (or adds) the storage image at name for this step.
	 * @remarks Images from RenderTarget attachments are automatically transitioned to GENERAL layout if needed.
	 */
	PostProcessingComputePass &bind_storage_image(const std::string &name, core::SampledImage &&new_image);

	/**
	 * @brief Set the uniform data to be bound at set 0, binding 0.
	 */
	template <typename T>
	inline PostProcessingComputePass &set_uniform_data(const T &data)
	{
		uniform_data.reserve(sizeof(data));
		auto data_ptr = reinterpret_cast<const uint8_t *>(&data);
		uniform_data.assign(data_ptr, data_ptr + sizeof(data));

		return *this;
	}

	/**
	 * @copydoc set_uniform_data(const T&)
	 */
	inline PostProcessingComputePass &set_uniform_data(const std::vector<uint8_t> &data)
	{
		uniform_data = data;

		return *this;
	}

	/**
	 * @brief Set the constants that are pushed before each draw.
	 */
	template <typename T>
	inline PostProcessingComputePass &set_push_constants(const T &data)
	{
		push_constants_data.reserve(sizeof(data));
		auto data_ptr = reinterpret_cast<const uint8_t *>(&data);
		push_constants_data.assign(data_ptr, data_ptr + sizeof(data));

		return *this;
	}

	/**
	 * @copydoc set_push_constants(const T&)
	 */
	inline PostProcessingComputePass &set_push_constants(const std::vector<uint8_t> &data)
	{
		push_constants_data = data;

		return *this;
	}

  private:
	ShaderSource         cs_source;
	ShaderVariant        cs_variant;
	glm::tvec3<uint32_t> n_workgroups{1, 1, 1};

	std::shared_ptr<core::Sampler> default_sampler{};
	SampledImageMap                sampled_images{};
	SampledImageMap                storage_images{};

	std::vector<uint8_t>              uniform_data{};
	std::unique_ptr<BufferAllocation> uniform_alloc{};
	std::vector<uint8_t>              push_constants_data{};

	/**
	 * @brief Transitions sampled_images (to SHADER_READ_ONLY_OPTIMAL)
	 *        and storage_images (to GENERAL) as appropriate.
	 */
	void transition_images(CommandBuffer &command_buffer, RenderTarget &default_render_target);

	BarrierInfo get_src_barrier_info() const override;
	BarrierInfo get_dst_barrier_info() const override;
};

}        // namespace vkb