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
#include "renderer.h"
#include <array>
#include <fstream>
#include <iostream>

// This file contains compute-related methods from the Renderer class

// Create compute pipeline
bool Renderer::createComputePipeline()
{
	try
	{
		// Read compute shader code
		auto computeShaderCode = readFile("shaders/hrtf.spv");

		// Create shader module
		vk::raii::ShaderModule computeShaderModule = createShaderModule(computeShaderCode);

		// Create shader stage info
		vk::PipelineShaderStageCreateInfo computeShaderStageInfo{
		    .stage  = vk::ShaderStageFlagBits::eCompute,
		    .module = *computeShaderModule,
		    .pName  = "main"};

		// Create compute descriptor set layout
		std::array<vk::DescriptorSetLayoutBinding, 4> computeBindings = {
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 0,
		        .descriptorType     = vk::DescriptorType::eStorageBuffer,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eCompute,
		        .pImmutableSamplers = nullptr},
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 1,
		        .descriptorType     = vk::DescriptorType::eStorageBuffer,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eCompute,
		        .pImmutableSamplers = nullptr},
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 2,
		        .descriptorType     = vk::DescriptorType::eStorageBuffer,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eCompute,
		        .pImmutableSamplers = nullptr},
		    vk::DescriptorSetLayoutBinding{
		        .binding            = 3,
		        .descriptorType     = vk::DescriptorType::eUniformBuffer,
		        .descriptorCount    = 1,
		        .stageFlags         = vk::ShaderStageFlagBits::eCompute,
		        .pImmutableSamplers = nullptr}};

		vk::DescriptorSetLayoutCreateInfo computeLayoutInfo{
		    .bindingCount = static_cast<uint32_t>(computeBindings.size()),
		    .pBindings    = computeBindings.data()};

		computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, computeLayoutInfo);

		// Create compute pipeline layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
		    .setLayoutCount         = 1,
		    .pSetLayouts            = &*computeDescriptorSetLayout,
		    .pushConstantRangeCount = 0,
		    .pPushConstantRanges    = nullptr};

		computePipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		// Create compute pipeline
		vk::ComputePipelineCreateInfo pipelineInfo{
		    .stage  = computeShaderStageInfo,
		    .layout = *computePipelineLayout};

		computePipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);

		// Create compute descriptor pool
		std::array<vk::DescriptorPoolSize, 2> poolSizes = {
		    vk::DescriptorPoolSize{
		        .type            = vk::DescriptorType::eStorageBuffer,
		        .descriptorCount = 6u * MAX_FRAMES_IN_FLIGHT        // room for multiple compute pipelines
		    },
		    vk::DescriptorPoolSize{
		        .type            = vk::DescriptorType::eUniformBuffer,
		        .descriptorCount = 2u * MAX_FRAMES_IN_FLIGHT}};

		vk::DescriptorPoolCreateInfo poolInfo{
		    .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		    .maxSets       = 2u * MAX_FRAMES_IN_FLIGHT,
		    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		    .pPoolSizes    = poolSizes.data()};

		computeDescriptorPool = vk::raii::DescriptorPool(device, poolInfo);

		return createComputeCommandPool();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create compute pipeline: " << e.what() << std::endl;
		return false;
	}
}

