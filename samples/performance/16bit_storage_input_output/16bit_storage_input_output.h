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

#include "scene_graph/components/camera.h"
#include "vulkan_sample.h"

/**
 * @brief Using 16-bit storage features to reduce bandwidth for input-output data between vertex and fragment shaders.
 */
class KHR16BitStorageInputOutputSample : public vkb::VulkanSample
{
  public:
	KHR16BitStorageInputOutputSample();

	virtual ~KHR16BitStorageInputOutputSample() = default;

	virtual bool prepare(vkb::Platform &platform) override;

	virtual void update(float delta_time) override;

	virtual void request_gpu_features(vkb::PhysicalDevice &gpu) override;

  private:
	vkb::sg::Camera *camera{nullptr};

	virtual void draw_gui() override;

	void recreate_swapchain();

	bool khr_16bit_storage_input_output_last_enabled{false};

	bool khr_16bit_storage_input_output_enabled{false};

	void setup_scene();

	struct Transform
	{
		vkb::sg::Transform *transform;
		glm::vec3 axis;
		float angular_frequency;
	};
	std::vector<Transform> teapot_transforms;

	void update_pipeline();
	bool supports_16bit_storage{false};
};

std::unique_ptr<vkb::VulkanSample> create_16bit_storage_input_output();
