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

#include <ctpl_stl.h>
#include <memory>
#include <unordered_map>

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#define KHR_LIGHTS_PUNCTUAL_EXTENSION "KHR_lights_punctual"

#include "scene_graph/components/hpp_light.h"
#include "scene_graph/scripts/hpp_animation.h"

namespace vkb
{
namespace core
{
class HPPDevice;
}

namespace scene_graph
{
class HPPNode;
class HPPScene;

namespace components
{
class HPPCamera;
class HPPImage;
class HPPMesh;
class HPPPBRMaterial;
class HPPSampler;
class HPPSubMesh;
class HPPTexture;
class HPPTransform;
}        // namespace components
}        // namespace scene_graph

/**
 * @brief vulkan.hpp version of the vkb::GLTFLoader class
 *
 * See vkb::GLTFLoader for documentation
 */
class HPPGLTFLoader
{
  public:
	HPPGLTFLoader(vkb::core::HPPDevice &device);

	virtual ~HPPGLTFLoader() = default;

	/**
	 * @brief Loads the first model from a GLTF file for use in simpler samples
	 *        makes use of the Vertex struct in vulkan_example_base.h
	 */
	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh> read_model_from_file(const std::string &file_name, uint32_t index, bool storage_buffer = false);

	std::unique_ptr<vkb::scene_graph::HPPScene> read_scene_from_file(const std::string &file_name, int scene_index = -1);