// Forward+ compute (tiled light culling)
bool Renderer::createForwardPlusPipelinesAndResources()
{
	try
	{
		// Load compute shader
		auto                   cullSpv    = readFile("shaders/forward_plus_cull.spv");
		vk::raii::ShaderModule cullModule = createShaderModule(cullSpv);

		// Descriptor set layout: 0=lights SSBO (RO), 1=tile headers SSBO (RW), 2=tile indices SSBO (RW), 3=params UBO (RO)
		std::array<vk::DescriptorSetLayoutBinding, 4> bindings = {
		    vk::DescriptorSetLayoutBinding{.binding = 0, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		    vk::DescriptorSetLayoutBinding{.binding = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		    vk::DescriptorSetLayoutBinding{.binding = 2, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		    vk::DescriptorSetLayoutBinding{.binding = 3, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute}};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()};
		forwardPlusDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);

		// Pipeline layout
		vk::PipelineLayoutCreateInfo plInfo{.setLayoutCount = 1, .pSetLayouts = &*forwardPlusDescriptorSetLayout};
		forwardPlusPipelineLayout = vk::raii::PipelineLayout(device, plInfo);

		// Pipeline
		vk::PipelineShaderStageCreateInfo stage{.stage = vk::ShaderStageFlagBits::eCompute, .module = *cullModule, .pName = "main"};
		vk::ComputePipelineCreateInfo     cpInfo{.stage = stage, .layout = *forwardPlusPipelineLayout};
		forwardPlusPipeline = vk::raii::Pipeline(device, nullptr, cpInfo);

		// Allocate per-frame structs
		forwardPlusPerFrame.resize(MAX_FRAMES_IN_FLIGHT);

		// Allocate compute descriptor sets (reuse computeDescriptorPool)
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *forwardPlusDescriptorSetLayout);
		vk::DescriptorSetAllocateInfo        allocInfo{.descriptorPool = *computeDescriptorPool, .descriptorSetCount = MAX_FRAMES_IN_FLIGHT, .pSetLayouts = layouts.data()};
		auto                                 sets = vk::raii::DescriptorSets(device, allocInfo);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			forwardPlusPerFrame[i].computeSet = std::move(sets[i]);
		}

		// Initial buffer allocation based on current swapchain extent (also updates descriptors)
		uint32_t tilesX = (swapChainExtent.width + forwardPlusTileSizeX - 1) / forwardPlusTileSizeX;
		uint32_t tilesY = (swapChainExtent.height + forwardPlusTileSizeY - 1) / forwardPlusTileSizeY;
		if (!createOrResizeForwardPlusBuffers(tilesX, tilesY, forwardPlusSlicesZ))
		{
			return false;
		}

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create Forward+ compute resources: " << e.what() << std::endl;
		return false;
	}
}

