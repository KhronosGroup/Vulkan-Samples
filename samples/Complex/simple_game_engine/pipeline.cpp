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
#include "pipeline.h"
#include "mesh_component.h"
#include <fstream>
#include <iostream>

// Constructor
Pipeline::Pipeline(VulkanDevice &device, SwapChain &swapChain) :
    device(device), swapChain(swapChain)
{
}

// Destructor
Pipeline::~Pipeline()
{
	// RAII will handle destruction
}

// Create descriptor set layout
bool Pipeline::createDescriptorSetLayout()
{
	try
	{
		// Create descriptor set layout bindings
		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 0,
		        .descriptorType     = vk::DescriptorType::eUniformBuffer,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		        .pImmutableSamplers = nullptr},
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 1,
		        .descriptorType     = vk::DescriptorType::eCombinedImageSampler,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
		        .pImmutableSamplers = nullptr}};

		// Create descriptor set layout
		vk::DescriptorSetLayoutCreateInfo layoutInfo{
		    .bindingCount = static_cast<uint32_t>(bindings.size()),
		    .pBindings    = bindings.data()};

		descriptorSetLayout = vk::raii::DescriptorSetLayout(device.getDevice(), layoutInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create descriptor set layout: " << e.what() << std::endl;
		return false;
	}
}

// Create PBR descriptor set layout
bool Pipeline::createPBRDescriptorSetLayout()
{
	try
	{
		// Create descriptor set layout bindings for PBR shader
		std::array<vk::DescriptorSetLayoutBinding, 7> bindings = {
		    // Binding 0: Uniform buffer (UBO)
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 0,
		        .descriptorType     = vk::DescriptorType::eUniformBuffer,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		        .pImmutableSamplers = nullptr},
		    // Binding 1: Base color map and sampler
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 1,
		        .descriptorType     = vk::DescriptorType::eCombinedImageSampler,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
		        .pImmutableSamplers = nullptr},
		    // Binding 2: Metallic roughness map and sampler
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 2,
		        .descriptorType     = vk::DescriptorType::eCombinedImageSampler,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
		        .pImmutableSamplers = nullptr},
		    // Binding 3: Normal map and sampler
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 3,
		        .descriptorType     = vk::DescriptorType::eCombinedImageSampler,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
		        .pImmutableSamplers = nullptr},
		    // Binding 4: Occlusion map and sampler
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 4,
		        .descriptorType     = vk::DescriptorType::eCombinedImageSampler,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
		        .pImmutableSamplers = nullptr},
		    // Binding 5: Emissive map and sampler
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 5,
		        .descriptorType     = vk::DescriptorType::eCombinedImageSampler,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
		        .pImmutableSamplers = nullptr},
		    // Binding 6: Light storage buffer (StructuredBuffer<LightData>)
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 6,
		        .descriptorType     = vk::DescriptorType::eStorageBuffer,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
		        .pImmutableSamplers = nullptr}};

		// Create descriptor set layout
		vk::DescriptorSetLayoutCreateInfo layoutInfo{
		    .bindingCount = static_cast<uint32_t>(bindings.size()),
		    .pBindings    = bindings.data()};

		pbrDescriptorSetLayout = vk::raii::DescriptorSetLayout(device.getDevice(), layoutInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create PBR descriptor set layout: " << e.what() << std::endl;
		return false;
	}
}