  private:
	void                                                                              add_default_camera(vkb::scene_graph::HPPScene &scene) const;
	void                                                                              check_extensions() const;
	std::unique_ptr<vkb::scene_graph::components::HPPCamera>                          create_default_camera() const;
	std::unique_ptr<vkb::scene_graph::components::HPPPBRMaterial>                     create_default_material() const;
	std::unique_ptr<vkb::scene_graph::components::HPPSampler>                         create_default_sampler() const;
	tinygltf::Value const                                                            *get_extension(tinygltf::ExtensionMap const &tinygltf_extensions, const std::string &extension) const;
	bool                                                                              is_extension_enabled(const std::string &requested_extension) const;
	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh>                         load_model(uint32_t mesh_index, bool storage_buffer = false);
	vkb::scene_graph::HPPScene                                                        load_scene(int scene_index = -1);
	std::unique_ptr<vkb::scene_graph::scripts::HPPAnimation>                          parse_animation(tinygltf::Animation const                                     &gltf_animation,
	                                                                                                  std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>> const &nodes) const;
	void                                                                              parse_animation_channels(std::vector<tinygltf::AnimationChannel> const &channels, std::vector<vkb::sg::AnimationSampler> const &samplers,
	                                                                                                           std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>> const &nodes,
	                                                                                                           vkb::scene_graph::scripts::HPPAnimation                       &animation) const;
	vkb::sg::AnimationSampler                                                         parse_animation_sampler(tinygltf::AnimationSampler const &gltf_sampler, int sampler_index) const;
	std::vector<vkb::sg::AnimationSampler>                                            parse_animation_samplers(std::vector<tinygltf::AnimationSampler> const &gltf_samplers) const;
	std::vector<std::unique_ptr<vkb::scene_graph::scripts::HPPAnimation>>             parse_animations(vkb::scene_graph::HPPScene const &scene) const;
	std::unique_ptr<vkb::scene_graph::components::HPPCamera>                          parse_camera(const tinygltf::Camera &gltf_camera) const;
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPCamera>>             parse_cameras() const;
	std::unique_ptr<vkb::scene_graph::components::HPPImage>                           parse_image(tinygltf::Image &gltf_image) const;
	std::vector<std::future<std::unique_ptr<vkb::scene_graph::components::HPPImage>>> parse_image_futures(ctpl::thread_pool &thread_pool);
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPImage>>              parse_images(unsigned int thread_count);
	std::unique_ptr<vkb::scene_graph::components::HPPLight>                           parse_khr_light(tinygltf::Value const &gltf_light, int light_index) const;
	vkb::sg::LightProperties                                                          parse_khr_light_properties(tinygltf::Value const &khr_light, vkb::sg::LightType type) const;
	vkb::sg::LightType                                                                parse_khr_light_type(std::string const &type) const;
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPLight>>              parse_khr_lights_punctual() const;
	std::unique_ptr<vkb::scene_graph::components::HPPPBRMaterial>                     parse_material(const tinygltf::Material                                      &gltf_material,
	                                                                                                 std::vector<vkb::scene_graph::components::HPPTexture *> const &textures) const;
	void                                                                              parse_material_values(tinygltf::ParameterMap const &parameters, vkb::scene_graph::components::HPPPBRMaterial &material,
	                                                                                                        std::vector<vkb::scene_graph::components::HPPTexture *> const &textures) const;
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPPBRMaterial>>        parse_materials(vkb::scene_graph::HPPScene const &scene) const;
	std::unique_ptr<vkb::scene_graph::components::HPPMesh>                            parse_mesh(vkb::scene_graph::HPPScene &scene, const tinygltf::Mesh &gltf_mesh,
	                                                                                             std::vector<vkb::scene_graph::components::HPPPBRMaterial *> const &materials) const;
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPMesh>>               parse_meshes(vkb::scene_graph::HPPScene &scene) const;
	std::unique_ptr<vkb::scene_graph::HPPNode>                                        parse_node(const tinygltf::Node &gltf_node, size_t index,
	                                                                                             std::vector<vkb::scene_graph::components::HPPMesh *> const   &meshes,
	                                                                                             std::vector<vkb::scene_graph::components::HPPCamera *> const &cameras,
	                                                                                             std::vector<vkb::scene_graph::components::HPPLight *> const  &lights) const;
	void                                                                              parse_node_camera(const tinygltf::Node &gltf_node, std::vector<vkb::scene_graph::components::HPPCamera *> const &cameras,
	                                                                                                    vkb::scene_graph::HPPNode &node) const;
	void                                                                              parse_node_extension(const tinygltf::Node &gltf_node, std::vector<vkb::scene_graph::components::HPPLight *> const &lights,
	                                                                                                       vkb::scene_graph::HPPNode &node) const;
	void                                                                              parse_node_mesh(const tinygltf::Node &gltf_node, std::vector<vkb::scene_graph::components::HPPMesh *> const &meshes,
	                                                                                                  vkb::scene_graph::HPPNode &node) const;
	void                                                                              parse_node_transform(tinygltf::Node const &gltf_node, vkb::scene_graph::components::HPPTransform &transform) const;
	std::vector<std::unique_ptr<vkb::scene_graph::HPPNode>>                           parse_nodes(vkb::scene_graph::HPPScene const &scene) const;
	std::unique_ptr<vkb::scene_graph::components::HPPSubMesh>
	                                                                       parse_primitive(tinygltf::Primitive const &gltf_primitive, std::string const &mesh_name, size_t primitive_index,
	                                                                                       std::vector<vkb::scene_graph::components::HPPPBRMaterial *> const &materials) const;
	void                                                                   parse_primitive_attributes(std::map<std::string, int> const &gltf_attributes, std::string const &mesh_name, size_t primitive_index,
	                                                                                                  vkb::scene_graph::components::HPPSubMesh &submesh) const;
	void                                                                   parse_primitive_indices(int indicesId, std::string const &mesh_name, size_t primitive_index, vkb::scene_graph::components::HPPSubMesh &submesh) const;
	void                                                                   parse_primitive_material(int materialId, std::vector<vkb::scene_graph::components::HPPPBRMaterial *> const &materials,
	                                                                                                vkb::scene_graph::components::HPPSubMesh &submesh) const;
	std::unique_ptr<vkb::scene_graph::components::HPPSampler>              parse_sampler(const tinygltf::Sampler &gltf_sampler) const;
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPSampler>> parse_samplers() const;
	std::unique_ptr<vkb::scene_graph::HPPNode>                             parse_scene(vkb::scene_graph::HPPScene const &scene, int scene_index) const;
	std::unique_ptr<vkb::scene_graph::components::HPPTexture>              parse_texture(const tinygltf::Texture                                       &gltf_texture,
	                                                                                     std::vector<vkb::scene_graph::components::HPPImage *> const   &images,
	                                                                                     std::vector<vkb::scene_graph::components::HPPSampler *> const &samplers) const;
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPTexture>> parse_textures(vkb::scene_graph::HPPScene &scene) const;
	std::vector<std::unique_ptr<vkb::scene_graph::components::HPPImage>>
	    upload_images(std::vector<std::future<std::unique_ptr<vkb::scene_graph::components::HPPImage>>> &image_futures) const;

	vkb::core::HPPDevice &device;
	tinygltf::Model       model;
	std::string           model_path;
	static std::unordered_map<std::string, bool>
	    supported_extensions;        /// The extensions that the GLTFLoader can load mapped to whether they should be enabled or not
};
}        // namespace vkb