bool Renderer::createOrResizeForwardPlusBuffers(uint32_t tilesX, uint32_t tilesY, uint32_t slicesZ, bool updateOnlyCurrentFrame)
{
	try
	{
		size_t clusters = static_cast<size_t>(tilesX) * static_cast<size_t>(tilesY) * static_cast<size_t>(slicesZ);
		size_t indices  = clusters * static_cast<size_t>(MAX_LIGHTS_PER_TILE);

		// Range of frames to touch this call
		size_t beginFrame = 0;
		size_t endFrame   = MAX_FRAMES_IN_FLIGHT;
		if (updateOnlyCurrentFrame)
		{
			beginFrame = static_cast<size_t>(currentFrame);
			endFrame   = beginFrame + 1;
		}

		for (size_t i = beginFrame; i < endFrame; ++i)
		{
			auto &f         = forwardPlusPerFrame[i];
			bool  needTiles = (f.tilesCapacity < clusters) || (f.tileHeaders == nullptr);
			bool  needIdx   = (f.indicesCapacity < indices) || (f.tileLightIndices == nullptr);

			if (needTiles)
			{
				if (!(f.tileHeaders == nullptr))
				{
					f.tileHeaders = nullptr;
					f.tileHeadersAlloc.reset();
				}
				auto [buf, alloc]  = createBufferPooled(clusters * sizeof(TileHeader), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
				f.tileHeaders      = std::move(buf);
				f.tileHeadersAlloc = std::move(alloc);
				f.tilesCapacity    = clusters;
				// Initialize headers to zero so that count==0 when Forward+ is disabled or before first dispatch
				if (f.tileHeadersAlloc && f.tileHeadersAlloc->mappedPtr)
				{
					std::memset(f.tileHeadersAlloc->mappedPtr, 0, clusters * sizeof(TileHeader));
				}
			}
			if (needIdx)
			{
				if (!(f.tileLightIndices == nullptr))
				{
					f.tileLightIndices = nullptr;
					f.tileLightIndicesAlloc.reset();
				}
				auto [buf, alloc]       = createBufferPooled(indices * sizeof(uint32_t), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
				f.tileLightIndices      = std::move(buf);
				f.tileLightIndicesAlloc = std::move(alloc);
				f.indicesCapacity       = indices;
				// Initialize indices to zero to avoid stray reads
				if (f.tileLightIndicesAlloc && f.tileLightIndicesAlloc->mappedPtr)
				{
					std::memset(f.tileLightIndicesAlloc->mappedPtr, 0, indices * sizeof(uint32_t));
				}
			}
			if (f.params == nullptr)
			{
				auto [pbuf, palloc] = createBufferPooled(sizeof(glm::mat4) * 2 + sizeof(glm::vec4) * 3, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
				f.params            = std::move(pbuf);
				f.paramsAlloc       = std::move(palloc);
				f.paramsMapped      = f.paramsAlloc->mappedPtr;
			}

			// Update compute descriptor set writes for this frame (only if buffers changed or first time)
			if (!(forwardPlusPerFrame[i].computeSet == nullptr))
			{
				if (!descriptorSetsValid.load(std::memory_order_relaxed))
				{
					// Descriptor sets are being recreated; skip writes this iteration
					continue;
				}
				if (isRecordingCmd.load(std::memory_order_relaxed))
				{
					// Avoid update-after-bind while a command buffer is recording
					continue;
				}
				// Only update descriptors if we resized or created any buffer this iteration
				if (needTiles || needIdx || f.params != nullptr)
				{
					// Build writes conditionally to avoid dereferencing uninitialized light buffers
					std::vector<vk::WriteDescriptorSet> writes;

					// Binding 0: lights SSBO (only if available)
					bool                     haveLightBuffer = (i < lightStorageBuffers.size()) && !(lightStorageBuffers[i].buffer == nullptr);
					vk::DescriptorBufferInfo lightsInfo{};
					if (haveLightBuffer)
					{
						lightsInfo = vk::DescriptorBufferInfo{.buffer = *lightStorageBuffers[i].buffer, .offset = 0, .range = VK_WHOLE_SIZE};
						writes.push_back(vk::WriteDescriptorSet{.dstSet = *forwardPlusPerFrame[i].computeSet, .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &lightsInfo});
					}

					// Binding 1: tile headers
					vk::DescriptorBufferInfo headersInfo{.buffer = *f.tileHeaders, .offset = 0, .range = VK_WHOLE_SIZE};
					writes.push_back(vk::WriteDescriptorSet{.dstSet = *forwardPlusPerFrame[i].computeSet, .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &headersInfo});

					// Binding 2: tile indices
					vk::DescriptorBufferInfo indicesInfo{.buffer = *f.tileLightIndices, .offset = 0, .range = VK_WHOLE_SIZE};
					writes.push_back(vk::WriteDescriptorSet{.dstSet = *forwardPlusPerFrame[i].computeSet, .dstBinding = 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &indicesInfo});

					// Binding 3: params UBO
					vk::DescriptorBufferInfo paramsInfo{.buffer = *f.params, .offset = 0, .range = VK_WHOLE_SIZE};
					writes.push_back(vk::WriteDescriptorSet{.dstSet = *forwardPlusPerFrame[i].computeSet, .dstBinding = 3, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &paramsInfo});

					if (!writes.empty())
					{
						std::lock_guard<std::mutex> lk(descriptorMutex);
						device.updateDescriptorSets(writes, {});
					}
				}
			}
		}

		// Update PBR descriptor sets to bind new tile buffers for forward shading.
		// Avoid updating sets that may be in use by in-flight command buffers.
		// If updateOnlyCurrentFrame=true, only update the current frame's sets (safe point after fence wait).
		try
		{
			// Only update PBR descriptor sets for bindings 7/8 in two situations:
			//  - When called in initialization/device-idle paths (updateOnlyCurrentFrame=false), or
			//  - When this call resulted in (re)creating the buffers for the current frame
			size_t beginFrameSets = 0;
			size_t endFrameSets   = forwardPlusPerFrame.size();
			if (updateOnlyCurrentFrame)
			{
				beginFrameSets = static_cast<size_t>(currentFrame);
				endFrameSets   = beginFrameSets + 1;
			}

			for (auto &kv : entityResources)
			{
				auto &resources = kv.second;
				if (resources.pbrDescriptorSets.empty())
					continue;
				for (size_t i = beginFrameSets; i < endFrameSets && i < resources.pbrDescriptorSets.size() && i < forwardPlusPerFrame.size(); ++i)
				{
					if (!descriptorSetsValid.load(std::memory_order_relaxed))
						continue;
					if (isRecordingCmd.load(std::memory_order_relaxed))
						continue;
					if (!(*resources.pbrDescriptorSets[i]))
						continue;
					auto &f = forwardPlusPerFrame[i];
					if ((f.tileHeaders == nullptr) || (f.tileLightIndices == nullptr))
						continue;
					vk::DescriptorBufferInfo              headersInfo{.buffer = *f.tileHeaders, .offset = 0, .range = VK_WHOLE_SIZE};
					vk::DescriptorBufferInfo              indicesInfo{.buffer = *f.tileLightIndices, .offset = 0, .range = VK_WHOLE_SIZE};
					std::array<vk::WriteDescriptorSet, 2> writes = {
					    vk::WriteDescriptorSet{.dstSet = *resources.pbrDescriptorSets[i], .dstBinding = 7, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &headersInfo},
					    vk::WriteDescriptorSet{.dstSet = *resources.pbrDescriptorSets[i], .dstBinding = 8, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &indicesInfo}};
					{
						std::lock_guard<std::mutex> lk(descriptorMutex);
						device.updateDescriptorSets(writes, {});
					}
				}
			}
		}
		catch (...)
		{}

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create/resize Forward+ buffers: " << e.what() << std::endl;
		return false;
	}
}

void Renderer::updateForwardPlusParams(uint32_t frameIndex, const glm::mat4 &view, const glm::mat4 &proj, uint32_t lightCount, uint32_t tilesX, uint32_t tilesY, uint32_t slicesZ, float nearZ, float farZ)
{
	if (frameIndex >= forwardPlusPerFrame.size())
		return;
	auto &f = forwardPlusPerFrame[frameIndex];
	if (!f.paramsMapped)
		return;

	// Pack: [view][proj][screen xy, tile xy][lightCount, maxPerTile, tilesX, tilesY][near, far, slicesZ, 0]
	struct ParamsCPU
	{
		glm::mat4  view;
		glm::mat4  proj;
		glm::vec4  screenTile;        // x=width,y=height,z=tileX,w=tileY
		glm::uvec4 counts;            // x=lightCount,y=maxPerTile,z=tilesX,w=tilesY
		glm::vec4  zParams;           // x=nearZ,y=farZ,z=slicesZ,w=0
	};

	ParamsCPU p{};
	p.view       = view;
	p.proj       = proj;
	p.screenTile = glm::vec4(static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), static_cast<float>(forwardPlusTileSizeX), static_cast<float>(forwardPlusTileSizeY));
	p.counts     = glm::uvec4(lightCount, MAX_LIGHTS_PER_TILE, tilesX, tilesY);
	p.zParams    = glm::vec4(nearZ, farZ, static_cast<float>(slicesZ), 0.0f);

	std::memcpy(f.paramsAlloc->mappedPtr, &p, sizeof(ParamsCPU));
}

void Renderer::dispatchForwardPlus(vk::raii::CommandBuffer &cmd, uint32_t tilesX, uint32_t tilesY, uint32_t slicesZ)
{
	if (forwardPlusPipeline == nullptr)
		return;
	if (currentFrame >= forwardPlusPerFrame.size())
		return;
	auto &f = forwardPlusPerFrame[currentFrame];
	if (f.computeSet == nullptr)
		return;

	// Ensure a valid lights buffer is bound; otherwise skip compute this frame
	bool haveLightBuffer = (currentFrame < lightStorageBuffers.size()) && !(lightStorageBuffers[currentFrame].buffer == nullptr);
	if (!haveLightBuffer)
		return;

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *forwardPlusPipeline);
	vk::DescriptorSet set = *f.computeSet;
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *forwardPlusPipelineLayout, 0, set, {});
	// One invocation per cluster (X,Y by workgroup grid, Z as third dimension)
	cmd.dispatch(tilesX, tilesY, slicesZ);
	// Make tilelist writes visible to fragment shader (Sync2)
	vk::MemoryBarrier2 memBarrier2{
	    .srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
	    .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
	    .dstStageMask  = vk::PipelineStageFlagBits2::eFragmentShader,
	    .dstAccessMask = vk::AccessFlagBits2::eShaderRead};
	vk::DependencyInfo depInfoComputeToFrag{.memoryBarrierCount = 1, .pMemoryBarriers = &memBarrier2};
	cmd.pipelineBarrier2(depInfoComputeToFrag);
}