// Create graphics pipeline
bool Pipeline::createGraphicsPipeline()
{
	try
	{
		// Read shader code
		auto vertShaderCode = readFile("shaders/texturedMesh.spv");
		auto fragShaderCode = readFile("shaders/texturedMesh.spv");

		// Create shader modules
		vk::raii::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		vk::raii::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// Create shader stage info
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
		    .stage  = vk::ShaderStageFlagBits::eVertex,
		    .module = *vertShaderModule,
		    .pName  = "VSMain"};

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
		    .stage  = vk::ShaderStageFlagBits::eFragment,
		    .module = *fragShaderModule,
		    .pName  = "PSMain"};

		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// Create vertex input info
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
		    .vertexBindingDescriptionCount   = 0,
		    .pVertexBindingDescriptions      = nullptr,
		    .vertexAttributeDescriptionCount = 0,
		    .pVertexAttributeDescriptions    = nullptr};

		// Create input assembly info
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
		    .topology               = vk::PrimitiveTopology::eTriangleList,
		    .primitiveRestartEnable = VK_FALSE};

		// Create viewport state info
		vk::Viewport viewport{
		    .x        = 0.0f,
		    .y        = 0.0f,
		    .width    = static_cast<float>(swapChain.getSwapChainExtent().width),
		    .height   = static_cast<float>(swapChain.getSwapChainExtent().height),
		    .minDepth = 0.0f,
		    .maxDepth = 1.0f};

		vk::Rect2D scissor{
		    .offset = {0, 0},
		    .extent = swapChain.getSwapChainExtent()};

		vk::PipelineViewportStateCreateInfo viewportState{
		    .viewportCount = 1,
		    .pViewports    = &viewport,
		    .scissorCount  = 1,
		    .pScissors     = &scissor};

		// Create rasterization state info
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		    .depthClampEnable        = VK_FALSE,
		    .rasterizerDiscardEnable = VK_FALSE,
		    .polygonMode             = vk::PolygonMode::eFill,
		    .cullMode                = vk::CullModeFlagBits::eBack,
		    .frontFace               = vk::FrontFace::eCounterClockwise,
		    .depthBiasEnable         = VK_FALSE,
		    .depthBiasConstantFactor = 0.0f,
		    .depthBiasClamp          = 0.0f,
		    .depthBiasSlopeFactor    = 0.0f,
		    .lineWidth               = 1.0f};

		// Create multisample state info
		vk::PipelineMultisampleStateCreateInfo multisampling{
		    .rasterizationSamples  = vk::SampleCountFlagBits::e1,
		    .sampleShadingEnable   = VK_FALSE,
		    .minSampleShading      = 1.0f,
		    .pSampleMask           = nullptr,
		    .alphaToCoverageEnable = VK_FALSE,
		    .alphaToOneEnable      = VK_FALSE};

		// Create depth stencil state info
		vk::PipelineDepthStencilStateCreateInfo depthStencil{
		    .depthTestEnable       = VK_TRUE,
		    .depthWriteEnable      = VK_TRUE,
		    .depthCompareOp        = vk::CompareOp::eLess,
		    .depthBoundsTestEnable = VK_FALSE,
		    .stencilTestEnable     = VK_FALSE,
		    .front                 = {},
		    .back                  = {},
		    .minDepthBounds        = 0.0f,
		    .maxDepthBounds        = 1.0f};

		// Create color blend attachment state
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		    .blendEnable         = VK_FALSE,
		    .srcColorBlendFactor = vk::BlendFactor::eOne,
		    .dstColorBlendFactor = vk::BlendFactor::eZero,
		    .colorBlendOp        = vk::BlendOp::eAdd,
		    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
		    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
		    .alphaBlendOp        = vk::BlendOp::eAdd,
		    .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

		// Create color blend state info
		std::array                            blendConstants = {0.0f, 0.0f, 0.0f, 0.0f};
		vk::PipelineColorBlendStateCreateInfo colorBlending{
		    .logicOpEnable   = VK_FALSE,
		    .logicOp         = vk::LogicOp::eCopy,
		    .attachmentCount = 1,
		    .pAttachments    = &colorBlendAttachment,
		    .blendConstants  = blendConstants};

		// Create dynamic state info
		std::vector dynamicStates = {
		    vk::DynamicState::eViewport,
		    vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo dynamicState{
		    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
		    .pDynamicStates    = dynamicStates.data()};

		// Create pipeline layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
		    .setLayoutCount         = 1,
		    .pSetLayouts            = &*descriptorSetLayout,
		    .pushConstantRangeCount = 0,
		    .pPushConstantRanges    = nullptr};

		pipelineLayout = vk::raii::PipelineLayout(device.getDevice(), pipelineLayoutInfo);

		// Create graphics pipeline
		vk::GraphicsPipelineCreateInfo pipelineInfo{
		    .stageCount          = 2,
		    .pStages             = shaderStages,
		    .pVertexInputState   = &vertexInputInfo,
		    .pInputAssemblyState = &inputAssembly,
		    .pViewportState      = &viewportState,
		    .pRasterizationState = &rasterizer,
		    .pMultisampleState   = &multisampling,
		    .pDepthStencilState  = &depthStencil,
		    .pColorBlendState    = &colorBlending,
		    .pDynamicState       = &dynamicState,
		    .layout              = *pipelineLayout,
		    .renderPass          = nullptr,
		    .subpass             = 0,
		    .basePipelineHandle  = nullptr,
		    .basePipelineIndex   = -1};

		// Create pipeline with dynamic rendering
		vk::Format                      swapChainFormat = swapChain.getSwapChainImageFormat();
		vk::PipelineRenderingCreateInfo renderingInfo{
		    .colorAttachmentCount    = 1,
		    .pColorAttachmentFormats = &swapChainFormat,
		    .depthAttachmentFormat   = vk::Format::eD32Sfloat,
		    .stencilAttachmentFormat = vk::Format::eUndefined};

		pipelineInfo.pNext = &renderingInfo;

		graphicsPipeline = vk::raii::Pipeline(device.getDevice(), nullptr, pipelineInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create graphics pipeline: " << e.what() << std::endl;
		return false;
	}
}

