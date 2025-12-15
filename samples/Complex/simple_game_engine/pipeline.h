/* Copyright (c) 2025 Holochip Corporation
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

#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "swap_chain.h"
#include "vulkan_device.h"

/**
 * @brief Structure for PBR material properties.
 * This structure must match the PushConstants structure in the PBR shader.
 */
struct MaterialProperties
{
	alignas(16) glm::vec4 baseColorFactor;
	alignas(4) float metallicFactor;
	alignas(4) float roughnessFactor;
	alignas(4) int baseColorTextureSet;
	alignas(4) int physicalDescriptorTextureSet;
	alignas(4) int normalTextureSet;
	alignas(4) int occlusionTextureSet;
	alignas(4) int emissiveTextureSet;
	alignas(4) float alphaMask;
	alignas(4) float alphaMaskCutoff;
	alignas(16) glm::vec3 emissiveFactor;        // Emissive factor for HDR emissive sources
	alignas(4) float emissiveStrength;           // KHR_materials_emissive_strength extension
	alignas(4) float transmissionFactor;         // KHR_materials_transmission
	alignas(4) int useSpecGlossWorkflow;         // 1 if using KHR_materials_pbrSpecularGlossiness
	alignas(4) float glossinessFactor;           // SpecGloss glossiness scalar
	alignas(16) glm::vec3 specularFactor;        // SpecGloss specular color factor
	alignas(4) float ior;                        // Index of refraction for transmission
};

/**
 * @brief Class for managing Vulkan pipelines.
 */
class Pipeline
{
  public:
	/**
	 * @brief Constructor.
	 * @param device The Vulkan device.
	 * @param swapChain The swap chain.
	 */
	Pipeline(VulkanDevice &device, SwapChain &swapChain);

	/**
	 * @brief Destructor.
	 */
	~Pipeline();

	/**
	 * @brief Create the descriptor set layout.
	 * @return True if the descriptor set layout was created successfully, false otherwise.
	 */
	bool createDescriptorSetLayout();

	/**
	 * @brief Create the PBR descriptor set layout.
	 * @return True if the PBR descriptor set layout was created successfully, false otherwise.
	 */
	bool createPBRDescriptorSetLayout();

	/**
	 * @brief Create the graphics pipeline.
	 * @return True if the graphics pipeline was created successfully, false otherwise.
	 */
	bool createGraphicsPipeline();

	/**
	 * @brief Create the PBR pipeline.
	 * @return True if the PBR pipeline was created successfully, false otherwise.
	 */
	bool createPBRPipeline();

	/**
	 * @brief Create the lighting pipeline.
	 * @return True if the lighting pipeline was created successfully, false otherwise.
	 */
	bool createLightingPipeline();

	/**
	 * @brief Push material properties to a command buffer.
	 * @param commandBuffer The command buffer.
	 * @param material The material properties.
	 */
	void pushMaterialProperties(vk::CommandBuffer commandBuffer, const MaterialProperties &material);

	/**
	 * @brief Get the descriptor set layout.
	 * @return The descriptor set layout.
	 */
	vk::raii::DescriptorSetLayout &getDescriptorSetLayout()
	{
		return descriptorSetLayout;
	}

	/**
	 * @brief Get the pipeline layout.
	 * @return The pipeline layout.
	 */
	vk::raii::PipelineLayout &getPipelineLayout()
	{
		return pipelineLayout;
	}

	/**
	 * @brief Get the graphics pipeline.
	 * @return The graphics pipeline.
	 */
	vk::raii::Pipeline &getGraphicsPipeline()
	{
		return graphicsPipeline;
	}

	/**
	 * @brief Get the PBR pipeline layout.
	 * @return The PBR pipeline layout.
	 */
	vk::raii::PipelineLayout &getPBRPipelineLayout()
	{
		return pbrPipelineLayout;
	}

	/**
	 * @brief Get the PBR graphics pipeline.
	 * @return The PBR graphics pipeline.
	 */
	vk::raii::Pipeline &getPBRGraphicsPipeline()
	{
		return pbrGraphicsPipeline;
	}

	/**
	 * @brief Get the lighting pipeline layout.
	 * @return The lighting pipeline layout.
	 */
	vk::raii::PipelineLayout &getLightingPipelineLayout()
	{
		return lightingPipelineLayout;
	}

	/**
	 * @brief Get the lighting pipeline.
	 * @return The lighting pipeline.
	 */
	vk::raii::Pipeline &getLightingPipeline()
	{
		return lightingPipeline;
	}

	/**
	 * @brief Get the compute pipeline layout.
	 * @return The compute pipeline layout.
	 */
	vk::raii::PipelineLayout &getComputePipelineLayout()
	{
		return computePipelineLayout;
	}

	/**
	 * @brief Get the compute pipeline.
	 * @return The compute pipeline.
	 */
	vk::raii::Pipeline &getComputePipeline()
	{
		return computePipeline;
	}

	/**
	 * @brief Get the compute descriptor set layout.
	 * @return The compute descriptor set layout.
	 */
	vk::raii::DescriptorSetLayout &getComputeDescriptorSetLayout()
	{
		return computeDescriptorSetLayout;
	}

	/**
	 * @brief Get the PBR descriptor set layout.
	 * @return The PBR descriptor set layout.
	 */
	vk::raii::DescriptorSetLayout &getPBRDescriptorSetLayout()
	{
		return pbrDescriptorSetLayout;
	}

  private:
	// Vulkan device
	VulkanDevice &device;

	// Swap chain
	SwapChain &swapChain;

	// Pipelines
	vk::raii::PipelineLayout pipelineLayout         = nullptr;
	vk::raii::Pipeline       graphicsPipeline       = nullptr;
	vk::raii::PipelineLayout pbrPipelineLayout      = nullptr;
	vk::raii::Pipeline       pbrGraphicsPipeline    = nullptr;
	vk::raii::PipelineLayout lightingPipelineLayout = nullptr;
	vk::raii::Pipeline       lightingPipeline       = nullptr;

	// Compute pipeline
	vk::raii::PipelineLayout      computePipelineLayout      = nullptr;
	vk::raii::Pipeline            computePipeline            = nullptr;
	vk::raii::DescriptorSetLayout computeDescriptorSetLayout = nullptr;

	// Descriptor set layouts
	vk::raii::DescriptorSetLayout descriptorSetLayout    = nullptr;
	vk::raii::DescriptorSetLayout pbrDescriptorSetLayout = nullptr;

	// Helper functions
	vk::raii::ShaderModule createShaderModule(const std::vector<char> &code);
	std::vector<char>      readFile(const std::string &filename);
};
