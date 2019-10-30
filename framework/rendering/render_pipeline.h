/* Copyright (c) 2019, Arm Limited and Contributors
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

#include "common/helpers.h"
#include "common/utils.h"
#include "core/buffer.h"
#include "rendering/render_frame.h"
#include "rendering/subpass.h"

namespace vkb
{
/**
 * @brief A RenderPipeline is a sequence of Subpass objects.
 * The most basic one is SceneSubpass.
 * Subpass holds shaders and can draw the core::sg::Scene.
 * SceneSubpass holds a GlobalUniform for uniform data across all meshes.
 * More subpasses can be added to the sequence if required.
 * For example, postprocessing can be implemented with two pipelines which
 * share render targets.
 */
class RenderPipeline : public NonCopyable
{
  public:
	RenderPipeline(std::vector<std::unique_ptr<Subpass>> &&subpasses = {});

	virtual ~RenderPipeline() = default;

	RenderPipeline(RenderPipeline &&other) = default;

	/**
	 * @return Load store info
	 */
	const std::vector<LoadStoreInfo> &get_load_store() const;

	/**
	 * @param load_store Load store info to set
	 */
	void set_load_store(const std::vector<LoadStoreInfo> &load_store);

	/**
	 * @return Clear values
	 */
	const std::vector<VkClearValue> &get_clear_value() const;

	/**
	 * @param clear_values Clear values to set
	 */
	void set_clear_value(const std::vector<VkClearValue> &clear_values);

	/**
	 * @brief Appends a subpass to the pipeline
	 * @param subpass Subpass to append
	 */
	void add_subpass(std::unique_ptr<Subpass> &&subpass);

	/**
	 * @brief Record draw commands for each Subpass
	 */
	void draw(CommandBuffer &command_buffer, RenderTarget &render_target, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

  private:
	std::vector<std::unique_ptr<Subpass>> subpasses;

	/// Default to two load store
	std::vector<LoadStoreInfo> load_store = std::vector<LoadStoreInfo>(2);

	/// Default to two clear values
	std::vector<VkClearValue> clear_value = std::vector<VkClearValue>(2);
};
}        // namespace vkb