// Create PBR pipeline
bool Pipeline::createPBRPipeline()
{
	try
	{
		// Create PBR descriptor set layout
		if (!createPBRDescriptorSetLayout())
		{
			return false;
		}

		// Read shader code
		auto vertShaderCode = readFile("shaders/pbr.spv");
		auto fragShaderCode = readFile("shaders/pbr.spv");

		// Create shader modules
		vk::raii::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		vk::raii::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// Create shader stage info
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
		    .stage  = vk::ShaderStageFlagBits::eVertex,
		    .module = *vertShaderModule,
		    .pName  = "VSMain"};

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
		    .stage  = vk::ShaderStageFlagBits::eFragment,
		    .module = *fragShaderModule,
		    .pName  = "PSMain"        // Changed from FSMain to PSMain to match the shader
		};

		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// Define vertex and instance binding descriptions using MeshComponent layouts
		auto                                             vertexBinding       = Vertex::getBindingDescription();
		auto                                             instanceBinding     = InstanceData::getBindingDescription();
		std::array<vk::VertexInputBindingDescription, 2> bindingDescriptions = {vertexBinding, instanceBinding};

		// Define vertex and instance attribute descriptions
		auto                                                vertexAttrArray   = Vertex::getAttributeDescriptions();
		auto                                                instanceAttrArray = InstanceData::getAttributeDescriptions();
		std::array<vk::VertexInputAttributeDescription, 11> attributeDescriptions{};
		// Copy vertex attributes (0..3)
		for (size_t i = 0; i < vertexAttrArray.size(); ++i)
		{
			attributeDescriptions[i] = vertexAttrArray[i];
		}
		// Copy instance attributes (4..10)
		for (size_t i = 0; i < instanceAttrArray.size(); ++i)
		{
			attributeDescriptions[vertexAttrArray.size() + i] = instanceAttrArray[i];
		}

		// Create vertex input info
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
		    .vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size()),
		    .pVertexBindingDescriptions      = bindingDescriptions.data(),
		    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
		    .pVertexAttributeDescriptions    = attributeDescriptions.data()};

		// Create input assembly info
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
		    .topology               = vk::PrimitiveTopology::eTriangleList,
		    .primitiveRestartEnable = VK_FALSE};

		// Create viewport state info
		vk::Viewport viewport{
		    .x        = 0.0f,
		    .y        = 0.0f,
		    .width    = static_cast<float>(swapChain.getSwapChainExtent().width),
		    .height   = static_cast<float>(swapChain.getSwapChainExtent().height),
		    .minDepth = 0.0f,
		    .maxDepth = 1.0f};

		vk::Rect2D scissor{
		    .offset = {0, 0},
		    .extent = swapChain.getSwapChainExtent()};

		vk::PipelineViewportStateCreateInfo viewportState{
		    .viewportCount = 1,
		    .pViewports    = &viewport,
		    .scissorCount  = 1,
		    .pScissors     = &scissor};

		// Create rasterization state info
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		    .depthClampEnable        = VK_FALSE,
		    .rasterizerDiscardEnable = VK_FALSE,
		    .polygonMode             = vk::PolygonMode::eFill,
		    .cullMode                = vk::CullModeFlagBits::eBack,
		    .frontFace               = vk::FrontFace::eCounterClockwise,
		    .depthBiasEnable         = VK_FALSE,
		    .depthBiasConstantFactor = 0.0f,
		    .depthBiasClamp          = 0.0f,
		    .depthBiasSlopeFactor    = 0.0f,
		    .lineWidth               = 1.0f};

		// Create multisample state info
		vk::PipelineMultisampleStateCreateInfo multisampling{
		    .rasterizationSamples  = vk::SampleCountFlagBits::e1,
		    .sampleShadingEnable   = VK_FALSE,
		    .minSampleShading      = 1.0f,
		    .pSampleMask           = nullptr,
		    .alphaToCoverageEnable = VK_TRUE,
		    .alphaToOneEnable      = VK_FALSE};

		// Create depth stencil state info
		vk::PipelineDepthStencilStateCreateInfo depthStencil{
		    .depthTestEnable       = VK_TRUE,
		    .depthWriteEnable      = VK_TRUE,
		    .depthCompareOp        = vk::CompareOp::eLess,
		    .depthBoundsTestEnable = VK_FALSE,
		    .stencilTestEnable     = VK_FALSE,
		    .front                 = {},
		    .back                  = {},
		    .minDepthBounds        = 0.0f,
		    .maxDepthBounds        = 1.0f};

		// Create color blend attachment state
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		    .blendEnable         = VK_FALSE,
		    .srcColorBlendFactor = vk::BlendFactor::eOne,
		    .dstColorBlendFactor = vk::BlendFactor::eZero,
		    .colorBlendOp        = vk::BlendOp::eAdd,
		    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
		    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
		    .alphaBlendOp        = vk::BlendOp::eAdd,
		    .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

		// Create color blend state info
		std::array<float, 4>                  blendConstants = {0.0f, 0.0f, 0.0f, 0.0f};
		vk::PipelineColorBlendStateCreateInfo colorBlending{
		    .logicOpEnable   = VK_FALSE,
		    .logicOp         = vk::LogicOp::eCopy,
		    .attachmentCount = 1,
		    .pAttachments    = &colorBlendAttachment,
		    .blendConstants  = blendConstants};

		// Create dynamic state info
		std::vector<vk::DynamicState> dynamicStates = {
		    vk::DynamicState::eViewport,
		    vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo dynamicState{
		    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
		    .pDynamicStates    = dynamicStates.data()};

		// Create push constant range for material properties
		vk::PushConstantRange pushConstantRange{
		    .stageFlags = vk::ShaderStageFlagBits::eFragment,
		    .offset     = 0,
		    .size       = sizeof(MaterialProperties)};

		// Create pipeline layout using the PBR descriptor set layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
		    .setLayoutCount         = 1,
		    .pSetLayouts            = &*pbrDescriptorSetLayout,        // Use PBR descriptor set layout
		    .pushConstantRangeCount = 1,
		    .pPushConstantRanges    = &pushConstantRange};

		pbrPipelineLayout = vk::raii::PipelineLayout(device.getDevice(), pipelineLayoutInfo);

		// Create graphics pipeline
		vk::GraphicsPipelineCreateInfo pipelineInfo{
		    .stageCount          = 2,
		    .pStages             = shaderStages,
		    .pVertexInputState   = &vertexInputInfo,
		    .pInputAssemblyState = &inputAssembly,
		    .pViewportState      = &viewportState,
		    .pRasterizationState = &rasterizer,
		    .pMultisampleState   = &multisampling,
		    .pDepthStencilState  = &depthStencil,
		    .pColorBlendState    = &colorBlending,
		    .pDynamicState       = &dynamicState,
		    .layout              = *pbrPipelineLayout,
		    .renderPass          = nullptr,
		    .subpass             = 0,
		    .basePipelineHandle  = nullptr,
		    .basePipelineIndex   = -1};

		// Create pipeline with dynamic rendering
		vk::Format                      swapChainFormat = swapChain.getSwapChainImageFormat();
		vk::PipelineRenderingCreateInfo renderingInfo{
		    .colorAttachmentCount    = 1,
		    .pColorAttachmentFormats = &swapChainFormat,
		    .depthAttachmentFormat   = vk::Format::eD32Sfloat,
		    .stencilAttachmentFormat = vk::Format::eUndefined};

		pipelineInfo.pNext = &renderingInfo;

		pbrGraphicsPipeline = vk::raii::Pipeline(device.getDevice(), nullptr, pipelineInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create PBR pipeline: " << e.what() << std::endl;
		return false;
	}
}