// Ensure compute descriptor binding 0 (lights SSBO) is bound for the given frame.
void Renderer::refreshForwardPlusComputeLightsBindingForFrame(uint32_t frameIndex)
{
	try
	{
		if (frameIndex >= forwardPlusPerFrame.size())
			return;
		if (forwardPlusPerFrame[frameIndex].computeSet == nullptr)
			return;
		if (frameIndex >= lightStorageBuffers.size())
			return;
		if (lightStorageBuffers[frameIndex].buffer == nullptr)
			return;

		// Updating descriptor sets during recording causes validation errors:
		// "commandBuffer must be in the recording state" and invalidates the command buffer.
		// These descriptor sets are already initialized earlier at the safe point (line 1059),
		// so this redundant update during recording is unnecessary and harmful.
		if (isRecordingCmd.load(std::memory_order_relaxed))
		{
			return;        // Skip update, descriptor is already valid from earlier initialization
		}

		vk::DescriptorBufferInfo lightsInfo{.buffer = *lightStorageBuffers[frameIndex].buffer, .offset = 0, .range = VK_WHOLE_SIZE};
		vk::WriteDescriptorSet   write{.dstSet = *forwardPlusPerFrame[frameIndex].computeSet, .dstBinding = 0, .dstArrayElement = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &lightsInfo};
		{
			std::lock_guard<std::mutex> lk(descriptorMutex);
			device.updateDescriptorSets(write, {});
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to refresh Forward+ compute lights binding for frame " << frameIndex << ": " << e.what() << std::endl;
	}
}

// Create compute command pool
bool Renderer::createComputeCommandPool()
{
	try
	{
		vk::CommandPoolCreateInfo poolInfo{
		    .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		    .queueFamilyIndex = queueFamilyIndices.computeFamily.value()};

		computeCommandPool = vk::raii::CommandPool(device, poolInfo);
		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create compute command pool: " << e.what() << std::endl;
		return false;
	}
}

// Dispatch compute shader
vk::raii::Fence Renderer::DispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ,
                                          vk::Buffer inputBuffer, vk::Buffer outputBuffer,
                                          vk::Buffer hrtfBuffer, vk::Buffer paramsBuffer)
{
	try
	{
		// Create fence for synchronization
		vk::FenceCreateInfo fenceInfo{};
		vk::raii::Fence     computeFence(device, fenceInfo);

		// Create descriptor sets
		vk::DescriptorSetAllocateInfo allocInfo{
		    .descriptorPool     = *computeDescriptorPool,
		    .descriptorSetCount = 1,
		    .pSetLayouts        = &*computeDescriptorSetLayout};

		{
			std::lock_guard<std::mutex> lk(descriptorMutex);
			computeDescriptorSets = device.allocateDescriptorSets(allocInfo);
		}

		// Update descriptor sets
		vk::DescriptorBufferInfo inputBufferInfo{
		    .buffer = inputBuffer,
		    .offset = 0,
		    .range  = VK_WHOLE_SIZE};

		vk::DescriptorBufferInfo outputBufferInfo{
		    .buffer = outputBuffer,
		    .offset = 0,
		    .range  = VK_WHOLE_SIZE};

		vk::DescriptorBufferInfo hrtfBufferInfo{
		    .buffer = hrtfBuffer,
		    .offset = 0,
		    .range  = VK_WHOLE_SIZE};

		vk::DescriptorBufferInfo paramsBufferInfo{
		    .buffer = paramsBuffer,
		    .offset = 0,
		    .range  = VK_WHOLE_SIZE};

		std::array<vk::WriteDescriptorSet, 4> descriptorWrites = {
		    vk::WriteDescriptorSet{
		        .dstSet          = computeDescriptorSets[0],
		        .dstBinding      = 0,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType  = vk::DescriptorType::eStorageBuffer,
		        .pBufferInfo     = &inputBufferInfo},
		    vk::WriteDescriptorSet{
		        .dstSet          = computeDescriptorSets[0],
		        .dstBinding      = 1,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType  = vk::DescriptorType::eStorageBuffer,
		        .pBufferInfo     = &outputBufferInfo},
		    vk::WriteDescriptorSet{
		        .dstSet          = computeDescriptorSets[0],
		        .dstBinding      = 2,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType  = vk::DescriptorType::eStorageBuffer,
		        .pBufferInfo     = &hrtfBufferInfo},
		    vk::WriteDescriptorSet{
		        .dstSet          = computeDescriptorSets[0],
		        .dstBinding      = 3,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType  = vk::DescriptorType::eUniformBuffer,
		        .pBufferInfo     = &paramsBufferInfo}};

		{
			std::lock_guard<std::mutex> lk(descriptorMutex);
			device.updateDescriptorSets(descriptorWrites, {});
		}

		// Create command buffer using dedicated compute command pool
		vk::CommandBufferAllocateInfo cmdAllocInfo{
		    .commandPool        = *computeCommandPool,
		    .level              = vk::CommandBufferLevel::ePrimary,
		    .commandBufferCount = 1};

		auto commandBuffers = device.allocateCommandBuffers(cmdAllocInfo);
		// Use RAII wrapper temporarily for recording to preserve dispatch loader
		vk::raii::CommandBuffer commandBufferRaii = std::move(commandBuffers[0]);

		// Begin command buffer
		vk::CommandBufferBeginInfo beginInfo{
		    .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

		commandBufferRaii.begin(beginInfo);

		// Bind compute pipeline
		commandBufferRaii.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);

		// Bind descriptor sets - properly convert RAII descriptor set to regular descriptor set
		std::vector<vk::DescriptorSet> descriptorSetsToBindRaw;
		descriptorSetsToBindRaw.reserve(1);
		descriptorSetsToBindRaw.push_back(*computeDescriptorSets[0]);
		commandBufferRaii.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipelineLayout, 0, descriptorSetsToBindRaw, {});

		// Dispatch compute shader
		commandBufferRaii.dispatch(groupCountX, groupCountY, groupCountZ);

		// End command buffer
		commandBufferRaii.end();

		// Extract raw command buffer for submission and release RAII ownership
		// This prevents premature destruction while preserving the recorded commands
		vk::CommandBuffer rawCommandBuffer = *commandBufferRaii;
		commandBufferRaii.release();        // Release RAII ownership to prevent destruction

		// Submit command buffer with fence for synchronization
		vk::SubmitInfo submitInfo{
		    .commandBufferCount = 1,
		    .pCommandBuffers    = &rawCommandBuffer};

		// Use mutex to ensure thread-safe access to compute queue
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			computeQueue.submit(submitInfo, *computeFence);
		}

		// Return fence for non-blocking synchronization
		return computeFence;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to dispatch compute shader: " << e.what() << std::endl;
		// Return a null fence on error
		vk::FenceCreateInfo fenceInfo{};
		return {device, fenceInfo};
	}
}
