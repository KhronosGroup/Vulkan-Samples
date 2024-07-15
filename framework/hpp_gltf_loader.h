/* Copyright (c) 2021-2024, NVIDIA CORPORATION. All rights reserved.
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

#include <gltf_loader.h>

#include <core/hpp_device.h>
#include <scene_graph/components/hpp_sub_mesh.h>
#include <scene_graph/hpp_scene.h>

namespace vkb
{
/**
 * @brief facade class around vkb::GLTFLoader, providing a vulkan.hpp-based interface
 *
 * See vkb::GLTFLoader for documentation
 */
class HPPGLTFLoader : private vkb::GLTFLoader
{
  public:
	HPPGLTFLoader(vkb::core::HPPDevice &device) :
	    GLTFLoader(reinterpret_cast<vkb::Device &>(device))
	{}

	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> read_model_from_file(const std::string &file_name, uint32_t index)
	{
		return std::unique_ptr<vkb::scene_graph::components::HPPSubMesh>(
		    reinterpret_cast<vkb::scene_graph::components::HPPSubMesh *>(
		        vkb::GLTFLoader::read_model_from_file(file_name, index).release()));
	}

	std::unique_ptr<vkb::scene_graph::HPPScene> read_scene_from_file(const std::string &file_name, int scene_index = -1)
	{
		return std::unique_ptr<vkb::scene_graph::HPPScene>(reinterpret_cast<vkb::scene_graph::HPPScene *>(vkb::GLTFLoader::read_scene_from_file(file_name, scene_index).release()));
	}
};
}        // namespace vkb