// Create lighting pipeline
bool Pipeline::createLightingPipeline()
{
	try
	{
		// Read shader code
		auto vertShaderCode = readFile("shaders/lighting.spv");
		auto fragShaderCode = readFile("shaders/lighting.spv");

		// Create shader modules
		vk::raii::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		vk::raii::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// Create shader stage info
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
		    .stage  = vk::ShaderStageFlagBits::eVertex,
		    .module = *vertShaderModule,
		    .pName  = "VSMain"};

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
		    .stage  = vk::ShaderStageFlagBits::eFragment,
		    .module = *fragShaderModule,
		    .pName  = "PSMain"};

		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// Create vertex input info
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
		    .vertexBindingDescriptionCount   = 0,
		    .pVertexBindingDescriptions      = nullptr,
		    .vertexAttributeDescriptionCount = 0,
		    .pVertexAttributeDescriptions    = nullptr};

		// Create input assembly info
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
		    .topology               = vk::PrimitiveTopology::eTriangleList,
		    .primitiveRestartEnable = VK_FALSE};

		// Create viewport state info
		vk::Viewport viewport{
		    .x        = 0.0f,
		    .y        = 0.0f,
		    .width    = static_cast<float>(swapChain.getSwapChainExtent().width),
		    .height   = static_cast<float>(swapChain.getSwapChainExtent().height),
		    .minDepth = 0.0f,
		    .maxDepth = 1.0f};

		vk::Rect2D scissor{
		    .offset = {0, 0},
		    .extent = swapChain.getSwapChainExtent()};

		vk::PipelineViewportStateCreateInfo viewportState{
		    .viewportCount = 1,
		    .pViewports    = &viewport,
		    .scissorCount  = 1,
		    .pScissors     = &scissor};

		// Create rasterization state info
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		    .depthClampEnable        = VK_FALSE,
		    .rasterizerDiscardEnable = VK_FALSE,
		    .polygonMode             = vk::PolygonMode::eFill,
		    .cullMode                = vk::CullModeFlagBits::eBack,
		    .frontFace               = vk::FrontFace::eCounterClockwise,
		    .depthBiasEnable         = VK_FALSE,
		    .depthBiasConstantFactor = 0.0f,
		    .depthBiasClamp          = 0.0f,
		    .depthBiasSlopeFactor    = 0.0f,
		    .lineWidth               = 1.0f};

		// Create multisample state info
		vk::PipelineMultisampleStateCreateInfo multisampling{
		    .rasterizationSamples  = vk::SampleCountFlagBits::e1,
		    .sampleShadingEnable   = VK_FALSE,
		    .minSampleShading      = 1.0f,
		    .pSampleMask           = nullptr,
		    .alphaToCoverageEnable = VK_FALSE,
		    .alphaToOneEnable      = VK_FALSE};

		// Create depth stencil state info
		vk::PipelineDepthStencilStateCreateInfo depthStencil{
		    .depthTestEnable       = VK_TRUE,
		    .depthWriteEnable      = VK_TRUE,
		    .depthCompareOp        = vk::CompareOp::eLess,
		    .depthBoundsTestEnable = VK_FALSE,
		    .stencilTestEnable     = VK_FALSE,
		    .front                 = {},
		    .back                  = {},
		    .minDepthBounds        = 0.0f,
		    .maxDepthBounds        = 1.0f};

		// Create color blend attachment state
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		    .blendEnable         = VK_FALSE,
		    .srcColorBlendFactor = vk::BlendFactor::eOne,
		    .dstColorBlendFactor = vk::BlendFactor::eZero,
		    .colorBlendOp        = vk::BlendOp::eAdd,
		    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
		    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
		    .alphaBlendOp        = vk::BlendOp::eAdd,
		    .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

		// Create color blend state info
		std::array<float, 4>                  blendConstants = {0.0f, 0.0f, 0.0f, 0.0f};
		vk::PipelineColorBlendStateCreateInfo colorBlending{
		    .logicOpEnable   = VK_FALSE,
		    .logicOp         = vk::LogicOp::eCopy,
		    .attachmentCount = 1,
		    .pAttachments    = &colorBlendAttachment,
		    .blendConstants  = blendConstants};

		// Create dynamic state info
		std::vector<vk::DynamicState> dynamicStates = {
		    vk::DynamicState::eViewport,
		    vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo dynamicState{
		    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
		    .pDynamicStates    = dynamicStates.data()};

		// Create push constant range for material properties
		vk::PushConstantRange pushConstantRange{
		    .stageFlags = vk::ShaderStageFlagBits::eFragment,
		    .offset     = 0,
		    .size       = sizeof(MaterialProperties)};

		// Create pipeline layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
		    .setLayoutCount         = 1,
		    .pSetLayouts            = &*descriptorSetLayout,
		    .pushConstantRangeCount = 1,
		    .pPushConstantRanges    = &pushConstantRange};

		lightingPipelineLayout = vk::raii::PipelineLayout(device.getDevice(), pipelineLayoutInfo);

		// Create graphics pipeline
		vk::GraphicsPipelineCreateInfo pipelineInfo{
		    .stageCount          = 2,
		    .pStages             = shaderStages,
		    .pVertexInputState   = &vertexInputInfo,
		    .pInputAssemblyState = &inputAssembly,
		    .pViewportState      = &viewportState,
		    .pRasterizationState = &rasterizer,
		    .pMultisampleState   = &multisampling,
		    .pDepthStencilState  = &depthStencil,
		    .pColorBlendState    = &colorBlending,
		    .pDynamicState       = &dynamicState,
		    .layout              = *lightingPipelineLayout,
		    .renderPass          = nullptr,
		    .subpass             = 0,
		    .basePipelineHandle  = nullptr,
		    .basePipelineIndex   = -1};

		// Create pipeline with dynamic rendering
		vk::Format                      swapChainFormat = swapChain.getSwapChainImageFormat();
		vk::PipelineRenderingCreateInfo renderingInfo{
		    .colorAttachmentCount    = 1,
		    .pColorAttachmentFormats = &swapChainFormat,
		    .depthAttachmentFormat   = vk::Format::eD32Sfloat,
		    .stencilAttachmentFormat = vk::Format::eUndefined};

		pipelineInfo.pNext = &renderingInfo;

		lightingPipeline = vk::raii::Pipeline(device.getDevice(), nullptr, pipelineInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create lighting pipeline: " << e.what() << std::endl;
		return false;
	}
}

// Push material properties
void Pipeline::pushMaterialProperties(vk::CommandBuffer commandBuffer, const MaterialProperties &material)
{
	commandBuffer.pushConstants<MaterialProperties>(*pbrPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, material);
}

// Create shader module
vk::raii::ShaderModule Pipeline::createShaderModule(const std::vector<char> &code)
{
	vk::ShaderModuleCreateInfo createInfo{
	    .codeSize = code.size(),
	    .pCode    = reinterpret_cast<const uint32_t *>(code.data())};

	return vk::raii::ShaderModule(device.getDevice(), createInfo);
}

// Read file
std::vector<char> Pipeline::readFile(const std::string &filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filename);
	}

	size_t            fileSize = file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}
