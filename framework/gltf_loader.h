/* Copyright (c) 2018-2019, Arm Limited and Contributors
 * Copyright (c) 2019, Sascha Willems
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

#include <memory>
#include <mutex>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#include "timer.h"

namespace vkb
{
class Device;

namespace sg
{
class Scene;
class Node;
class Camera;
class Image;
class Mesh;
class PBRMaterial;
class Sampler;
class SubMesh;
class Texture;
}        // namespace sg

/**
 * @brief Helper Function to change array type T to array type Y
 * Create a struct that can be used with std::transform so that we do not need to recreate lambda functions
 * @param T 
 * @param Y 
 */
template <class T, class Y>
struct TypeCast
{
	Y operator()(T value) const noexcept
	{
		return static_cast<Y>(value);
	}
};

/// Read a gltf file and return a scene object. Converts the gltf objects
/// to our internal scene implementation. Mesh data is copied to vulkan buffers and
/// images are loaded from the folder of gltf file to vulkan images.
class GLTFLoader
{
  public:
	GLTFLoader(Device &device);

	virtual ~GLTFLoader() = default;

	std::unique_ptr<sg::Scene> read_scene_from_file(const std::string &file_name);

	/**
	 * @brief Loads the first model from a GLTF file for use in simpler samples
	 *        makes use of the Vertex struct in vulkan_example_base.h
	 */
	std::unique_ptr<sg::SubMesh> read_model_from_file(const std::string &file_name, uint32_t index);

  protected:
	virtual std::unique_ptr<sg::Node> parse_node(const tinygltf::Node &gltf_node, size_t index) const;

	virtual std::unique_ptr<sg::Camera> parse_camera(const tinygltf::Camera &gltf_camera) const;

	virtual std::unique_ptr<sg::Mesh> parse_mesh(const tinygltf::Mesh &gltf_mesh) const;

	virtual std::unique_ptr<sg::PBRMaterial> parse_material(const tinygltf::Material &gltf_material) const;

	virtual std::unique_ptr<sg::Image> parse_image(tinygltf::Image &gltf_image) const;

	virtual std::unique_ptr<sg::Sampler> parse_sampler(const tinygltf::Sampler &gltf_sampler) const;

	virtual std::unique_ptr<sg::Texture> parse_texture(const tinygltf::Texture &gltf_texture) const;

	virtual std::unique_ptr<sg::PBRMaterial> create_default_material();

	virtual std::unique_ptr<sg::Sampler> create_default_sampler();

	virtual std::unique_ptr<sg::Camera> create_default_camera();

	Device &device;

	tinygltf::Model model;

	std::string model_path;

  private:
	sg::Scene load_scene();

	std::unique_ptr<sg::SubMesh> load_model(uint32_t index);
};
}        // namespace vkb
