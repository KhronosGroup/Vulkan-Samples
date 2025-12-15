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
#include "imgui/imgui.h"
#include "imgui_system.h"
#include "model_loader.h"
#include "renderer.h"
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <map>
#include <ranges>
#include <stdexcept>

// ===================== Culling helpers implementation =====================

Renderer::FrustumPlanes Renderer::extractFrustumPlanes(const glm::mat4 &vp)
{
	// Work in row-major form for standard plane extraction by transposing GLM's column-major matrix
	glm::mat4     m = glm::transpose(vp);
	FrustumPlanes fp{};
	// Left   : m[3] + m[0]
	fp.planes[0] = m[3] + m[0];
	// Right  : m[3] - m[0]
	fp.planes[1] = m[3] - m[0];
	// Bottom : m[3] + m[1]
	fp.planes[2] = m[3] + m[1];
	// Top    : m[3] - m[1]
	fp.planes[3] = m[3] - m[1];
	// Near   : m[3] + m[2]
	fp.planes[4] = m[3] + m[2];
	// Far    : m[3] - m[2]
	fp.planes[5] = m[3] - m[2];

	// Normalize planes
	for (auto &p : fp.planes)
	{
		glm::vec3 n(p.x, p.y, p.z);
		float     len = glm::length(n);
		if (len > 0.0f)
		{
			p /= len;
		}
	}
	return fp;
}

void Renderer::transformAABB(const glm::mat4 &M,
                             const glm::vec3 &localMin,
                             const glm::vec3 &localMax,
                             glm::vec3       &outMin,
                             glm::vec3       &outMax)
{
	// OBB (from model) to world AABB using center/extents and absolute 3x3
	const glm::vec3 c = 0.5f * (localMin + localMax);
	const glm::vec3 e = 0.5f * (localMax - localMin);

	const glm::vec3 worldCenter = glm::vec3(M * glm::vec4(c, 1.0f));
	// Upper-left 3x3
	const glm::mat3 A            = glm::mat3(M);
	const glm::mat3 AbsA         = glm::mat3(glm::abs(A[0]), glm::abs(A[1]), glm::abs(A[2]));
	const glm::vec3 worldExtents = AbsA * e;        // component-wise combination

	outMin = worldCenter - worldExtents;
	outMax = worldCenter + worldExtents;
}

bool Renderer::aabbIntersectsFrustum(const glm::vec3     &worldMin,
                                     const glm::vec3     &worldMax,
                                     const FrustumPlanes &frustum)
{
	// Use the p-vertex test against each plane; if outside any plane → culled
	for (const auto &p : frustum.planes)
	{
		const glm::vec3 n(p.x, p.y, p.z);
		// Choose positive vertex
		glm::vec3 v{
		    n.x >= 0.0f ? worldMax.x : worldMin.x,
		    n.y >= 0.0f ? worldMax.y : worldMin.y,
		    n.z >= 0.0f ? worldMax.z : worldMin.z};
		if (glm::dot(n, v) + p.w < 0.0f)
		{
			return false;        // completely outside
		}
	}
	return true;
}

// This file contains rendering-related methods from the Renderer class

// Create swap chain
bool Renderer::createSwapChain()
{
	try
	{
		// Query swap chain support
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		// Choose swap surface format, present mode, and extent
		vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		vk::PresentModeKHR   presentMode   = chooseSwapPresentMode(swapChainSupport.presentModes);
		vk::Extent2D         extent        = chooseSwapExtent(swapChainSupport.capabilities);

		// Choose image count
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		// Create swap chain info
		vk::SwapchainCreateInfoKHR createInfo{
		    .surface          = *surface,
		    .minImageCount    = imageCount,
		    .imageFormat      = surfaceFormat.format,
		    .imageColorSpace  = surfaceFormat.colorSpace,
		    .imageExtent      = extent,
		    .imageArrayLayers = 1,
		    .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
		    .preTransform     = swapChainSupport.capabilities.currentTransform,
		    .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		    .presentMode      = presentMode,
		    .clipped          = VK_TRUE,
		    .oldSwapchain     = nullptr};

		// Find queue families
		QueueFamilyIndices indices                 = findQueueFamilies(physicalDevice);
		uint32_t           queueFamilyIndicesLoc[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		// Set sharing mode
		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices   = queueFamilyIndicesLoc;
		}
		else
		{
			createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices   = nullptr;
		}

		// Create swap chain
		swapChain = vk::raii::SwapchainKHR(device, createInfo);

		// Get swap chain images
		swapChainImages = swapChain.getImages();

		// Swapchain images start in UNDEFINED layout; track per-image layout for correct barriers.
		swapChainImageLayouts.assign(swapChainImages.size(), vk::ImageLayout::eUndefined);

		// Store swap chain format and extent
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent      = extent;

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create swap chain: " << e.what() << std::endl;
		return false;
	}
}

// ===================== Planar reflections resources =====================
bool Renderer::createReflectionResources(uint32_t width, uint32_t height)
{
	try
	{
		destroyReflectionResources();
		reflections.clear();
		reflections.resize(MAX_FRAMES_IN_FLIGHT);
		reflectionVPs.clear();
		reflectionVPs.resize(MAX_FRAMES_IN_FLIGHT, glm::mat4(1.0f));
		sampleReflectionVP = glm::mat4(1.0f);

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			auto &rt  = reflections[i];
			rt.width  = width;
			rt.height = height;

			// Color RT: use swapchain format to match existing PBR pipeline rendering formats
			vk::Format colorFmt         = swapChainImageFormat;
			auto [colorImg, colorAlloc] = createImagePooled(
			    width,
			    height,
			    colorFmt,
			    vk::ImageTiling::eOptimal,
			    // Allow sampling in glass and blitting to swapchain for diagnostics
			    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
			    vk::MemoryPropertyFlagBits::eDeviceLocal,
			    /*mipLevels*/ 1,
			    vk::SharingMode::eExclusive,
			    {});
			rt.color      = std::move(colorImg);
			rt.colorAlloc = std::move(colorAlloc);
			rt.colorView  = createImageView(rt.color, colorFmt, vk::ImageAspectFlagBits::eColor, 1);
			// Simple sampler for sampling reflection texture (no mips)
			vk::SamplerCreateInfo sampInfo{.magFilter = vk::Filter::eLinear, .minFilter = vk::Filter::eLinear, .mipmapMode = vk::SamplerMipmapMode::eNearest, .addressModeU = vk::SamplerAddressMode::eClampToEdge, .addressModeV = vk::SamplerAddressMode::eClampToEdge, .addressModeW = vk::SamplerAddressMode::eClampToEdge, .minLod = 0.0f, .maxLod = 0.0f};
			rt.colorSampler = vk::raii::Sampler(device, sampInfo);

			// Depth RT
			vk::Format depthFmt         = findDepthFormat();
			auto [depthImg, depthAlloc] = createImagePooled(
			    width,
			    height,
			    depthFmt,
			    vk::ImageTiling::eOptimal,
			    vk::ImageUsageFlagBits::eDepthStencilAttachment,
			    vk::MemoryPropertyFlagBits::eDeviceLocal,
			    /*mipLevels*/ 1,
			    vk::SharingMode::eExclusive,
			    {});
			rt.depth      = std::move(depthImg);
			rt.depthAlloc = std::move(depthAlloc);
			rt.depthView  = createImageView(rt.depth, depthFmt, vk::ImageAspectFlagBits::eDepth, 1);
		}

		// One-time initialization: transition all per-frame reflection color images
		// from UNDEFINED to SHADER_READ_ONLY_OPTIMAL so that the first frame can
		// legally sample the "previous" frame's image.
		if (!reflections.empty())
		{
			vk::CommandPoolCreateInfo     poolInfo{.flags            = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			                                       .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()};
			vk::raii::CommandPool         tempPool(device, poolInfo);
			vk::CommandBufferAllocateInfo allocInfo{.commandPool = *tempPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
			vk::raii::CommandBuffers      cbs(device, allocInfo);
			vk::raii::CommandBuffer      &cb = cbs[0];
			cb.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

			std::vector<vk::ImageMemoryBarrier2> barriers;
			barriers.reserve(reflections.size());
			for (auto &rt : reflections)
			{
				if (*rt.color)
				{
					barriers.push_back(vk::ImageMemoryBarrier2{
					    .srcStageMask        = vk::PipelineStageFlagBits2::eTopOfPipe,
					    .srcAccessMask       = vk::AccessFlagBits2::eNone,
					    .dstStageMask        = vk::PipelineStageFlagBits2::eFragmentShader,
					    .dstAccessMask       = vk::AccessFlagBits2::eShaderRead,
					    .oldLayout           = vk::ImageLayout::eUndefined,
					    .newLayout           = vk::ImageLayout::eShaderReadOnlyOptimal,
					    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					    .image               = *rt.color,
					    .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
				}
			}
			if (!barriers.empty())
			{
				vk::DependencyInfo depInfo{.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()), .pImageMemoryBarriers = barriers.data()};
				cb.pipelineBarrier2(depInfo);
			}
			cb.end();
			vk::SubmitInfo  submit{.commandBufferCount = 1, .pCommandBuffers = &*cb};
			vk::raii::Fence fence(device, vk::FenceCreateInfo{});
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				graphicsQueue.submit(submit, *fence);
			}
			(void) device.waitForFences({*fence}, VK_TRUE, UINT64_MAX);
		}

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create reflection resources: " << e.what() << std::endl;
		destroyReflectionResources();
		return false;
	}
}

void Renderer::destroyReflectionResources()
{
	for (auto &rt : reflections)
	{
		rt.colorSampler = nullptr;
		rt.colorView    = nullptr;
		rt.colorAlloc   = nullptr;
		rt.color        = nullptr;
		rt.depthView    = nullptr;
		rt.depthAlloc   = nullptr;
		rt.depth        = nullptr;
		rt.width = rt.height = 0;
	}
}

void Renderer::renderReflectionPass(vk::raii::CommandBuffer                    &cmd,
                                    const glm::vec4                            &planeWS,
                                    CameraComponent                            *camera,
                                    const std::vector<std::unique_ptr<Entity>> &entities)
{
	// Initial scaffolding: clear the reflection RT; drawing the mirrored scene will be added next.
	if (reflections.empty())
		return;
	auto &rt = reflections[currentFrame];
	if (rt.width == 0 || rt.height == 0 || rt.colorView == nullptr || rt.depthView == nullptr)
		return;

	// Transition reflection color to COLOR_ATTACHMENT_OPTIMAL (Sync2)
	vk::ImageMemoryBarrier2 toColor2{
	    .srcStageMask        = vk::PipelineStageFlagBits2::eTopOfPipe,
	    .srcAccessMask       = {},
	    .dstStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
	    .dstAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite,
	    .oldLayout           = vk::ImageLayout::eShaderReadOnlyOptimal,
	    .newLayout           = vk::ImageLayout::eColorAttachmentOptimal,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image               = *rt.color,
	    .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
	// Transition reflection depth to DEPTH_STENCIL_ATTACHMENT_OPTIMAL (Sync2)
	vk::ImageMemoryBarrier2 toDepth2{
	    .srcStageMask        = vk::PipelineStageFlagBits2::eTopOfPipe,
	    .srcAccessMask       = {},
	    .dstStageMask        = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
	    .dstAccessMask       = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
	    .oldLayout           = vk::ImageLayout::eUndefined,
	    .newLayout           = vk::ImageLayout::eDepthAttachmentOptimal,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image               = *rt.depth,
	    .subresourceRange    = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}};
	std::array<vk::ImageMemoryBarrier2, 2> preBarriers{toColor2, toDepth2};
	vk::DependencyInfo                     depInfoToColor{.imageMemoryBarrierCount = static_cast<uint32_t>(preBarriers.size()), .pImageMemoryBarriers = preBarriers.data()};
	cmd.pipelineBarrier2(depInfoToColor);

	vk::RenderingAttachmentInfo colorAtt{
	    .imageView   = *rt.colorView,
	    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
	    .loadOp      = vk::AttachmentLoadOp::eClear,
	    .storeOp     = vk::AttachmentStoreOp::eStore,
	    // Clear to black so scene content dominates reflections
	    .clearValue = vk::ClearValue{vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}}};
	vk::RenderingAttachmentInfo depthAtt{
	    .imageView   = *rt.depthView,
	    .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
	    .loadOp      = vk::AttachmentLoadOp::eClear,
	    .storeOp     = vk::AttachmentStoreOp::eDontCare,
	    .clearValue  = vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}}};
	vk::RenderingInfo rinfo{
	    .renderArea           = vk::Rect2D({0, 0}, {rt.width, rt.height}),
	    .layerCount           = 1,
	    .colorAttachmentCount = 1,
	    .pColorAttachments    = &colorAtt,
	    .pDepthAttachment     = &depthAtt};
	cmd.beginRendering(rinfo);
	// Compute mirrored view matrix about planeWS (default Y=0 plane)
	glm::mat4 reflectM(1.0f);
	// For Y=0 plane, reflection is simply flip Y
	if (glm::length(glm::vec3(planeWS.x, planeWS.y, planeWS.z)) > 0.5f && fabsf(planeWS.y - 1.0f) < 1e-3f && fabsf(planeWS.x) < 1e-3f && fabsf(planeWS.z) < 1e-3f)
	{
		reflectM[1][1] = -1.0f;
	}
	else
	{
		// General plane reflection matrix R = I - 2*n*n^T for normalized plane; ignore translation for now
		glm::vec3 n = glm::normalize(glm::vec3(planeWS));
		glm::mat3 R = glm::mat3(1.0f) - 2.0f * glm::outerProduct(n, n);
		reflectM    = glm::mat4(R);
	}

	glm::mat4 viewReflected = camera ? (camera->GetViewMatrix() * reflectM) : reflectM;
	glm::mat4 projReflected = camera ? camera->GetProjectionMatrix() : glm::mat4(1.0f);
	projReflected[1][1] *= -1.0f;
	currentReflectionVP    = projReflected * viewReflected;
	currentReflectionPlane = planeWS;
	if (currentFrame < reflectionVPs.size())
	{
		reflectionVPs[currentFrame] = currentReflectionVP;
	}

	// Set viewport/scissor to reflection RT size
	vk::Viewport rv(0.0f, 0.0f, static_cast<float>(rt.width), static_cast<float>(rt.height), 0.0f, 1.0f);
	cmd.setViewport(0, rv);
	vk::Rect2D rs({0, 0}, {rt.width, rt.height});
	cmd.setScissor(0, rs);

	// Draw opaque entities with mirrored view
	// Use reflection-specific pipeline (cull none) to avoid mirrored winding issues.
	if (pbrReflectionGraphicsPipeline != nullptr)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pbrReflectionGraphicsPipeline);
	}
	else if (pbrGraphicsPipeline != nullptr)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pbrGraphicsPipeline);
	}

	// Render all entities with meshes (skip transparency; glass revisit later)
	for (const auto &uptr : entities)
	{
		Entity *entity = uptr.get();
		if (!entity || !entity->IsActive())
			continue;
		auto meshComponent = entity->GetComponent<MeshComponent>();
		if (!meshComponent)
			continue;

		auto entityIt = entityResources.find(entity);
		if (entityIt == entityResources.end())
			continue;

		auto meshIt = meshResources.find(meshComponent);
		if (meshIt == meshResources.end())
			continue;

		// Bind geometry
		std::array<vk::Buffer, 2>     buffers = {*meshIt->second.vertexBuffer, *entityIt->second.instanceBuffer};
		std::array<vk::DeviceSize, 2> offsets = {0, 0};
		cmd.bindVertexBuffers(0, buffers, offsets);
		cmd.bindIndexBuffer(*meshIt->second.indexBuffer, 0, vk::IndexType::eUint32);

		// Populate UBO with mirrored view + clip plane and reflection flags
		UniformBufferObject ubo{};
		if (auto tc = entity->GetComponent<TransformComponent>())
			ubo.model = tc->GetModelMatrix();
		else
			ubo.model = glm::mat4(1.0f);
		ubo.view              = viewReflected;
		ubo.proj              = projReflected;
		ubo.camPos            = glm::vec4(camera ? camera->GetPosition() : glm::vec3(0), 1.0f);
		ubo.reflectionPass    = 1;
		ubo.reflectionEnabled = 0;
		ubo.reflectionVP      = currentReflectionVP;
		ubo.clipPlaneWS       = planeWS;
		updateUniformBufferInternal(currentFrame, entity, const_cast<CameraComponent *>(camera), ubo);

		// Bind descriptor set (PBR)
		auto &descSets = entityIt->second.pbrDescriptorSets;
		if (descSets.empty() || currentFrame >= descSets.size())
			continue;
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pbrPipelineLayout, 0, {*descSets[currentFrame]}, {});

		// Push material properties for reflection pass (use textures)
		MaterialProperties mp{};
		// Neutral defaults; textures from descriptor set will provide actual albedo/normal/etc.
		mp.baseColorFactor = glm::vec4(1.0f);
		mp.metallicFactor  = 0.0f;
		mp.roughnessFactor = 0.8f;
		// Transmission suppressed during reflection pass via UBO (reflectionPass=1)
		mp.transmissionFactor = 0.0f;
		pushMaterialProperties(*cmd, mp);

		// Issue draw
		uint32_t instanceCount = std::max(1u, static_cast<uint32_t>(meshComponent->GetInstanceCount()));
		cmd.drawIndexed(meshIt->second.indexCount, instanceCount, 0, 0, 0);
	}

	cmd.endRendering();

	// Transition reflection color to SHADER_READ_ONLY for sampling in main pass (Sync2)
	vk::ImageMemoryBarrier2 toSample2{
	    .srcStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
	    .srcAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite,
	    .dstStageMask        = vk::PipelineStageFlagBits2::eFragmentShader,
	    .dstAccessMask       = vk::AccessFlagBits2::eShaderRead,
	    .oldLayout           = vk::ImageLayout::eColorAttachmentOptimal,
	    .newLayout           = vk::ImageLayout::eShaderReadOnlyOptimal,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image               = *rt.color,
	    .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
	vk::DependencyInfo depInfoToSample{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &toSample2};
	cmd.pipelineBarrier2(depInfoToSample);
}

// Create image views
bool Renderer::createImageViews()
{
	try
	{
		opaqueSceneColorImage.clear();
		opaqueSceneColorImageView.clear();
		opaqueSceneColorSampler.clear();
		// Resize image views vector
		swapChainImageViews.clear();
		swapChainImageViews.reserve(swapChainImages.size());

		// Create image view for each swap chain image
		for (const auto &image : swapChainImages)
		{
			// Create image view info
			vk::ImageViewCreateInfo createInfo{
			    .image      = image,
			    .viewType   = vk::ImageViewType::e2D,
			    .format     = swapChainImageFormat,
			    .components = {
			        .r = vk::ComponentSwizzle::eIdentity,
			        .g = vk::ComponentSwizzle::eIdentity,
			        .b = vk::ComponentSwizzle::eIdentity,
			        .a = vk::ComponentSwizzle::eIdentity},
			    .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

			// Create image view
			swapChainImageViews.emplace_back(device, createInfo);
		}

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create image views: " << e.what() << std::endl;
		return false;
	}
}

// Setup dynamic rendering
bool Renderer::setupDynamicRendering()
{
	try
	{
		// Create color attachment
		colorAttachments = {
		    vk::RenderingAttachmentInfo{
		        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		        .loadOp      = vk::AttachmentLoadOp::eClear,
		        .storeOp     = vk::AttachmentStoreOp::eStore,
		        .clearValue  = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f})}};

		// Create depth attachment
		depthAttachment = vk::RenderingAttachmentInfo{
		    .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
		    .loadOp      = vk::AttachmentLoadOp::eClear,
		    .storeOp     = vk::AttachmentStoreOp::eStore,
		    .clearValue  = vk::ClearDepthStencilValue(1.0f, 0)};

		// Create rendering info
		renderingInfo = vk::RenderingInfo{
		    .renderArea           = vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent),
		    .layerCount           = 1,
		    .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
		    .pColorAttachments    = colorAttachments.data(),
		    .pDepthAttachment     = &depthAttachment};

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to setup dynamic rendering: " << e.what() << std::endl;
		return false;
	}
}

// Create command pool
bool Renderer::createCommandPool()
{
	try
	{
		// Find queue families
		QueueFamilyIndices queueFamilyIndicesLoc = findQueueFamilies(physicalDevice);

		// Create command pool info
		vk::CommandPoolCreateInfo poolInfo{
		    .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		    .queueFamilyIndex = queueFamilyIndicesLoc.graphicsFamily.value()};

		// Create command pool
		commandPool = vk::raii::CommandPool(device, poolInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create command pool: " << e.what() << std::endl;
		return false;
	}
}

// Create command buffers
bool Renderer::createCommandBuffers()
{
	try
	{
		// Resize command buffers vector
		commandBuffers.clear();
		commandBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

		// Create command buffer allocation info
		vk::CommandBufferAllocateInfo allocInfo{
		    .commandPool        = *commandPool,
		    .level              = vk::CommandBufferLevel::ePrimary,
		    .commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)};

		// Allocate command buffers
		commandBuffers = vk::raii::CommandBuffers(device, allocInfo);

		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create command buffers: " << e.what() << std::endl;
		return false;
	}
}

// Create sync objects
bool Renderer::createSyncObjects()
{
	try
	{
		// Resize semaphores and fences vectors
		imageAvailableSemaphores.clear();
		renderFinishedSemaphores.clear();
		inFlightFences.clear();

		const auto semaphoreCount = static_cast<uint32_t>(swapChainImages.size());
		imageAvailableSemaphores.reserve(semaphoreCount);
		renderFinishedSemaphores.reserve(semaphoreCount);

		// Fences remain per frame-in-flight for CPU-GPU synchronization
		inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

		// Create semaphore and fence info
		vk::SemaphoreCreateInfo semaphoreInfo{};
		vk::FenceCreateInfo     fenceInfo{
		        .flags = vk::FenceCreateFlagBits::eSignaled};

		// Create semaphores per swapchain image (indexed by imageIndex from acquireNextImage)
		for (uint32_t i = 0; i < semaphoreCount; i++)
		{
			imageAvailableSemaphores.emplace_back(device, semaphoreInfo);
			renderFinishedSemaphores.emplace_back(device, semaphoreInfo);
		}

		// Create fences per frame-in-flight (indexed by currentFrame for CPU-GPU pacing)
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			inFlightFences.emplace_back(device, fenceInfo);
		}

		// Ensure uploads timeline semaphore exists (created early in createLogicalDevice)
		// No action needed here unless reinitializing after swapchain recreation.
		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to create sync objects: " << e.what() << std::endl;
		return false;
	}
}

// Clean up swap chain
void Renderer::cleanupSwapChain()
{
	// Clean up depth resources
	depthImageView       = nullptr;
	depthImage           = nullptr;
	depthImageAllocation = nullptr;

	// Clean up swap chain image views
	swapChainImageViews.clear();

	// Note: Keep descriptor pool alive here to ensure descriptor sets remain valid during swapchain recreation.
	// descriptorPool is preserved; it will be managed during full renderer teardown.

	// Destroy reflection render targets if present
	destroyReflectionResources();

	// Clean up pipelines
	graphicsPipeline    = nullptr;
	pbrGraphicsPipeline = nullptr;
	lightingPipeline    = nullptr;

	// Clean up pipeline layouts
	pipelineLayout         = nullptr;
	pbrPipelineLayout      = nullptr;
	lightingPipelineLayout = nullptr;

	// Clean up sync objects (they need to be recreated with new swap chain image count)
	imageAvailableSemaphores.clear();
	renderFinishedSemaphores.clear();
	inFlightFences.clear();

	// Clean up swap chain
	swapChain = nullptr;
}

// Recreate swap chain
void Renderer::recreateSwapChain()
{
	// Prevent background uploads worker from mutating descriptors while we rebuild
	StopUploadsWorker();

	// Block descriptor writes while we rebuild swapchain and descriptor pools
	descriptorSetsValid.store(false, std::memory_order_relaxed);
	{
		// Drop any deferred descriptor updates that target old descriptor sets
		std::lock_guard<std::mutex> lk(pendingDescMutex);
		pendingDescOps.clear();
		descriptorRefreshPending.store(false, std::memory_order_relaxed);
	}

	// Wait for all frames in flight to complete before recreating the swap chain
	std::vector<vk::Fence> allFences;
	allFences.reserve(inFlightFences.size());
	for (const auto &fence : inFlightFences)
	{
		allFences.push_back(*fence);
	}
	if (!allFences.empty())
	{
		if (device.waitForFences(allFences, VK_TRUE, UINT64_MAX) != vk::Result::eSuccess)
		{
		}
	}

	// Wait for the device to be idle before recreating the swap chain
	// External synchronization required (VVL): serialize against queue submits/present.
	WaitIdle();

	// Clean up old swap chain resources
	cleanupSwapChain();

	// Recreate swap chain and related resources
	createSwapChain();
	createImageViews();
	setupDynamicRendering();
	createDepthResources();

	// (Re)create reflection resources if enabled
	if (enablePlanarReflections)
	{
		uint32_t rw = std::max(1u, static_cast<uint32_t>(static_cast<float>(swapChainExtent.width) * reflectionResolutionScale));
		uint32_t rh = std::max(1u, static_cast<uint32_t>(static_cast<float>(swapChainExtent.height) * reflectionResolutionScale));
		createReflectionResources(rw, rh);
	}

	// Recreate sync objects with correct sizing for new swap chain
	createSyncObjects();

	// Recreate off-screen opaque scene color and descriptor sets needed by transparent pass
	createOpaqueSceneColorResources();
	createTransparentDescriptorSets();
	createTransparentFallbackDescriptorSets();

	// Wait for all command buffers to complete before clearing resources
	for (const auto &fence : inFlightFences)
	{
		if (device.waitForFences(*fence, VK_TRUE, UINT64_MAX) != vk::Result::eSuccess)
		{
		}
	}

	// Clear all entity descriptor sets since they're now invalid (allocated from the old pool)
	{
		// Serialize descriptor frees against any other descriptor operations
		std::lock_guard<std::mutex> lk(descriptorMutex);
		for (auto &kv : entityResources)
		{
			auto &resources = kv.second;
			resources.basicDescriptorSets.clear();
			resources.pbrDescriptorSets.clear();
		}
	}

	// Clear ray query descriptor sets - they reference the old output image which will be destroyed
	// Must clear before recreating to avoid descriptor set corruption
	rayQueryDescriptorSets.clear();

	// Destroy ray query output image resources - they're sized to old swapchain dimensions
	rayQueryOutputImageView       = nullptr;
	rayQueryOutputImage           = nullptr;
	rayQueryOutputImageAllocation = nullptr;

	createGraphicsPipeline();
	createPBRPipeline();
	createLightingPipeline();
	createCompositePipeline();

	// Recreate Forward+ specific pipelines/resources and resize tile buffers for new extent
	if (useForwardPlus)
	{
		createDepthPrepassPipeline();
		uint32_t tilesX = (swapChainExtent.width + forwardPlusTileSizeX - 1) / forwardPlusTileSizeX;
		uint32_t tilesY = (swapChainExtent.height + forwardPlusTileSizeY - 1) / forwardPlusTileSizeY;
		createOrResizeForwardPlusBuffers(tilesX, tilesY, forwardPlusSlicesZ);
	}

	// Re-create command buffers to ensure fresh recording against new swapchain state
	commandBuffers.clear();
	createCommandBuffers();
	currentFrame = 0;

	// Recreate ray query resources with new swapchain dimensions
	// This must happen after descriptor pool is valid but before marking descriptor sets valid
	if (rayQueryEnabled && accelerationStructureEnabled)
	{
		if (!createRayQueryResources())
		{
			std::cerr << "Warning: Failed to recreate ray query resources after swapchain recreation\n";
		}
	}

	// Recreate descriptor sets for all entities after swapchain/pipeline rebuild
	for (const auto &kv : entityResources)
	{
		const auto &entity = kv.first;
		if (!entity)
			continue;
		auto meshComponent = entity->GetComponent<MeshComponent>();
		if (!meshComponent)
			continue;

		std::string texturePath = meshComponent->GetTexturePath();
		// Fallback for basic pipeline: use baseColor when legacy path is empty
		if (texturePath.empty())
		{
			const std::string &baseColor = meshComponent->GetBaseColorTexturePath();
			if (!baseColor.empty())
			{
				texturePath = baseColor;
			}
		}
		// Recreate basic descriptor sets (ignore failures here to avoid breaking resize)
		createDescriptorSets(entity, texturePath, false);
		// Recreate PBR descriptor sets
		createDescriptorSets(entity, texturePath, true);
	}

	// Descriptor sets are now valid again
	descriptorSetsValid.store(true, std::memory_order_relaxed);

	// Resume background uploads worker now that swapchain and descriptors are recreated
	StartUploadsWorker();
}

// Update uniform buffer
void Renderer::updateUniformBuffer(uint32_t currentImage, Entity *entity, CameraComponent *camera)
{
	// Get entity resources
	auto entityIt = entityResources.find(entity);
	if (entityIt == entityResources.end())
	{
		return;
	}

	// Get transform component
	auto transformComponent = entity->GetComponent<TransformComponent>();
	if (!transformComponent)
	{
		return;
	}

	// Create uniform buffer object
	UniformBufferObject ubo{};
	ubo.model = transformComponent->GetModelMatrix();
	ubo.view  = camera->GetViewMatrix();
	ubo.proj  = camera->GetProjectionMatrix();
	ubo.proj[1][1] *= -1;        // Flip Y for Vulkan

	// DIAGNOSTIC: Print view matrix being set for ray query
	static bool printedOnce = false;
	if (!printedOnce)
	{
		std::cout << "[CPU VIEW MATRIX] Setting for entity '" << entity->GetName() << "':\n";
		std::cout << "  [" << ubo.view[0][0] << " " << ubo.view[0][1] << " " << ubo.view[0][2] << " " << ubo.view[0][3] << "]\n";
		std::cout << "  [" << ubo.view[1][0] << " " << ubo.view[1][1] << " " << ubo.view[1][2] << " " << ubo.view[1][3] << "]\n";
		std::cout << "  [" << ubo.view[2][0] << " " << ubo.view[2][1] << " " << ubo.view[2][2] << " " << ubo.view[2][3] << "]\n";
		std::cout << "  [" << ubo.view[3][0] << " " << ubo.view[3][1] << " " << ubo.view[3][2] << " " << ubo.view[3][3] << "]\n";
		printedOnce = true;
	}

	// Continue with the rest of the uniform buffer setup
	updateUniformBufferInternal(currentImage, entity, camera, ubo);
}

// Overloaded version that accepts a custom transform matrix
void Renderer::updateUniformBuffer(uint32_t currentImage, Entity *entity, CameraComponent *camera, const glm::mat4 &customTransform)
{
	// Create the uniform buffer object with custom transform
	UniformBufferObject ubo{};
	ubo.model = customTransform;
	ubo.view  = camera->GetViewMatrix();
	ubo.proj  = camera->GetProjectionMatrix();
	ubo.proj[1][1] *= -1;        // Flip Y for Vulkan

	// Continue with the rest of the uniform buffer setup
	updateUniformBufferInternal(currentImage, entity, camera, ubo);
}

// Internal helper function to complete uniform buffer setup
void Renderer::updateUniformBufferInternal(uint32_t currentImage, Entity *entity, CameraComponent *camera, UniformBufferObject &ubo)
{
	// Get entity resources
	auto entityIt = entityResources.find(entity);
	if (entityIt == entityResources.end())
	{
		return;
	}

	// Use a single source of truth for the frame's light count, set in Render()
	// right before the Forward+ compute dispatch. This ensures all entities see
	// a consistent lightCount and that the PBR fallback loop can run when needed.
	ubo.lightCount = static_cast<int>(lastFrameLightCount);

	// Shadows removed: no shadow bias

	// Set camera position for PBR calculations
	ubo.camPos = glm::vec4(camera->GetPosition(), 1.0f);

	// Set PBR parameters (use member variables for UI control)
	// Clamp exposure to a sane range to avoid washout
	ubo.exposure                 = std::clamp(this->exposure, 0.2f, 4.0f);
	ubo.gamma                    = this->gamma;
	ubo.prefilteredCubeMipLevels = 0.0f;
	ubo.scaleIBLAmbient          = 0.25f;
	ubo.screenDimensions         = glm::vec2(swapChainExtent.width, swapChainExtent.height);
	// Forward+ clustered parameters for fragment shader
	ubo.nearZ   = camera ? camera->GetNearPlane() : 0.1f;
	ubo.farZ    = camera ? camera->GetFarPlane() : 1000.0f;
	ubo.slicesZ = static_cast<float>(forwardPlusSlicesZ);

	// Signal to the shader whether swapchain is sRGB (1) or not (0) using padding0
	int outputIsSRGB = (swapChainImageFormat == vk::Format::eR8G8B8A8Srgb ||
	                    swapChainImageFormat == vk::Format::eB8G8R8A8Srgb) ?
	                       1 :
	                       0;
	ubo.padding0     = outputIsSRGB;
	// Padding fields no longer used for runtime debug toggles
	ubo.padding1 = 0.0f;
	ubo.padding2 = 0.0f;

	// Planar reflections: set sampling flags/matrices for main pass; preserve reflectionPass if already set by caller
	if (ubo.reflectionPass != 1)
	{
		// Main pass: enable planar reflection sampling for glass only when the feature is toggled
		// and we have a valid previous-frame reflection render target to sample from.
		ubo.reflectionPass = 0;
		bool reflReady     = false;
		if (enablePlanarReflections && !reflections.empty())
		{
			// CRITICAL FIX: Use currentFrame (frame-in-flight index) instead of currentImage (swapchain index)
			// Reflection resources are per-frame-in-flight, not per-swapchain-image
			uint32_t prev   = currentImage > 0 ? (currentImage - 1) : (static_cast<uint32_t>(reflections.size()) - 1);
			auto    &rtPrev = reflections[prev];
			reflReady       = !(rtPrev.colorView == nullptr) && !(rtPrev.colorSampler == nullptr);
		}
		ubo.reflectionEnabled = reflReady ? 1 : 0;
		ubo.reflectionVP      = sampleReflectionVP;
		ubo.clipPlaneWS       = currentReflectionPlane;
	}

	// Reflection intensity from UI
	ubo.reflectionIntensity = std::clamp(reflectionIntensity, 0.0f, 2.0f);

	// Ray query rendering options from UI
	ubo.enableRayQueryReflections  = enableRayQueryReflections ? 1 : 0;
	ubo.enableRayQueryTransparency = enableRayQueryTransparency ? 1 : 0;

	// Copy to uniform buffer (guard against null mapped pointer)
	// CRITICAL FIX: Use currentImage (the frame parameter) for uniform buffer indexing
	// uniformBuffersMapped is sized per-frame-in-flight, and currentImage is the frameIndex parameter passed in
	void *dst = entityIt->second.uniformBuffersMapped[currentImage];
	if (!dst)
	{
		// Mapped pointer not available (shouldn’t happen for HostVisible/Coherent). Avoid crash and continue.
		std::cerr << "Warning: UBO mapped ptr null for entity '" << entity->GetName() << "' frame " << currentImage << std::endl;
		return;
	}
	std::memcpy(dst, &ubo, sizeof(ubo));
}

// Render the scene
void Renderer::Render(const std::vector<std::unique_ptr<Entity>> &entities, CameraComponent *camera, ImGuiSystem *imguiSystem)
{
	// Update watchdog timestamp to prove frame is progressing
	lastFrameUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);

	static bool firstRenderLogged = false;
	if (!firstRenderLogged)
	{
		std::cout << "Entering main render loop - application is running successfully!" << std::endl;
		firstRenderLogged = true;
	}

	if (memoryPool)
		memoryPool->setRenderingActive(true);
	struct RenderingStateGuard
	{
		MemoryPool *pool;
		explicit RenderingStateGuard(MemoryPool *p) :
		    pool(p)
		{}
		~RenderingStateGuard()
		{
			if (pool)
				pool->setRenderingActive(false);
		}
	} guard(memoryPool.get());

	// Track if ray query rendered successfully this frame to skip rasterization code path
	bool rayQueryRenderedThisFrame = false;

	// Wait for the previous frame's work on this frame slot to complete
	if (device.waitForFences(*inFlightFences[currentFrame], VK_TRUE, UINT64_MAX) != vk::Result::eSuccess)
	{
		std::cerr << "Warning: Failed to wait for fence on frame " << currentFrame << std::endl;
		return;
	}

	// Reset the fence immediately after successful wait, before any new work
	device.resetFences(*inFlightFences[currentFrame]);

	// Execute any pending GPU uploads (enqueued by worker/loading threads) on the render thread
	// at this safe point to ensure all Vulkan submits happen on a single thread.
	// This prevents validation/GPU-AV PostSubmit crashes due to cross-thread queue usage.
	ProcessPendingMeshUploads();

	// Process deferred AS deletion queue at safe point (after fence wait)
	// Increment frame counters and delete AS structures that are no longer in use
	// Wait for MAX_FRAMES_IN_FLIGHT + 1 frames to ensure GPU has finished all work
	// (The +1 ensures we've waited through a full cycle of all frame slots)
	{
		auto it = pendingASDeletions.begin();
		while (it != pendingASDeletions.end())
		{
			it->framesSinceDestroy++;
			if (it->framesSinceDestroy > MAX_FRAMES_IN_FLIGHT)
			{
				// Safe to delete - all frames have finished using these AS structures
				it = pendingASDeletions.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// Opportunistically request AS rebuild when more meshes become ready than in the last built AS.
	// This makes the TLAS grow as streaming/allocations complete, then settle (no rebuild spam).
	if (rayQueryEnabled && accelerationStructureEnabled)
	{
		size_t readyRenderableCount = 0;
		size_t readyUniqueMeshCount = 0;
		{
			std::map<MeshComponent *, uint32_t> meshToBLASProbe;
			for (const auto &uptr : entities)
			{
				Entity *e = uptr.get();
				if (!e || !e->IsActive())
					continue;
				// In Ray Query static-only mode, ignore dynamic/animated entities for readiness
				if (IsRayQueryStaticOnly())
				{
					const std::string &nm = e->GetName();
					if (nm.find("_AnimNode_") != std::string::npos)
						continue;
					if (!nm.empty() && nm.rfind("Ball_", 0) == 0)
						continue;
				}
				auto meshComp = e->GetComponent<MeshComponent>();
				if (!meshComp)
					continue;
				try
				{
					auto it = meshResources.find(meshComp);
					if (it == meshResources.end())
						continue;
					const auto &res = it->second;
					// STRICT readiness: uploads must be finished (staging sizes zero)
					if (res.vertexBufferSizeBytes != 0 || res.indexBufferSizeBytes != 0)
						continue;
					if (!*res.vertexBuffer || !*res.indexBuffer)
						continue;
					if (res.indexCount == 0)
						continue;
				}
				catch (...)
				{
					continue;
				}
				readyRenderableCount++;
				if (meshToBLASProbe.find(meshComp) == meshToBLASProbe.end())
				{
					meshToBLASProbe[meshComp] = static_cast<uint32_t>(meshToBLASProbe.size());
				}
			}
			readyUniqueMeshCount = meshToBLASProbe.size();
		}
		if (asOpportunisticRebuildEnabled && !asFrozen && (readyRenderableCount > lastASBuiltInstanceCount || readyUniqueMeshCount > lastASBuiltBLASCount) && !asBuildRequested.load(std::memory_order_relaxed))
		{
			std::cout << "AS rebuild requested: counts increased (built instances=" << lastASBuiltInstanceCount
			          << ", ready instances=" << readyRenderableCount
			          << ", built meshes=" << lastASBuiltBLASCount
			          << ", ready meshes=" << readyUniqueMeshCount << ")\n";
			RequestAccelerationStructureBuild("counts increased");
		}

		// Post-load repair: if loading is done and the current TLAS instance count is far below readiness,
		// force a one-time rebuild even when frozen so we include the whole scene.
		if (!IsLoading() && !asBuildRequested.load(std::memory_order_relaxed))
		{
			const size_t targetInstances = readyRenderableCount;
			if (targetInstances > 0 && lastASBuiltInstanceCount < static_cast<size_t>(static_cast<double>(targetInstances) * 0.95))
			{
				asDevOverrideAllowRebuild = true;        // allow rebuild even if frozen
				std::cout << "AS rebuild requested: post-load full build (built instances=" << lastASBuiltInstanceCount
				          << ", ready instances=" << targetInstances << ")\n";
				RequestAccelerationStructureBuild("post-load full build");
			}
		}

		// If in Ray Query static-only mode and TLAS not yet built post-load, request a one-time build now
		if (currentRenderMode == RenderMode::RayQuery && IsRayQueryStaticOnly() && !IsLoading() && !*tlasStructure.handle && !asBuildRequested.load(std::memory_order_relaxed))
		{
			RequestAccelerationStructureBuild("static-only initial build");
		}
	}

	// Check if acceleration structure build was requested (e.g., after scene loading or counts grew)
	// Build at this safe frame point to avoid threading issues
	if (asBuildRequested.load(std::memory_order_acquire))
	{
		// Defer TLAS/BLAS build while the scene is loading to avoid partial builds (e.g., only animated fans)
		if (IsLoading())
		{
			// Keep the request flag set; we'll build once loading completes
			static uint32_t asDeferredLoadingCounter = 0;
			if ((asDeferredLoadingCounter++ % 120u) == 0u)
			{
				std::cout << "AS build deferred: scene still loading" << std::endl;
			}
		}
		else if (asFrozen && !asDevOverrideAllowRebuild)
		{
			// Ignore rebuilds while frozen to avoid wiping TLAS during animation playback
			std::cout << "AS rebuild request ignored (frozen). Reason: " << lastASBuildRequestReason << "\n";
			asBuildRequested.store(false, std::memory_order_release);
		}
		else
		{
			// Gate initial build until readiness is high enough to represent the full scene
			size_t totalRenderableEntities = 0;
			size_t readyRenderableCount    = 0;
			size_t readyUniqueMeshCount    = 0;
			{
				std::map<MeshComponent *, uint32_t> meshToBLASProbe;
				for (const auto &uptr : entities)
				{
					Entity *e = uptr.get();
					if (!e || !e->IsActive())
						continue;
					// In Ray Query static-only mode, ignore dynamic/animated entities for totals/readiness
					if (IsRayQueryStaticOnly())
					{
						const std::string &nm = e->GetName();
						if (nm.find("_AnimNode_") != std::string::npos)
							continue;
						if (!nm.empty() && nm.rfind("Ball_", 0) == 0)
							continue;
					}
					auto meshComp = e->GetComponent<MeshComponent>();
					if (!meshComp)
						continue;
					totalRenderableEntities++;
					try
					{
						auto it = meshResources.find(meshComp);
						if (it == meshResources.end())
							continue;
						const auto &res = it->second;
						// STRICT readiness here too: uploads finished
						if (res.vertexBufferSizeBytes != 0 || res.indexBufferSizeBytes != 0)
							continue;
						if (!*res.vertexBuffer || !*res.indexBuffer)
							continue;
						if (res.indexCount == 0)
							continue;
					}
					catch (...)
					{
						continue;
					}
					readyRenderableCount++;
					if (meshToBLASProbe.find(meshComp) == meshToBLASProbe.end())
					{
						meshToBLASProbe[meshComp] = static_cast<uint32_t>(meshToBLASProbe.size());
					}
				}
				readyUniqueMeshCount = meshToBLASProbe.size();
			}
			const double readiness      = (totalRenderableEntities > 0) ? static_cast<double>(readyRenderableCount) / static_cast<double>(totalRenderableEntities) : 0.0;
			const double buildThreshold = 0.95;        // build only when ~full scene is ready
			if (readiness < buildThreshold && !asDevOverrideAllowRebuild)
			{
				static uint32_t asDeferredReadinessCounter = 0;
				if ((asDeferredReadinessCounter++ % 120u) == 0u)
				{
					std::cout << "AS build deferred: readiness " << readyRenderableCount << "/" << totalRenderableEntities
					          << " entities (" << static_cast<int>(readiness * 100.0) << "%), uniqueMeshesReady="
					          << readyUniqueMeshCount << std::endl;
				}
				// Keep the request flag set; try again next frame
			}
			else
			{
				// CRITICAL: Wait for ALL GPU work to complete BEFORE building AS.
				// External synchronization required (VVL): serialize against queue submits/present.
				// This ensures no command buffers are still using vertex/index buffers that the AS build will reference.
				WaitIdle();

				if (buildAccelerationStructures(entities))
				{
					asBuildRequested.store(false, std::memory_order_release);
					// Freeze only when the built TLAS is "full" (>=95% of static opaque renderables)
					if (asFreezeAfterFullBuild)
					{
						const double threshold = 0.95;
						if (totalRenderableEntities > 0 && static_cast<double>(lastASBuiltInstanceCount) >= threshold * static_cast<double>(totalRenderableEntities))
						{
							asFrozen = true;
							std::cout << "AS frozen after full build (instances=" << lastASBuiltInstanceCount
							          << "/" << totalRenderableEntities << ")" << std::endl;
						}
						else
						{
							std::cout << "AS not frozen yet (built instances=" << lastASBuiltInstanceCount
							          << ", total renderables=" << totalRenderableEntities << ")" << std::endl;
						}
					}
					// One-line TLAS summary with device address
					if (*tlasStructure.handle)
					{
						if (IsRayQueryStaticOnly())
						{
							std::cout << "TLAS ready (static-only): instances=" << lastASBuiltInstanceCount
							          << ", BLAS=" << lastASBuiltBLASCount
							          << ", addr=0x" << std::hex << tlasStructure.deviceAddress << std::dec << std::endl;
						}
						else
						{
							std::cout << "TLAS ready: instances=" << lastASBuiltInstanceCount
							          << ", BLAS=" << lastASBuiltBLASCount
							          << ", addr=0x" << std::hex << tlasStructure.deviceAddress << std::dec << std::endl;
						}
					}
				}
				else
				{
					std::cout << "Failed to build acceleration structures, will retry next frame" << std::endl;
				}
				// Reset dev override after one use
				asDevOverrideAllowRebuild = false;
			}
		}
	}

	// Safe point: the previous work referencing this frame's descriptor sets is complete.
	// Apply any deferred descriptor set updates for entities whose textures finished streaming.
	ProcessDirtyDescriptorsForFrame(currentFrame);

	// Safe point pre-pass: ensure descriptor sets exist for all visible entities this frame
	// and initialize only binding 0 (UBO) for the current frame if not already done.
	{
		uint32_t entityProcessCount = 0;
		for (const auto &uptr : entities)
		{
			Entity *entity = uptr.get();
			if (!entity || !entity->IsActive())
				continue;
			auto meshComponent = entity->GetComponent<MeshComponent>();
			if (!meshComponent)
				continue;
			auto entityIt = entityResources.find(entity);
			if (entityIt == entityResources.end())
				continue;

			// Update watchdog every 100 entities to prevent false hang detection during heavy descriptor creation
			entityProcessCount++;
			if (entityProcessCount % 100 == 0)
			{
				lastFrameUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
			}

			// Determine a reasonable base texture path for initial descriptor writes
			std::string texPath = meshComponent->GetBaseColorTexturePath();
			if (texPath.empty())
				texPath = meshComponent->GetTexturePath();

			// Create descriptor sets on demand if missing
			if (entityIt->second.basicDescriptorSets.empty())
			{
				createDescriptorSets(entity, texPath, /*usePBR=*/false);
			}
			if (entityIt->second.pbrDescriptorSets.empty())
			{
				createDescriptorSets(entity, texPath, /*usePBR=*/true);
			}

			// Ensure ONLY binding 0 (UBO) is written for the CURRENT frame's PBR set once.
			// Avoid touching image bindings here to keep per-frame descriptor churn minimal.
			updateDescriptorSetsForFrame(entity,
			                             texPath,
			                             /*usePBR=*/true,
			                             currentFrame,
			                             /*imagesOnly=*/false,
			                             /*uboOnly=*/true);

			// Cold-initialize image bindings for CURRENT frame once to avoid per-frame black flashes.
			// This writes PBR b1..b5 and Basic b1 with either real textures or shared defaults.
			// It does not touch UBO (handled above).
			// PBR images
			if (entityIt->second.pbrImagesWritten.size() != MAX_FRAMES_IN_FLIGHT)
			{
				entityIt->second.pbrImagesWritten.assign(MAX_FRAMES_IN_FLIGHT, false);
			}
			if (!entityIt->second.pbrImagesWritten[currentFrame])
			{
				updateDescriptorSetsForFrame(entity,
				                             texPath,
				                             /*usePBR=*/true,
				                             currentFrame,
				                             /*imagesOnly=*/true,
				                             /*uboOnly=*/false);
				entityIt->second.pbrImagesWritten[currentFrame] = true;
			}
			// Basic images
			if (entityIt->second.basicImagesWritten.size() != MAX_FRAMES_IN_FLIGHT)
			{
				entityIt->second.basicImagesWritten.assign(MAX_FRAMES_IN_FLIGHT, false);
			}
			if (!entityIt->second.basicImagesWritten[currentFrame])
			{
				updateDescriptorSetsForFrame(entity,
				                             texPath,
				                             /*usePBR=*/false,
				                             currentFrame,
				                             /*imagesOnly=*/true,
				                             /*uboOnly=*/false);
				entityIt->second.basicImagesWritten[currentFrame] = true;
			}
		}
	}

	// Safe point: flush any descriptor updates that were deferred while a command buffer
	// was recording in a prior frame. Only apply ops for the current frame to avoid
	// update-after-bind on pending frames.
	if (descriptorRefreshPending.load(std::memory_order_relaxed))
	{
		std::vector<PendingDescOp> ops;
		{
			std::lock_guard<std::mutex> lk(pendingDescMutex);
			ops.swap(pendingDescOps);
			descriptorRefreshPending.store(false, std::memory_order_relaxed);
		}
		for (auto &op : ops)
		{
			if (op.frameIndex == currentFrame)
			{
				// Now not recording; safe to apply updates for this frame
				updateDescriptorSetsForFrame(op.entity, op.texPath, op.usePBR, op.frameIndex, op.imagesOnly);
			}
			else
			{
				// Keep other frame ops queued for next frame’s safe point
				std::lock_guard<std::mutex> lk(pendingDescMutex);
				pendingDescOps.push_back(op);
				descriptorRefreshPending.store(true, std::memory_order_relaxed);
			}
		}
	}

	// Safe point: handle any pending reflection resource (re)creation and per-frame descriptor refreshes
	if (reflectionResourcesDirty)
	{
		if (enablePlanarReflections)
		{
			uint32_t rw = std::max(1u, static_cast<uint32_t>(static_cast<float>(swapChainExtent.width) * reflectionResolutionScale));
			uint32_t rh = std::max(1u, static_cast<uint32_t>(static_cast<float>(swapChainExtent.height) * reflectionResolutionScale));
			createReflectionResources(rw, rh);
		}
		else
		{
			destroyReflectionResources();
		}
		reflectionResourcesDirty = false;
	}

	// Reflection descriptor binding refresh is handled elsewhere; avoid redundant per-frame mass updates here.
	// Pick the VP associated with the previous frame's reflection texture for sampling in the main pass
	if (enablePlanarReflections && !reflectionVPs.empty())
	{
		uint32_t prev      = (currentFrame > 0) ? (currentFrame - 1) : (static_cast<uint32_t>(reflectionVPs.size()) - 1);
		sampleReflectionVP = reflectionVPs[prev];
	}

	// CRITICAL FIX: DO NOT call refreshPBRForwardPlusBindingsForFrame every frame!
	// This function updates bindings 6/7/8 (storage buffers) which don't have UPDATE_AFTER_BIND.
	// Updating these every frame causes "updated without UPDATE_AFTER_BIND" errors with MAX_FRAMES_IN_FLIGHT > 1.
	// These bindings are already initialized in createDescriptorSets and updated when buffers change.
	// Binding 10 (reflection map) has UPDATE_AFTER_BIND and can be updated separately if needed.
	// refreshPBRForwardPlusBindingsForFrame(currentFrame);

	// Acquire next swapchain image
	// We must provide a semaphore to acquireNextImage that will be signaled when the image is ready.
	// Use currentFrame to cycle through available semaphores (one per frame-in-flight).
	// After acquire, we'll use imageIndex to select semaphores for submit/present.
	uint32_t acquireSemaphoreIndex = currentFrame % static_cast<uint32_t>(imageAvailableSemaphores.size());

	uint32_t                  imageIndex;
	vk::ResultValue<uint32_t> result{{}, 0};
	try
	{
		result = swapChain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[acquireSemaphoreIndex]);
	}
	catch (const vk::OutOfDateKHRError &)
	{
		// Swapchain is out of date (e.g., window resized) before we could
		// query the result. Trigger recreation and exit this frame cleanly.
		framebufferResized.store(true, std::memory_order_relaxed);
		if (imguiSystem)
			ImGui::EndFrame();
		recreateSwapChain();
		return;
	}

	imageIndex = result.value;

	if (result.result == vk::Result::eErrorOutOfDateKHR || result.result == vk::Result::eSuboptimalKHR || framebufferResized.load(std::memory_order_relaxed))
	{
		framebufferResized.store(false, std::memory_order_relaxed);
		if (imguiSystem)
			ImGui::EndFrame();
		recreateSwapChain();
		return;
	}
	if (result.result != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	if (framebufferResized.load(std::memory_order_relaxed))
	{
		recreateSwapChain();
		return;
	}

	// Perform any descriptor updates that must not happen during command buffer recording
	if (useForwardPlus)
	{
		uint32_t tilesX_pre = (swapChainExtent.width + forwardPlusTileSizeX - 1) / forwardPlusTileSizeX;
		uint32_t tilesY_pre = (swapChainExtent.height + forwardPlusTileSizeY - 1) / forwardPlusTileSizeY;
		// Only update current frame's descriptors to avoid touching in-flight frames
		createOrResizeForwardPlusBuffers(tilesX_pre, tilesY_pre, forwardPlusSlicesZ, /*updateOnlyCurrentFrame=*/true);
		// After (re)creating Forward+ buffers, bindings 7/8 will be refreshed as needed.
	}

	// Ensure light buffers are sufficiently large before recording to avoid resizing while in use
	{
		// Reserve capacity based on emissive lights only (punctual lights disabled for now)
		size_t desiredLightCapacity = 0;
		if (!staticLights.empty())
		{
			size_t emissiveCount = 0;
			for (const auto &L : staticLights)
			{
				if (L.type == ExtractedLight::Type::Emissive)
				{
					++emissiveCount;
					if (emissiveCount >= MAX_ACTIVE_LIGHTS)
						break;
				}
			}
			desiredLightCapacity = emissiveCount;
		}
		if (desiredLightCapacity > 0)
		{
			createOrResizeLightStorageBuffers(desiredLightCapacity);
			// Ensure compute (binding 0) sees the current frame's lights buffer
			refreshForwardPlusComputeLightsBindingForFrame(currentFrame);
			// Bindings 6/7/8 for PBR are refreshed only when buffers change (handled in resize path).
		}
	}

	// Safe point: Update ray query descriptor sets if ray query mode is active
	// This MUST happen before command buffer recording starts to avoid "descriptor updated without UPDATE_AFTER_BIND" errors
	if (currentRenderMode == RenderMode::RayQuery && rayQueryEnabled && accelerationStructureEnabled)
	{
		if (*tlasStructure.handle)
		{
			updateRayQueryDescriptorSets(currentFrame, entities);
		}
	}

	commandBuffers[currentFrame].reset();
	// Begin command buffer recording for this frame
	commandBuffers[currentFrame].begin(vk::CommandBufferBeginInfo());
	isRecordingCmd.store(true, std::memory_order_relaxed);
	if (framebufferResized.load(std::memory_order_relaxed))
	{
		commandBuffers[currentFrame].end();
		recreateSwapChain();
		return;
	}

	// Extract lights for this frame (needed by both ray query and rasterization)
	// Build a single light list once per frame (emissive lights only for this scene)
	std::vector<ExtractedLight> lightsSubset;
	if (!staticLights.empty())
	{
		lightsSubset.reserve(std::min(staticLights.size(), static_cast<size_t>(MAX_ACTIVE_LIGHTS)));
		for (const auto &L : staticLights)
		{
			if (L.type == ExtractedLight::Type::Emissive)
			{
				lightsSubset.push_back(L);
				if (lightsSubset.size() >= MAX_ACTIVE_LIGHTS)
					break;
			}
		}
	}
	auto lightCountF    = static_cast<uint32_t>(lightsSubset.size());
	lastFrameLightCount = lightCountF;
	if (!lightsSubset.empty())
	{
		updateLightStorageBuffer(currentFrame, lightsSubset);
	}

	// Ray query rendering mode dispatch
	if (currentRenderMode == RenderMode::RayQuery && rayQueryEnabled && accelerationStructureEnabled)
	{
		// Check if TLAS handle is valid (dereference RAII handle)
		if (!*tlasStructure.handle)
		{
			// TLAS not built yet – present a diagnostic frame from the ray-query path to avoid
			// accidentally showing rasterized content. Fill swapchain with a distinct color.
			// Transition swapchain image from PRESENT to TRANSFER_DST
			vk::ImageMemoryBarrier2 swapchainBarrier{};
			swapchainBarrier.srcStageMask                = vk::PipelineStageFlagBits2::eBottomOfPipe;
			swapchainBarrier.srcAccessMask               = vk::AccessFlagBits2::eNone;
			swapchainBarrier.dstStageMask                = vk::PipelineStageFlagBits2::eTransfer;
			swapchainBarrier.dstAccessMask               = vk::AccessFlagBits2::eTransferWrite;
			swapchainBarrier.oldLayout                   = (imageIndex < swapChainImageLayouts.size()) ? swapChainImageLayouts[imageIndex] : vk::ImageLayout::eUndefined;
			swapchainBarrier.newLayout                   = vk::ImageLayout::eTransferDstOptimal;
			swapchainBarrier.image                       = swapChainImages[imageIndex];
			swapchainBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			swapchainBarrier.subresourceRange.levelCount = 1;
			swapchainBarrier.subresourceRange.layerCount = 1;

			vk::DependencyInfo depInfoSwap{};
			depInfoSwap.imageMemoryBarrierCount = 1;
			depInfoSwap.pImageMemoryBarriers    = &swapchainBarrier;
			commandBuffers[currentFrame].pipelineBarrier2(depInfoSwap);
			if (imageIndex < swapChainImageLayouts.size())
				swapChainImageLayouts[imageIndex] = swapchainBarrier.newLayout;

			// Clear to a distinct magenta diagnostic color
			vk::ClearColorValue       clearColor{std::array<float, 4>{1.0f, 0.0f, 1.0f, 1.0f}};
			vk::ImageSubresourceRange clearRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
			commandBuffers[currentFrame].clearColorImage(swapChainImages[imageIndex], vk::ImageLayout::eTransferDstOptimal, clearColor, clearRange);

			// Transition back to PRESENT
			swapchainBarrier.srcStageMask  = vk::PipelineStageFlagBits2::eTransfer;
			swapchainBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
			swapchainBarrier.dstStageMask  = vk::PipelineStageFlagBits2::eBottomOfPipe;
			swapchainBarrier.dstAccessMask = vk::AccessFlagBits2::eNone;
			swapchainBarrier.oldLayout     = vk::ImageLayout::eTransferDstOptimal;
			swapchainBarrier.newLayout     = vk::ImageLayout::ePresentSrcKHR;
			commandBuffers[currentFrame].pipelineBarrier2(depInfoSwap);
			if (imageIndex < swapChainImageLayouts.size())
				swapChainImageLayouts[imageIndex] = swapchainBarrier.newLayout;

			rayQueryRenderedThisFrame = true;        // Skip raster; ensure we are looking at RQ path only
		}
		else
		{
			// TLAS is valid and descriptor sets were already updated at safe point
			// Proceed with ray query rendering
			// In static-only mode, skip refit to keep TLAS immutable
			if (!IsRayQueryStaticOnly())
			{
				// If animation updated transforms this frame, refit TLAS instead of rebuilding
				// This prevents wiping TLAS contents to only animated instances
				refitTopLevelAS(entities);
			}

			// Update descriptors for this frame. If it fails (e.g., stale/invalid sets), skip ray query safely.
			if (!updateRayQueryDescriptorSets(currentFrame, entities))
			{
				std::cerr << "Ray Query descriptor update failed; skipping ray query this frame\n";
			}
			else
			{
				// Bind ray query compute pipeline
				commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eCompute, *rayQueryPipeline);

				// Bind descriptor set
				commandBuffers[currentFrame].bindDescriptorSets(
				    vk::PipelineBindPoint::eCompute,
				    *rayQueryPipelineLayout,
				    0,
				    *rayQueryDescriptorSets[currentFrame],
				    nullptr);

				// CRITICAL: Update dedicated ray query UBO with camera matrices
				// This dedicated UBO is separate from entity UBOs and uses a Ray Query-specific layout.
				if (rayQueryUniformBuffersMapped.size() > currentFrame && rayQueryUniformBuffersMapped[currentFrame])
				{
					RayQueryUniformBufferObject ubo{};
					ubo.model = glm::mat4(1.0f);        // Identity - not used for ray query

					// Force view matrix update to reflect current camera position
					// (the dirty flag isn't automatically set when camera position changes)
					camera->ForceViewMatrixUpdate();

					// Get camera matrices
					glm::mat4 camView = camera->GetViewMatrix();
					ubo.view          = camView;
					ubo.proj          = camera->GetProjectionMatrix();
					ubo.proj[1][1] *= -1;        // Flip Y for Vulkan
					ubo.camPos = glm::vec4(camera->GetPosition(), 1.0f);
					// Clamp to sane ranges to avoid black output (exposure=0 → 1-exp(0)=0)
					ubo.exposure = std::clamp(exposure, 0.2f, 4.0f);
					ubo.gamma    = std::clamp(gamma, 1.6f, 2.6f);
					// Match raster convention: ambient scale factor for simple IBL/ambient term.
					// (Raster defaults to ~0.25 in the main pass; keep Ray Query consistent.)
					ubo.scaleIBLAmbient = 0.25f;
					// Provide the per-frame light count so the ray query shader can iterate lights.
					ubo.lightCount                 = static_cast<int>(lastFrameLightCount);
					ubo.screenDimensions           = glm::vec2(swapChainExtent.width, swapChainExtent.height);
					ubo.enableRayQueryReflections  = enableRayQueryReflections ? 1 : 0;
					ubo.enableRayQueryTransparency = enableRayQueryTransparency ? 1 : 0;
					// Max secondary bounces (reflection/refraction). Stored in the padding slot to avoid UBO layout churn.
					// Shader clamps this value.
					ubo._pad0 = rayQueryMaxBounces;
					// Provide geometry info count for shader-side bounds checking (per-instance)
					ubo.geometryInfoCount = static_cast<int>(tlasInstanceCount);
					// Provide material buffer count for shader-side bounds checking
					ubo.materialCount = static_cast<int>(materialCountCPU);

					// Copy to mapped memory
					std::memcpy(rayQueryUniformBuffersMapped[currentFrame], &ubo, sizeof(RayQueryUniformBufferObject));
				}
				else
				{
					// Keep concise error for visibility
					std::cerr << "Ray Query UBO not mapped for frame " << currentFrame << "\n";
				}

				// Dispatch compute shader (8x8 workgroups as defined in shader)
				uint32_t workgroupsX = (swapChainExtent.width + 7) / 8;
				uint32_t workgroupsY = (swapChainExtent.height + 7) / 8;
				commandBuffers[currentFrame].dispatch(workgroupsX, workgroupsY, 1);

				// Barrier: wait for compute shader to finish writing to output image,
				// then make it readable by fragment shader for sampling in composite pass
				vk::ImageMemoryBarrier2 rqToSample{};
				rqToSample.srcStageMask                = vk::PipelineStageFlagBits2::eComputeShader;
				rqToSample.srcAccessMask               = vk::AccessFlagBits2::eShaderWrite;
				rqToSample.dstStageMask                = vk::PipelineStageFlagBits2::eFragmentShader;
				rqToSample.dstAccessMask               = vk::AccessFlagBits2::eShaderRead;
				rqToSample.oldLayout                   = vk::ImageLayout::eGeneral;
				rqToSample.newLayout                   = vk::ImageLayout::eShaderReadOnlyOptimal;
				rqToSample.image                       = *rayQueryOutputImage;
				rqToSample.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				rqToSample.subresourceRange.levelCount = 1;
				rqToSample.subresourceRange.layerCount = 1;

				vk::DependencyInfo depRQToSample{};
				depRQToSample.imageMemoryBarrierCount = 1;
				depRQToSample.pImageMemoryBarriers    = &rqToSample;
				commandBuffers[currentFrame].pipelineBarrier2(depRQToSample);

				// Composite fullscreen: sample rayQueryOutputImage to the swapchain using the composite pipeline
				// Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL
				vk::ImageMemoryBarrier2 swapchainToColor{};
				swapchainToColor.srcStageMask                = vk::PipelineStageFlagBits2::eBottomOfPipe;
				swapchainToColor.srcAccessMask               = vk::AccessFlagBits2::eNone;
				swapchainToColor.dstStageMask                = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
				swapchainToColor.dstAccessMask               = vk::AccessFlagBits2::eColorAttachmentWrite;
				swapchainToColor.oldLayout                   = (imageIndex < swapChainImageLayouts.size()) ? swapChainImageLayouts[imageIndex] : vk::ImageLayout::eUndefined;
				swapchainToColor.newLayout                   = vk::ImageLayout::eColorAttachmentOptimal;
				swapchainToColor.image                       = swapChainImages[imageIndex];
				swapchainToColor.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				swapchainToColor.subresourceRange.levelCount = 1;
				swapchainToColor.subresourceRange.layerCount = 1;
				vk::DependencyInfo depSwapToColor{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &swapchainToColor};
				commandBuffers[currentFrame].pipelineBarrier2(depSwapToColor);
				if (imageIndex < swapChainImageLayouts.size())
					swapChainImageLayouts[imageIndex] = swapchainToColor.newLayout;

				// Begin dynamic rendering for composite (no depth)
				colorAttachments[0].imageView  = *swapChainImageViews[imageIndex];
				colorAttachments[0].loadOp     = vk::AttachmentLoadOp::eClear;
				depthAttachment.loadOp         = vk::AttachmentLoadOp::eDontCare;
				renderingInfo.renderArea       = vk::Rect2D({0, 0}, swapChainExtent);
				auto savedDepthPtr2            = renderingInfo.pDepthAttachment;
				renderingInfo.pDepthAttachment = nullptr;
				commandBuffers[currentFrame].beginRendering(renderingInfo);

				if (compositePipeline != nullptr)
				{
					commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *compositePipeline);
				}
				vk::Viewport vp(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
				vk::Rect2D   sc({0, 0}, swapChainExtent);
				commandBuffers[currentFrame].setViewport(0, vp);
				commandBuffers[currentFrame].setScissor(0, sc);

				// Bind the RQ composite descriptor set (samples rayQueryOutputImage)
				if (!rqCompositeDescriptorSets.empty())
				{
					commandBuffers[currentFrame].bindDescriptorSets(
					    vk::PipelineBindPoint::eGraphics,
					    *compositePipelineLayout,
					    0,
					    {*rqCompositeDescriptorSets[currentFrame]},
					    {});
				}

				// Push exposure/gamma and sRGB flag
				struct CompositePush
				{
					float exposure;
					float gamma;
					int   outputIsSRGB;
					float _pad;
				} pc2{};
				pc2.exposure     = std::clamp(this->exposure, 0.2f, 4.0f);
				pc2.gamma        = this->gamma;
				pc2.outputIsSRGB = (swapChainImageFormat == vk::Format::eR8G8B8A8Srgb || swapChainImageFormat == vk::Format::eB8G8R8A8Srgb) ? 1 : 0;
				commandBuffers[currentFrame].pushConstants<CompositePush>(*compositePipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pc2);

				commandBuffers[currentFrame].draw(3, 1, 0, 0);
				commandBuffers[currentFrame].endRendering();
				renderingInfo.pDepthAttachment = savedDepthPtr2;

				// Transition swapchain back to PRESENT and RQ image back to GENERAL for next frame
				vk::ImageMemoryBarrier2 swapchainToPresent{};
				swapchainToPresent.srcStageMask                = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
				swapchainToPresent.srcAccessMask               = vk::AccessFlagBits2::eColorAttachmentWrite;
				swapchainToPresent.dstStageMask                = vk::PipelineStageFlagBits2::eBottomOfPipe;
				swapchainToPresent.dstAccessMask               = vk::AccessFlagBits2::eNone;
				swapchainToPresent.oldLayout                   = vk::ImageLayout::eColorAttachmentOptimal;
				swapchainToPresent.newLayout                   = vk::ImageLayout::ePresentSrcKHR;
				swapchainToPresent.image                       = swapChainImages[imageIndex];
				swapchainToPresent.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				swapchainToPresent.subresourceRange.levelCount = 1;
				swapchainToPresent.subresourceRange.layerCount = 1;

				vk::ImageMemoryBarrier2 rqBackToGeneral{};
				rqBackToGeneral.srcStageMask                = vk::PipelineStageFlagBits2::eFragmentShader;
				rqBackToGeneral.srcAccessMask               = vk::AccessFlagBits2::eShaderRead;
				rqBackToGeneral.dstStageMask                = vk::PipelineStageFlagBits2::eComputeShader;
				rqBackToGeneral.dstAccessMask               = vk::AccessFlagBits2::eShaderWrite;
				rqBackToGeneral.oldLayout                   = vk::ImageLayout::eShaderReadOnlyOptimal;
				rqBackToGeneral.newLayout                   = vk::ImageLayout::eGeneral;
				rqBackToGeneral.image                       = *rayQueryOutputImage;
				rqBackToGeneral.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				rqBackToGeneral.subresourceRange.levelCount = 1;
				rqBackToGeneral.subresourceRange.layerCount = 1;

				std::array<vk::ImageMemoryBarrier2, 2> barriers{swapchainToPresent, rqBackToGeneral};
				vk::DependencyInfo                     depEnd{.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()), .pImageMemoryBarriers = barriers.data()};
				commandBuffers[currentFrame].pipelineBarrier2(depEnd);
				if (imageIndex < swapChainImageLayouts.size())
					swapChainImageLayouts[imageIndex] = swapchainToPresent.newLayout;

				// Ray query rendering complete - set flag to skip rasterization code path
				rayQueryRenderedThisFrame = true;
			}
		}
	}

	// Process texture streaming uploads (see Renderer::ProcessPendingTextureJobs)

	vk::raii::Pipeline          *currentPipeline = nullptr;
	vk::raii::PipelineLayout    *currentLayout   = nullptr;
	std::vector<Entity *>        blendedQueue;
	std::unordered_set<Entity *> blendedSet;

	// Incrementally process pending texture uploads on the main thread so that
	// all Vulkan submits happen from a single place while worker threads only
	// handle CPU-side decoding. While the loading screen is up, prioritize
	// critical textures so the first rendered frame looks mostly correct.
	if (IsLoading())
	{
		// Larger budget while loading screen is visible so we don't stall
		// streaming of near-field baseColor textures.
		ProcessPendingTextureJobs(/*maxJobs=*/16, /*includeCritical=*/true, /*includeNonCritical=*/false);
	}
	else
	{
		// After loading screen disappears, we want the scene to remain
		// responsive (~20 fps) while textures stream in. Limit the number
		// of non-critical uploads per frame so we don't tank frame time.
		static uint32_t streamingFrameCounter = 0;
		streamingFrameCounter++;
		// Ray Query needs textures visible quickly; process more streaming work when in Ray Query mode.
		if (currentRenderMode == RenderMode::RayQuery)
		{
			// Aggressively drain both critical and non-critical queues each frame for faster bring-up.
			ProcessPendingTextureJobs(/*maxJobs=*/32, /*includeCritical=*/true, /*includeNonCritical=*/true);
		}
		else
		{
			// Raster path: keep previous throttling to avoid stalls.
			if ((streamingFrameCounter % 3) == 0)
			{
				ProcessPendingTextureJobs(/*maxJobs=*/1, /*includeCritical=*/false, /*includeNonCritical=*/true);
			}
		}
	}

	// Renderer UI - available for both ray query and rasterization modes
	// Skip rendering the UI when loading or if ImGuiSystem already called Render() during NewFrame().
	// This prevents calling ImGui::Begin() after ImGui::Render() has been called in the same frame,
	// which would violate ImGui's frame lifecycle and trigger assertion failures.
	if (imguiSystem && !IsLoading() && !imguiSystem->IsFrameRendered())
	{
		if (ImGui::Begin("Renderer"))
		{
			// Declare variables that need to persist across conditional blocks
			bool prevFwdPlus = useForwardPlus;

			// === RENDERING MODE SELECTION (TOP) ===
			ImGui::Text("Rendering Mode:");
			if (rayQueryEnabled && accelerationStructureEnabled)
			{
				const char *modeNames[] = {"Rasterization", "Ray Query"};
				int         currentMode = (currentRenderMode == RenderMode::RayQuery) ? 1 : 0;
				if (ImGui::Combo("Mode", &currentMode, modeNames, 2))
				{
					RenderMode newMode = (currentMode == 1) ? RenderMode::RayQuery : RenderMode::Rasterization;
					if (newMode != currentRenderMode)
					{
						currentRenderMode = newMode;
						std::cout << "Switched to " << modeNames[currentMode] << " mode\n";

						// Request acceleration structure build when switching to ray query mode
						if (currentRenderMode == RenderMode::RayQuery)
						{
							std::cout << "Requesting acceleration structure build...\n";
							RequestAccelerationStructureBuild();
						}
					}
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Rasterization only (ray query not supported)");
			}

			// === RASTERIZATION-SPECIFIC OPTIONS ===
			if (currentRenderMode == RenderMode::Rasterization)
			{
				ImGui::Separator();
				ImGui::Text("Rasterization Options:");

				// Lighting Controls - BRDF/PBR is now the default lighting model
				bool useBasicLighting = imguiSystem && !imguiSystem->IsPBREnabled();
				if (ImGui::Checkbox("Use Basic Lighting (Phong)", &useBasicLighting))
				{
					imguiSystem->SetPBREnabled(!useBasicLighting);
					std::cout << "Lighting mode: " << (!useBasicLighting ? "BRDF/PBR (default)" : "Basic Phong") << std::endl;
				}

				if (!useBasicLighting)
				{
					ImGui::Text("Status: BRDF/PBR pipeline active (default)");
					ImGui::Text("All models rendered with physically-based lighting");
				}
				else
				{
					ImGui::Text("Status: Basic Phong pipeline active");
					ImGui::Text("All models rendered with basic Phong shading");
				}

				ImGui::Checkbox("Forward+ (tiled light culling)", &useForwardPlus);
				if (useForwardPlus && !prevFwdPlus)
				{
					// Lazily create Forward+ resources if enabled at runtime
					if (forwardPlusPipeline == nullptr || forwardPlusDescriptorSetLayout == nullptr || forwardPlusPerFrame.empty())
					{
						createForwardPlusPipelinesAndResources();
					}
					if (depthPrepassPipeline == nullptr)
					{
						createDepthPrepassPipeline();
					}
				}

				// Planar reflections controls
				ImGui::Spacing();
				if (ImGui::Checkbox("Planar reflections (experimental)", &enablePlanarReflections))
				{
					// Defer actual (re)creation/destruction to the next safe point at frame start
					reflectionResourcesDirty = true;
				}
				float scaleBefore = reflectionResolutionScale;
				if (ImGui::SliderFloat("Reflection resolution scale", &reflectionResolutionScale, 0.25f, 1.0f, "%.2f"))
				{
					reflectionResolutionScale = std::clamp(reflectionResolutionScale, 0.25f, 1.0f);
					if (enablePlanarReflections && std::abs(scaleBefore - reflectionResolutionScale) > 1e-3f)
					{
						reflectionResourcesDirty = true;
					}
				}
				if (enablePlanarReflections && !reflections.empty())
				{
					auto &rt = reflections[currentFrame];
					if (rt.width > 0)
					{
						ImGui::Text("Reflection RT: %ux%u", rt.width, rt.height);
					}
				}
				if (enablePlanarReflections)
				{
					ImGui::SliderFloat("Reflection intensity", &reflectionIntensity, 0.0f, 2.0f, "%.2f");
				}
			}

			// === RAY QUERY-SPECIFIC OPTIONS ===
			if (currentRenderMode == RenderMode::RayQuery && rayQueryEnabled && accelerationStructureEnabled)
			{
				ImGui::Separator();
				ImGui::Text("Ray Query Status:");

				// Show acceleration structure status
				if (*tlasStructure.handle)
				{
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Acceleration Structures: Built (%zu meshes)", blasStructures.size());
				}
				else
				{
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Acceleration Structures: Not built");
				}

				ImGui::Spacing();
				ImGui::Text("Ray Query Features:");
				ImGui::Checkbox("Enable Reflections", &enableRayQueryReflections);
				ImGui::Checkbox("Enable Transparency/Refraction", &enableRayQueryTransparency);
				ImGui::SliderInt("Max secondary bounces", &rayQueryMaxBounces, 0, 10);
			}

			// === SHARED OPTIONS (BOTH MODES) ===
			ImGui::Separator();
			ImGui::Text("Culling & LOD:");
			if (ImGui::Checkbox("Frustum culling", &enableFrustumCulling))
			{
				// no-op, takes effect immediately
			}
			if (ImGui::Checkbox("Distance LOD (projected-size skip)", &enableDistanceLOD))
			{
			}
			ImGui::SliderFloat("LOD threshold opaque (px)", &lodPixelThresholdOpaque, 0.5f, 8.0f, "%.1f");
			ImGui::SliderFloat("LOD threshold transparent (px)", &lodPixelThresholdTransparent, 0.5f, 12.0f, "%.1f");
			// Anisotropy control (recreate samplers on change)
			{
				float deviceMaxAniso = physicalDevice.getProperties().limits.maxSamplerAnisotropy;
				if (ImGui::SliderFloat("Sampler max anisotropy", &samplerMaxAnisotropy, 1.0f, deviceMaxAniso, "%.1f"))
				{
					// Recreate samplers for all textures to apply new anisotropy
					std::unique_lock<std::shared_mutex> texLock(textureResourcesMutex);
					for (auto &kv : textureResources)
					{
						createTextureSampler(kv.second);
					}
					// Default texture
					createTextureSampler(defaultTextureResources);
				}
			}
			if (lastCullingVisibleCount + lastCullingCulledCount > 0)
			{
				ImGui::Text("Culling: visible=%u, culled=%u", lastCullingVisibleCount, lastCullingCulledCount);
			}

			// Basic tone mapping controls
			ImGui::Separator();
			ImGui::Text("Tone Mapping:");
			ImGui::SliderFloat("Exposure", &exposure, 0.1f, 4.0f, "%.2f");
			ImGui::SliderFloat("Gamma", &gamma, 1.6f, 2.6f, "%.2f");
		}
		ImGui::End();
	}

	// Rasterization rendering: only execute if ray query did not render this frame.
	// Previously this always executed, but now we skip it when ray query mode successfully renders.
	if (!rayQueryRenderedThisFrame)
	{
		// Prepare frustum once per frame
		FrustumPlanes frustum{};
		const bool    doCulling = enableFrustumCulling && camera;
		if (doCulling)
		{
			const glm::mat4 vp = camera->GetProjectionMatrix() * camera->GetViewMatrix();
			frustum            = extractFrustumPlanes(vp);
		}

		lastCullingVisibleCount = 0;
		lastCullingCulledCount  = 0;

		// Optional: render planar reflections first
		if (enablePlanarReflections)
		{
			// Default plane: Y=0 (upwards normal) — replace with component-driven plane later
			glm::vec4 planeWS(0.0f, 1.0f, 0.0f, 0.0f);
			renderReflectionPass(commandBuffers[currentFrame], planeWS, camera, entities);
		}

		for (const auto &uptr : entities)
		{
			Entity *entity = uptr.get();
			if (!entity || !entity->IsActive())
				continue;
			auto meshComponent = entity->GetComponent<MeshComponent>();
			if (!meshComponent)
				continue;

			// Frustum culling
			if (doCulling && meshComponent->HasLocalAABB())
			{
				auto           *tc    = entity->GetComponent<TransformComponent>();
				const glm::mat4 model = tc ? tc->GetModelMatrix() : glm::mat4(1.0f);
				glm::vec3       wmin, wmax;
				transformAABB(model, meshComponent->GetLocalAABBMin(), meshComponent->GetLocalAABBMax(), wmin, wmax);
				if (!aabbIntersectsFrustum(wmin, wmax, frustum))
				{
					lastCullingCulledCount++;
					continue;        // culled early
				}
			}
			lastCullingVisibleCount++;
			bool useBlended = false;
			if (modelLoader && entity->GetName().find("_Material_") != std::string::npos)
			{
				std::string entityName = entity->GetName();
				size_t      tagPos     = entityName.find("_Material_");
				if (tagPos != std::string::npos)
				{
					size_t afterTag = tagPos + std::string("_Material_").size();
					if (afterTag < entityName.length())
					{
						// Entity name format: "modelName_Material_<index>_<materialName>"
						// Find the next underscore after the material index to get the actual material name
						std::string remainder      = entityName.substr(afterTag);
						size_t      nextUnderscore = remainder.find('_');
						if (nextUnderscore != std::string::npos && nextUnderscore + 1 < remainder.length())
						{
							std::string materialName = remainder.substr(nextUnderscore + 1);
							Material   *material     = modelLoader->GetMaterial(materialName);
							// Classify as blended only for true alpha-blend materials, glass or liquids, or high transmission.
							// This avoids shunting most opaque materials into the transparent pass (which skips the off-screen buffer).
							bool isBlendedMat = false;
							if (material)
							{
								bool alphaBlend       = (material->alphaMode == "BLEND");
								bool highTransmission = (material->transmissionFactor > 0.2f);
								bool glassLike        = material->isGlass;
								bool liquidLike       = material->isLiquid;
								isBlendedMat          = alphaBlend || highTransmission || glassLike || liquidLike;
							}
							if (isBlendedMat)
							{
								useBlended = true;
							}
						}
					}
				}
			}

			// Ensure all entities are considered regardless of reflections setting.
			// Previous diagnostic mode skipped non-glass when reflections were ON, which could
			// result in frames with few/no draws and visible black flashes. We no longer filter here.

			// Distance-based LOD: approximate screen-space size of entity's bounds
			if (enableDistanceLOD && camera && meshComponent && meshComponent->HasLocalAABB())
			{
				auto           *tc       = entity->GetComponent<TransformComponent>();
				const glm::mat4 model    = tc ? tc->GetModelMatrix() : glm::mat4(1.0f);
				glm::vec3       localMin = meshComponent->GetLocalAABBMin();
				glm::vec3       localMax = meshComponent->GetLocalAABBMax();
				// Compute world AABB bounds
				glm::vec3 wmin, wmax;
				transformAABB(model, localMin, localMax, wmin, wmax);
				glm::vec3 center  = 0.5f * (wmin + wmax);
				glm::vec3 extents = 0.5f * (wmax - wmin);
				float     radius  = glm::length(extents);
				if (radius > 0.0f)
				{
					glm::vec4 centerVS4 = camera->GetViewMatrix() * glm::vec4(center, 1.0f);
					float     z         = std::abs(centerVS4.z);
					if (z > 1e-3f)
					{
						float fov           = glm::radians(camera->GetFieldOfView());
						float pixelRadius   = (radius * static_cast<float>(swapChainExtent.height)) / (z * 2.0f * std::tan(fov * 0.5f));
						float pixelDiameter = pixelRadius * 2.0f;
						float threshold     = useBlended ? lodPixelThresholdTransparent : lodPixelThresholdOpaque;
						if (pixelDiameter < threshold)
						{
							// Too small to matter; skip adding to draw queues
							continue;
						}
					}
				}
			}
			if (useBlended)
			{
				blendedQueue.push_back(entity);
				blendedSet.insert(entity);
			}
		}

		// Sort transparent entities back-to-front for correct blending of nested glass/liquids
		if (!blendedQueue.empty())
		{
			// Sort by squared distance from the camera in world space.
			// Farther objects must be rendered first so that nearer glass correctly
			// appears in front (standard back-to-front transparency ordering).
			glm::vec3 camPos = camera ? camera->GetPosition() : glm::vec3(0.0f);
			std::ranges::sort(blendedQueue, [this, camPos](Entity *a, Entity *b) {
				auto     *ta  = (a ? a->GetComponent<TransformComponent>() : nullptr);
				auto     *tb  = (b ? b->GetComponent<TransformComponent>() : nullptr);
				glm::vec3 pa  = ta ? ta->GetPosition() : glm::vec3(0.0f);
				glm::vec3 pb  = tb ? tb->GetPosition() : glm::vec3(0.0f);
				float     da2 = glm::length2(pa - camPos);
				float     db2 = glm::length2(pb - camPos);

				// Primary key: distance (farther first)
				if (da2 != db2)
				{
					return da2 > db2;
				}

				// Secondary key: for entities at nearly the same distance, prefer
				// rendering liquid volumes before glass shells so bar glasses look
				// correctly filled. This is a heuristic based on material flags.
				auto classify = [this](Entity *e) {
					bool hasGlass  = false;
					bool hasLiquid = false;
					if (!e || !modelLoader)
						return std::pair<bool, bool>{false, false};

					const std::string &name   = e->GetName();
					size_t             tagPos = name.find("_Material_");
					if (tagPos != std::string::npos)
					{
						size_t afterTag = tagPos + std::string("_Material_").size();
						if (afterTag < name.length())
						{
							std::string remainder      = name.substr(afterTag);
							size_t      nextUnderscore = remainder.find('_');
							if (nextUnderscore != std::string::npos && nextUnderscore + 1 < remainder.length())
							{
								std::string materialName = remainder.substr(nextUnderscore + 1);
								if (Material *m = modelLoader->GetMaterial(materialName))
								{
									hasGlass  = m->isGlass;
									hasLiquid = m->isLiquid;
								}
							}
						}
					}
					return std::pair<bool, bool>{hasGlass, hasLiquid};
				};

				auto [aIsGlass, aIsLiquid] = classify(a);
				auto [bIsGlass, bIsLiquid] = classify(b);

				// If one is liquid and the other is glass at the same distance,
				// render the liquid first (i.e., treat it as slightly farther).
				if (aIsLiquid && bIsGlass && !bIsLiquid)
				{
					return true;        // a (liquid) comes before b (glass)
				}
				if (bIsLiquid && aIsGlass && !aIsLiquid)
				{
					return false;        // b (liquid) comes before a (glass)
				}

				// Fallback to stable ordering when distances and classifications are equal.
				return a < b;
			});
		}

		// Track whether we executed a depth pre-pass this frame (used to choose depth load op and pipeline state)
		bool didOpaqueDepthPrepass = false;

		// Optional Forward+ depth pre-pass for opaque geometry
		if (useForwardPlus)
		{
			// Build list of non-blended entities
			std::vector<Entity *> opaqueEntities;
			opaqueEntities.reserve(entities.size());
			for (const auto &uptr : entities)
			{
				Entity *entity = uptr.get();
				if (!entity || !entity->IsActive() || blendedSet.contains(entity))
					continue;
				auto meshComponent = entity->GetComponent<MeshComponent>();
				if (!meshComponent)
					continue;
				opaqueEntities.push_back(entity);
			}

			if (!opaqueEntities.empty())
			{
				// Transition depth image for attachment write (Sync2)
				vk::ImageMemoryBarrier2 depthBarrier2{
				    .srcStageMask        = vk::PipelineStageFlagBits2::eTopOfPipe,
				    .srcAccessMask       = vk::AccessFlagBits2::eNone,
				    .dstStageMask        = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
				    .dstAccessMask       = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
				    .oldLayout           = vk::ImageLayout::eUndefined,
				    .newLayout           = vk::ImageLayout::eDepthAttachmentOptimal,
				    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				    .image               = *depthImage,
				    .subresourceRange    = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}};
				vk::DependencyInfo depInfoDepth{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &depthBarrier2};
				commandBuffers[currentFrame].pipelineBarrier2(depInfoDepth);

				// Depth-only rendering
				vk::RenderingAttachmentInfo depthOnlyAttachment{.imageView = *depthImageView, .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal, .loadOp = vk::AttachmentLoadOp::eClear, .storeOp = vk::AttachmentStoreOp::eStore, .clearValue = vk::ClearDepthStencilValue{1.0f, 0}};
				vk::RenderingInfo           depthOnlyInfo{.renderArea = vk::Rect2D({0, 0}, swapChainExtent), .layerCount = 1, .colorAttachmentCount = 0, .pColorAttachments = nullptr, .pDepthAttachment = &depthOnlyAttachment};
				commandBuffers[currentFrame].beginRendering(depthOnlyInfo);
				vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
				commandBuffers[currentFrame].setViewport(0, viewport);
				vk::Rect2D scissor({0, 0}, swapChainExtent);
				commandBuffers[currentFrame].setScissor(0, scissor);

				// Bind depth pre-pass pipeline
				if (!(depthPrepassPipeline == nullptr))
				{
					commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *depthPrepassPipeline);
				}

				for (Entity *entity : opaqueEntities)
				{
					auto meshComponent = entity->GetComponent<MeshComponent>();
					auto entityIt      = entityResources.find(entity);
					auto meshIt        = meshResources.find(meshComponent);
					if (!meshComponent || entityIt == entityResources.end() || meshIt == meshResources.end())
						continue;

					// Skip alpha-masked geometry in the depth pre-pass so that depth is not written
					// where fragments would be discarded by alpha test. These will write depth during
					// the opaque color pass using the standard opaque pipeline.
					bool isAlphaMasked = false;
					if (modelLoader && entity->GetName().find("_Material_") != std::string::npos)
					{
						std::string entityName = entity->GetName();
						size_t      tagPos     = entityName.find("_Material_");
						if (tagPos != std::string::npos)
						{
							size_t afterTag = tagPos + std::string("_Material_").size();
							if (afterTag < entityName.length())
							{
								std::string remainder      = entityName.substr(afterTag);
								size_t      nextUnderscore = remainder.find('_');
								if (nextUnderscore != std::string::npos && nextUnderscore + 1 < remainder.length())
								{
									std::string materialName = remainder.substr(nextUnderscore + 1);
									if (Material *m = modelLoader->GetMaterial(materialName))
									{
										isAlphaMasked = (m->alphaMode == "MASK");
									}
								}
							}
						}
					}
					// Fallback: infer mask from baseColor texture alpha usage hint
					if (!isAlphaMasked)
					{
						std::string baseColorPath;
						if (meshComponent)
						{
							if (!meshComponent->GetBaseColorTexturePath().empty())
							{
								baseColorPath = meshComponent->GetBaseColorTexturePath();
							}
							else if (!meshComponent->GetTexturePath().empty())
							{
								baseColorPath = meshComponent->GetTexturePath();
							}
						}
						if (!baseColorPath.empty())
						{
							const std::string                   resolvedBase = ResolveTextureId(baseColorPath);
							std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
							auto                                itTex = textureResources.find(resolvedBase);
							if (itTex != textureResources.end() && itTex->second.alphaMaskedHint)
							{
								isAlphaMasked = true;
							}
						}
					}
					if (isAlphaMasked)
					{
						continue;        // do not write depth for masked foliage in pre-pass
					}

					std::array<vk::Buffer, 2>     buffers = {*meshIt->second.vertexBuffer, *entityIt->second.instanceBuffer};
					std::array<vk::DeviceSize, 2> offsets = {0, 0};
					commandBuffers[currentFrame].bindVertexBuffers(0, buffers, offsets);
					commandBuffers[currentFrame].bindIndexBuffer(*meshIt->second.indexBuffer, 0, vk::IndexType::eUint32);

					updateUniformBuffer(currentFrame, entity, camera);

					auto &descSets = entityIt->second.pbrDescriptorSets;
					if (descSets.empty() || currentFrame >= descSets.size())
						continue;
					commandBuffers[currentFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pbrPipelineLayout, 0, {*descSets[currentFrame]}, {});
					uint32_t instanceCount = std::max(1u, static_cast<uint32_t>(meshComponent->GetInstanceCount()));
					commandBuffers[currentFrame].drawIndexed(meshIt->second.indexCount, instanceCount, 0, 0, 0);
				}

				commandBuffers[currentFrame].endRendering();

				// Barrier to ensure depth is visible for subsequent passes (Sync2)
				vk::ImageMemoryBarrier2 depthToRead2{
				    .srcStageMask        = vk::PipelineStageFlagBits2::eLateFragmentTests,
				    .srcAccessMask       = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
				    .dstStageMask        = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
				    .dstAccessMask       = vk::AccessFlagBits2::eDepthStencilAttachmentRead,
				    .oldLayout           = vk::ImageLayout::eDepthAttachmentOptimal,
				    .newLayout           = vk::ImageLayout::eDepthAttachmentOptimal,
				    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				    .image               = *depthImage,
				    .subresourceRange    = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}};
				vk::DependencyInfo depInfoDepthToRead{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &depthToRead2};
				commandBuffers[currentFrame].pipelineBarrier2(depInfoDepthToRead);

				didOpaqueDepthPrepass = true;
			}

			// Forward+ compute culling based on current camera and screen tiles
			uint32_t tilesX = (swapChainExtent.width + forwardPlusTileSizeX - 1) / forwardPlusTileSizeX;
			uint32_t tilesY = (swapChainExtent.height + forwardPlusTileSizeY - 1) / forwardPlusTileSizeY;

			// Lights already extracted at frame start - use lastFrameLightCount for Forward+ params
			glm::mat4 view = camera->GetViewMatrix();
			glm::mat4 proj = camera->GetProjectionMatrix();
			proj[1][1] *= -1.0f;
			float nearZ = camera->GetNearPlane();
			float farZ  = camera->GetFarPlane();
			updateForwardPlusParams(currentFrame, view, proj, lastFrameLightCount, tilesX, tilesY, forwardPlusSlicesZ, nearZ, farZ);
			// As a last guard before dispatch, make sure compute binding 0 is valid for this frame
			refreshForwardPlusComputeLightsBindingForFrame(currentFrame);

			// Forward+ per-frame debug printing removed

			dispatchForwardPlus(commandBuffers[currentFrame], tilesX, tilesY, forwardPlusSlicesZ);
			// Forward+ debug dumps and tile header prints removed
		}

		// PASS 1: RENDER OPAQUE OBJECTS TO OFF-SCREEN TEXTURE
		// Transition off-screen color from last frame's sampling to attachment write (Sync2)
		vk::ImageMemoryBarrier2 oscToColor2{.srcStageMask        = vk::PipelineStageFlagBits2::eFragmentShader,
		                                    .srcAccessMask       = vk::AccessFlagBits2::eShaderRead,
		                                    .dstStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		                                    .dstAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite,
		                                    .oldLayout           = vk::ImageLayout::eShaderReadOnlyOptimal,
		                                    .newLayout           = vk::ImageLayout::eColorAttachmentOptimal,
		                                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		                                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		                                    .image               = *opaqueSceneColorImage,
		                                    .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
		vk::DependencyInfo      depOscToColor{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &oscToColor2};
		commandBuffers[currentFrame].pipelineBarrier2(depOscToColor);
		// Clear the off-screen target at the start of opaque rendering to a neutral black background
		vk::RenderingAttachmentInfo colorAttachment{.imageView = *opaqueSceneColorImageView, .imageLayout = vk::ImageLayout::eColorAttachmentOptimal, .loadOp = vk::AttachmentLoadOp::eClear, .storeOp = vk::AttachmentStoreOp::eStore, .clearValue = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f})};
		depthAttachment.imageView = *depthImageView;
		// Load depth only if we actually performed a pre-pass (and not in opaque-only debug which intentionally ignores transparency ordering)
		depthAttachment.loadOp = (didOpaqueDepthPrepass) ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear;
		vk::RenderingInfo passInfo{.renderArea = vk::Rect2D({0, 0}, swapChainExtent), .layerCount = 1, .colorAttachmentCount = 1, .pColorAttachments = &colorAttachment, .pDepthAttachment = &depthAttachment};
		commandBuffers[currentFrame].beginRendering(passInfo);
		vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
		commandBuffers[currentFrame].setViewport(0, viewport);
		vk::Rect2D scissor({0, 0}, swapChainExtent);
		commandBuffers[currentFrame].setScissor(0, scissor);
		{
			uint32_t opaqueDrawsThisPass = 0;
			for (const auto &uptr : entities)
			{
				Entity *entity = uptr.get();
				if (!entity || !entity->IsActive() || (blendedSet.contains(entity)))
					continue;
				auto meshComponent = entity->GetComponent<MeshComponent>();
				if (!meshComponent)
					continue;
				bool                      useBasic         = imguiSystem && !imguiSystem->IsPBREnabled();
				vk::raii::Pipeline       *selectedPipeline = nullptr;
				vk::raii::PipelineLayout *selectedLayout   = nullptr;
				if (useBasic)
				{
					selectedPipeline = &graphicsPipeline;
					selectedLayout   = &pipelineLayout;
				}
				else
				{
					// Determine if this entity uses alpha masking so we can bypass the post-prepass
					// read-only pipeline and use the normal depth-writing opaque pipeline instead.
					bool isAlphaMaskedOpaque = false;
					if (modelLoader && entity->GetName().find("_Material_") != std::string::npos)
					{
						std::string entityName = entity->GetName();
						size_t      tagPos     = entityName.find("_Material_");
						if (tagPos != std::string::npos)
						{
							size_t afterTag = tagPos + std::string("_Material_").size();
							if (afterTag < entityName.length())
							{
								std::string remainder      = entityName.substr(afterTag);
								size_t      nextUnderscore = remainder.find('_');
								if (nextUnderscore != std::string::npos && nextUnderscore + 1 < remainder.length())
								{
									std::string materialName = remainder.substr(nextUnderscore + 1);
									if (Material *m = modelLoader->GetMaterial(materialName))
									{
										isAlphaMaskedOpaque = (m->alphaMode == "MASK");
									}
								}
							}
						}
					}
					// Fallback based on texture hint if material flag not set
					if (!isAlphaMaskedOpaque)
					{
						std::string baseColorPath;
						if (meshComponent)
						{
							if (!meshComponent->GetBaseColorTexturePath().empty())
							{
								baseColorPath = meshComponent->GetBaseColorTexturePath();
							}
							else if (!meshComponent->GetTexturePath().empty())
							{
								baseColorPath = meshComponent->GetTexturePath();
							}
						}
						if (!baseColorPath.empty())
						{
							const std::string                   resolvedBase = ResolveTextureId(baseColorPath);
							std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
							auto                                itTex = textureResources.find(resolvedBase);
							if (itTex != textureResources.end() && itTex->second.alphaMaskedHint)
							{
								isAlphaMaskedOpaque = true;
							}
						}
					}
					// If masked, we need depth writes with alpha test; otherwise, after-prepass read-only is fine.
					if (isAlphaMaskedOpaque)
					{
						selectedPipeline = &pbrGraphicsPipeline;        // writes depth, compare Less
					}
					else
					{
						selectedPipeline = didOpaqueDepthPrepass && (pbrPrepassGraphicsPipeline != nullptr) ? &pbrPrepassGraphicsPipeline : &pbrGraphicsPipeline;
					}
					selectedLayout = &pbrPipelineLayout;
				}
				if (currentPipeline != selectedPipeline)
				{
					commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, **selectedPipeline);
					currentPipeline = selectedPipeline;
					currentLayout   = selectedLayout;
				}
				auto meshIt   = meshResources.find(meshComponent);
				auto entityIt = entityResources.find(entity);
				if (meshIt == meshResources.end() || entityIt == entityResources.end())
					continue;
				std::array<vk::Buffer, 2>     buffers = {*meshIt->second.vertexBuffer, *entityIt->second.instanceBuffer};
				std::array<vk::DeviceSize, 2> offsets = {0, 0};
				commandBuffers[currentFrame].bindVertexBuffers(0, buffers, offsets);
				commandBuffers[currentFrame].bindIndexBuffer(*meshIt->second.indexBuffer, 0, vk::IndexType::eUint32);
				updateUniformBuffer(currentFrame, entity, camera);
				auto *descSetsPtr = useBasic ? &entityIt->second.basicDescriptorSets : &entityIt->second.pbrDescriptorSets;
				if (descSetsPtr->empty() || currentFrame >= descSetsPtr->size())
				{
					// Never create or update descriptor sets during command buffer recording.
					// Mark this entity dirty so the safe point will initialize its descriptors next frame.
					MarkEntityDescriptorsDirty(entity);
					static bool printedOnceMissingSets = false;
					if (!printedOnceMissingSets)
					{
						std::cerr << "[Descriptors] Descriptor sets missing for '" << entity->GetName() << "' — deferring to safe point, draw skipped this frame" << std::endl;
						printedOnceMissingSets = true;
					}
					continue;
				}
				// (binding of descriptor sets happens below using descSetsPtr for the chosen pipeline)
				if (!useBasic)
				{
					MaterialProperties pushConstants{};
					// Sensible defaults for entities without explicit material
					pushConstants.baseColorFactor              = glm::vec4(1.0f);
					pushConstants.metallicFactor               = 0.0f;
					pushConstants.roughnessFactor              = 1.0f;
					pushConstants.baseColorTextureSet          = 0;        // sample bound baseColor (falls back to shared default if none)
					pushConstants.physicalDescriptorTextureSet = 0;        // default to sampling metallic-roughness on binding 2
					pushConstants.normalTextureSet             = -1;
					pushConstants.occlusionTextureSet          = -1;
					pushConstants.emissiveTextureSet           = -1;
					pushConstants.alphaMask                    = 0.0f;
					pushConstants.alphaMaskCutoff              = 0.5f;
					pushConstants.emissiveFactor               = glm::vec3(0.0f);
					pushConstants.emissiveStrength             = 1.0f;
					pushConstants.hasEmissiveStrengthExtension = false;        // Default entities don't have emissive strength extension
					pushConstants.transmissionFactor           = 0.0f;
					pushConstants.useSpecGlossWorkflow         = 0;
					pushConstants.glossinessFactor             = 0.0f;
					pushConstants.specularFactor               = glm::vec3(1.0f);
					// pushConstants.ior already 1.5f default
					// If we don't resolve a material below, still show emissive textures if bound at set 5
					if (meshComponent && !meshComponent->GetEmissiveTexturePath().empty())
					{
						pushConstants.emissiveTextureSet           = 0;
						pushConstants.emissiveFactor               = glm::vec3(1.0f);
						pushConstants.emissiveStrength             = 1.0f;
						pushConstants.hasEmissiveStrengthExtension = false;
					}
					if (modelLoader && entity->GetName().find("_Material_") != std::string::npos)
					{
						std::string entityName = entity->GetName();
						size_t      tagPos     = entityName.find("_Material_");
						if (tagPos != std::string::npos)
						{
							size_t afterTag = tagPos + std::string("_Material_").size();
							if (afterTag < entityName.length())
							{
								// Entity name format: "modelName_Material_<index>_<materialName>"
								// Find the next underscore after the material index to get the actual material name
								std::string remainder      = entityName.substr(afterTag);
								size_t      nextUnderscore = remainder.find('_');
								if (nextUnderscore != std::string::npos && nextUnderscore + 1 < remainder.length())
								{
									std::string materialName = remainder.substr(nextUnderscore + 1);
									Material   *material     = modelLoader->GetMaterial(materialName);
									if (material)
									{
										// Base factors
										pushConstants.baseColorFactor = glm::vec4(material->albedo, material->alpha);
										pushConstants.metallicFactor  = material->metallic;
										pushConstants.roughnessFactor = material->roughness;

										// Texture set flags (-1 = no texture)
										pushConstants.baseColorTextureSet = material->albedoTexturePath.empty() ? -1 : 0;
										// physical descriptor: MR or SpecGloss
										if (material->useSpecularGlossiness)
										{
											pushConstants.useSpecGlossWorkflow         = 1;
											pushConstants.physicalDescriptorTextureSet = material->specGlossTexturePath.empty() ? -1 : 0;
											pushConstants.glossinessFactor             = material->glossinessFactor;
											pushConstants.specularFactor               = material->specularFactor;
										}
										else
										{
											pushConstants.useSpecGlossWorkflow         = 0;
											pushConstants.physicalDescriptorTextureSet = material->metallicRoughnessTexturePath.empty() ? -1 : 0;
										}
										pushConstants.normalTextureSet    = material->normalTexturePath.empty() ? -1 : 0;
										pushConstants.occlusionTextureSet = material->occlusionTexturePath.empty() ? -1 : 0;
										pushConstants.emissiveTextureSet  = material->emissiveTexturePath.empty() ? -1 : 0;

										// Emissive and transmission/IOR
										pushConstants.emissiveFactor   = material->emissive;
										pushConstants.emissiveStrength = material->emissiveStrength;
										// Heuristic: consider emissive strength extension present when strength != 1.0
										pushConstants.hasEmissiveStrengthExtension = (std::abs(material->emissiveStrength - 1.0f) > 1e-6f);
										pushConstants.transmissionFactor           = material->transmissionFactor;
										pushConstants.ior                          = material->ior;

										// Alpha mask handling
										pushConstants.alphaMask       = (material->alphaMode == "MASK") ? 1.0f : 0.0f;
										pushConstants.alphaMaskCutoff = material->alphaCutoff;
									}
								}
							}
						}
					}
					// If no explicit MASK from a material, infer it from the baseColor texture's alpha usage
					if (pushConstants.alphaMask < 0.5f)
					{
						std::string baseColorPath;
						if (meshComponent)
						{
							if (!meshComponent->GetBaseColorTexturePath().empty())
							{
								baseColorPath = meshComponent->GetBaseColorTexturePath();
							}
							else if (!meshComponent->GetTexturePath().empty())
							{
								baseColorPath = meshComponent->GetTexturePath();
							}
							else
							{
								baseColorPath = SHARED_DEFAULT_ALBEDO_ID;
							}
						}
						else
						{
							baseColorPath = SHARED_DEFAULT_ALBEDO_ID;
						}
						// Avoid inferring MASK from the shared default albedo (semi-transparent placeholder)
						if (baseColorPath != SHARED_DEFAULT_ALBEDO_ID)
						{
							const std::string                   resolvedBase = ResolveTextureId(baseColorPath);
							std::shared_lock<std::shared_mutex> texLock(textureResourcesMutex);
							auto                                itTex = textureResources.find(resolvedBase);
							if (itTex != textureResources.end() && itTex->second.alphaMaskedHint)
							{
								pushConstants.alphaMask       = 1.0f;
								pushConstants.alphaMaskCutoff = 0.5f;
							}
						}
					}
					commandBuffers[currentFrame].pushConstants<MaterialProperties>(**currentLayout, vk::ShaderStageFlagBits::eFragment, 0, {pushConstants});
				}
				// Bind descriptor sets for the selected pipeline
				if (useBasic)
				{
					commandBuffers[currentFrame].bindDescriptorSets(
					    vk::PipelineBindPoint::eGraphics,
					    **selectedLayout,
					    0,
					    {*(*descSetsPtr)[currentFrame]},
					    {});
				}
				else
				{
					// Opaque PBR binds set0 (PBR) and set1 (scene color fallback for transparency path, not used here but layout expects it)
					vk::DescriptorSet set1Opaque = *transparentFallbackDescriptorSets[currentFrame];
					commandBuffers[currentFrame].bindDescriptorSets(
					    vk::PipelineBindPoint::eGraphics,
					    **selectedLayout,
					    0,
					    {*(*descSetsPtr)[currentFrame], set1Opaque},
					    {});
				}
				uint32_t instanceCount = std::max(1u, static_cast<uint32_t>(meshComponent->GetInstanceCount()));
				commandBuffers[currentFrame].drawIndexed(meshIt->second.indexCount, instanceCount, 0, 0, 0);
				++opaqueDrawsThisPass;
			}
		}
		commandBuffers[currentFrame].endRendering();
		// PASS 1b: PRESENT – composite path
		{
			// Transition off-screen to SHADER_READ for sampling (Sync2)
			vk::ImageMemoryBarrier2 opaqueToSample2{.srcStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			                                        .srcAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite,
			                                        .dstStageMask        = vk::PipelineStageFlagBits2::eFragmentShader,
			                                        .dstAccessMask       = vk::AccessFlagBits2::eShaderRead,
			                                        .oldLayout           = vk::ImageLayout::eColorAttachmentOptimal,
			                                        .newLayout           = vk::ImageLayout::eShaderReadOnlyOptimal,
			                                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			                                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			                                        .image               = *opaqueSceneColorImage,
			                                        .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
			vk::DependencyInfo      depOpaqueToSample{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &opaqueToSample2};
			commandBuffers[currentFrame].pipelineBarrier2(depOpaqueToSample);

			// Make the swapchain image ready for color attachment output and clear it (Sync2)
			vk::ImageMemoryBarrier2 swapchainToColor2{.srcStageMask        = vk::PipelineStageFlagBits2::eTopOfPipe,
			                                          .srcAccessMask       = vk::AccessFlagBits2::eNone,
			                                          .dstStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			                                          .dstAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite,
			                                          .oldLayout           = vk::ImageLayout::eUndefined,
			                                          .newLayout           = vk::ImageLayout::eColorAttachmentOptimal,
			                                          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			                                          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			                                          .image               = swapChainImages[imageIndex],
			                                          .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
			vk::DependencyInfo      depSwapchainToColor{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &swapchainToColor2};
			commandBuffers[currentFrame].pipelineBarrier2(depSwapchainToColor);

			// Begin rendering to swapchain for composite
			colorAttachments[0].imageView = *swapChainImageViews[imageIndex];
			colorAttachments[0].loadOp    = vk::AttachmentLoadOp::eClear;           // clear before composing base layer (full-screen composite overwrites all pixels)
			depthAttachment.loadOp        = vk::AttachmentLoadOp::eDontCare;        // no depth for composite
			renderingInfo.renderArea      = vk::Rect2D({0, 0}, swapChainExtent);
			// IMPORTANT: Composite pass does not use a depth attachment. Avoid binding it to satisfy dynamic rendering VUIDs.
			auto savedDepthPtr             = renderingInfo.pDepthAttachment;        // save to restore later
			renderingInfo.pDepthAttachment = nullptr;
			commandBuffers[currentFrame].beginRendering(renderingInfo);

			// Bind composite pipeline
			if (compositePipeline != nullptr)
			{
				commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *compositePipeline);
			}
			vk::Viewport viewport2(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
			commandBuffers[currentFrame].setViewport(0, viewport2);
			vk::Rect2D scissor2({0, 0}, swapChainExtent);
			commandBuffers[currentFrame].setScissor(0, scissor2);

			// Bind descriptor set 0 for the composite (reuse transparent descriptor set which points to off-screen color)
			vk::DescriptorSet setComposite = transparentDescriptorSets.empty() ? *transparentFallbackDescriptorSets[currentFrame] : *transparentDescriptorSets[currentFrame];
			commandBuffers[currentFrame].bindDescriptorSets(
			    vk::PipelineBindPoint::eGraphics,
			    *compositePipelineLayout,
			    0,
			    {setComposite},
			    {});

			// Push exposure/gamma and sRGB flag
			struct CompositePush
			{
				float exposure;
				float gamma;
				int   outputIsSRGB;
				float _pad;
			} pc{};
			pc.exposure     = std::clamp(this->exposure, 0.2f, 4.0f);
			pc.gamma        = this->gamma;
			pc.outputIsSRGB = (swapChainImageFormat == vk::Format::eR8G8B8A8Srgb || swapChainImageFormat == vk::Format::eB8G8R8A8Srgb) ? 1 : 0;
			commandBuffers[currentFrame].pushConstants<CompositePush>(*compositePipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pc);

			// Draw fullscreen triangle
			commandBuffers[currentFrame].draw(3, 1, 0, 0);

			commandBuffers[currentFrame].endRendering();
			// Restore depth attachment pointer for subsequent passes
			renderingInfo.pDepthAttachment = savedDepthPtr;
		}
		// PASS 2: RENDER TRANSPARENT OBJECTS TO THE SWAPCHAIN
		{
			// Ensure depth attachment is bound again for the transparent pass
			renderingInfo.pDepthAttachment = &depthAttachment;
			colorAttachments[0].imageView  = *swapChainImageViews[imageIndex];
			colorAttachments[0].loadOp     = vk::AttachmentLoadOp::eLoad;
			depthAttachment.loadOp         = vk::AttachmentLoadOp::eLoad;
			renderingInfo.renderArea       = vk::Rect2D({0, 0}, swapChainExtent);
			commandBuffers[currentFrame].beginRendering(renderingInfo);
			commandBuffers[currentFrame].setViewport(0, viewport);
			commandBuffers[currentFrame].setScissor(0, scissor);

			if (!blendedQueue.empty())
			{
				currentLayout = &pbrTransparentPipelineLayout;

				// Track currently bound pipeline so we only rebind when needed
				vk::raii::Pipeline *activeTransparentPipeline = nullptr;

				for (Entity *entity : blendedQueue)
				{
					auto meshComponent = entity->GetComponent<MeshComponent>();
					auto entityIt      = entityResources.find(entity);
					auto meshIt        = meshResources.find(meshComponent);
					if (!meshComponent || entityIt == entityResources.end() || meshIt == meshResources.end())
						continue;

					// Resolve material for this entity (if any)
					Material *material = nullptr;
					if (modelLoader && entity->GetName().find("_Material_") != std::string::npos)
					{
						std::string entityName = entity->GetName();
						size_t      tagPos     = entityName.find("_Material_");
						if (tagPos != std::string::npos)
						{
							size_t afterTag = tagPos + std::string("_Material_").size();
							if (afterTag < entityName.length())
							{
								// Entity name format: "modelName_Material_<index>_<materialName>"
								// Find the next underscore after the material index to get the actual material name
								std::string remainder      = entityName.substr(afterTag);
								size_t      nextUnderscore = remainder.find('_');
								if (nextUnderscore != std::string::npos && nextUnderscore + 1 < remainder.length())
								{
									std::string materialName = remainder.substr(nextUnderscore + 1);
									material                 = modelLoader->GetMaterial(materialName);
								}
							}
						}
					}

					// Choose pipeline: specialized glass pipeline for architectural glass,
					// otherwise the generic blended PBR pipeline.
					bool                useGlassPipeline = material && material->isGlass;
					vk::raii::Pipeline *desiredPipeline  = useGlassPipeline ? &glassGraphicsPipeline : &pbrBlendGraphicsPipeline;
					if (desiredPipeline != activeTransparentPipeline)
					{
						commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, **desiredPipeline);
						activeTransparentPipeline = desiredPipeline;
					}

					std::array<vk::Buffer, 2>     buffers = {*meshIt->second.vertexBuffer, *entityIt->second.instanceBuffer};
					std::array<vk::DeviceSize, 2> offsets = {0, 0};
					commandBuffers[currentFrame].bindVertexBuffers(0, buffers, offsets);
					commandBuffers[currentFrame].bindIndexBuffer(*meshIt->second.indexBuffer, 0, vk::IndexType::eUint32);
					updateUniformBuffer(currentFrame, entity, camera);

					auto &pbrDescSets = entityIt->second.pbrDescriptorSets;
					if (pbrDescSets.empty() || currentFrame >= pbrDescSets.size())
						continue;

					// Bind PBR (set 0) and scene color (set 1). If primary set 1 is unavailable, use fallback.
					vk::DescriptorSet set1 = transparentDescriptorSets.empty() ? *transparentFallbackDescriptorSets[currentFrame] : *transparentDescriptorSets[currentFrame];
					commandBuffers[currentFrame].bindDescriptorSets(
					    vk::PipelineBindPoint::eGraphics,
					    **currentLayout,
					    0,
					    {*pbrDescSets[currentFrame], set1},
					    {});

					MaterialProperties pushConstants{};
					// Sensible defaults for entities without explicit material
					pushConstants.baseColorFactor              = glm::vec4(1.0f);
					pushConstants.metallicFactor               = 0.0f;
					pushConstants.roughnessFactor              = 1.0f;
					pushConstants.baseColorTextureSet          = 0;        // sample bound baseColor (falls back to shared default if none)
					pushConstants.physicalDescriptorTextureSet = 0;        // default to sampling metallic-roughness on binding 2
					pushConstants.normalTextureSet             = -1;
					pushConstants.occlusionTextureSet          = -1;
					pushConstants.emissiveTextureSet           = -1;
					pushConstants.alphaMask                    = 0.0f;
					pushConstants.alphaMaskCutoff              = 0.5f;
					pushConstants.emissiveFactor               = glm::vec3(0.0f);
					pushConstants.emissiveStrength             = 1.0f;
					pushConstants.hasEmissiveStrengthExtension = false;
					pushConstants.transmissionFactor           = 0.0f;
					pushConstants.useSpecGlossWorkflow         = 0;
					pushConstants.glossinessFactor             = 0.0f;
					pushConstants.specularFactor               = glm::vec3(1.0f);
					// pushConstants.ior already 1.5f default
					if (material)
					{
						// Base factors
						pushConstants.baseColorFactor = glm::vec4(material->albedo, material->alpha);
						pushConstants.metallicFactor  = material->metallic;
						pushConstants.roughnessFactor = material->roughness;

						// Texture set flags (-1 = no texture)
						pushConstants.baseColorTextureSet = material->albedoTexturePath.empty() ? -1 : 0;
						if (material->useSpecularGlossiness)
						{
							pushConstants.useSpecGlossWorkflow         = 1;
							pushConstants.physicalDescriptorTextureSet = material->specGlossTexturePath.empty() ? -1 : 0;
							pushConstants.glossinessFactor             = material->glossinessFactor;
							pushConstants.specularFactor               = material->specularFactor;
						}
						else
						{
							pushConstants.useSpecGlossWorkflow         = 0;
							pushConstants.physicalDescriptorTextureSet = material->metallicRoughnessTexturePath.empty() ? -1 : 0;
						}
						pushConstants.normalTextureSet    = material->normalTexturePath.empty() ? -1 : 0;
						pushConstants.occlusionTextureSet = material->occlusionTexturePath.empty() ? -1 : 0;
						pushConstants.emissiveTextureSet  = material->emissiveTexturePath.empty() ? -1 : 0;

						// Emissive and transmission/IOR
						pushConstants.emissiveFactor               = material->emissive;
						pushConstants.emissiveStrength             = material->emissiveStrength;
						pushConstants.hasEmissiveStrengthExtension = false;        // Material has emissive strength data
						pushConstants.transmissionFactor           = material->transmissionFactor;
						pushConstants.ior                          = material->ior;

						// Alpha mask handling
						pushConstants.alphaMask       = (material->alphaMode == "MASK") ? 1.0f : 0.0f;
						pushConstants.alphaMaskCutoff = material->alphaCutoff;

						// For bar liquids and similar volumes, we want the fill to be
						// clearly visible rather than fully transmissive. For these
						// materials, disable the transmission branch in the PBR shader
						// and treat them as regular alpha-blended PBR surfaces.
						if (material->isLiquid)
						{
							pushConstants.transmissionFactor = 0.0f;
						}
					}
					commandBuffers[currentFrame].pushConstants<MaterialProperties>(**currentLayout, vk::ShaderStageFlagBits::eFragment, 0, {pushConstants});
					uint32_t instanceCountT = std::max(1u, static_cast<uint32_t>(meshComponent->GetInstanceCount()));
					commandBuffers[currentFrame].drawIndexed(meshIt->second.indexCount, instanceCountT, 0, 0, 0);
				}
			}
			// End transparent rendering pass before any layout transitions (even if no transparent draws)
			commandBuffers[currentFrame].endRendering();
		}

		{
			// Screenshot and final present transition are handled in rasterization path only
			// Ray query path handles these separately

			// Final layout transition for present (rasterization path only)
			{
				vk::ImageMemoryBarrier2 presentBarrier2{.srcStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
				                                        .srcAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite,
				                                        .dstStageMask        = vk::PipelineStageFlagBits2::eNone,
				                                        .dstAccessMask       = {},
				                                        .oldLayout           = vk::ImageLayout::eColorAttachmentOptimal,
				                                        .newLayout           = vk::ImageLayout::ePresentSrcKHR,
				                                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				                                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				                                        .image               = swapChainImages[imageIndex],
				                                        .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
				vk::DependencyInfo      depToPresentFinal{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &presentBarrier2};
				commandBuffers[currentFrame].pipelineBarrier2(depToPresentFinal);
				if (imageIndex < swapChainImageLayouts.size())
					swapChainImageLayouts[imageIndex] = presentBarrier2.newLayout;
			}
		}
	}        // skip rasterization when ray query has rendered

	// Render ImGui UI overlay AFTER rasterization/ray query (must always execute regardless of render mode)
	// ImGui expects Render() to be called every frame after NewFrame() - skipping it causes hangs
	if (imguiSystem)
	{
		// When ray query renders, swapchain is in PRESENT layout with valid content.
		// When rasterization renders, swapchain is also in PRESENT layout with valid content.
		// Transition to COLOR_ATTACHMENT with loadOp=eLoad to preserve existing pixels for ImGui overlay.
		vk::ImageMemoryBarrier2 presentToColor{
		    .srcStageMask        = vk::PipelineStageFlagBits2::eBottomOfPipe,
		    .srcAccessMask       = vk::AccessFlagBits2::eNone,
		    .dstStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		    .dstAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite,
		    .oldLayout           = (imageIndex < swapChainImageLayouts.size()) ? swapChainImageLayouts[imageIndex] : vk::ImageLayout::eUndefined,
		    .newLayout           = vk::ImageLayout::eColorAttachmentOptimal,
		    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .image               = swapChainImages[imageIndex],
		    .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
		vk::DependencyInfo depInfo{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &presentToColor};
		commandBuffers[currentFrame].pipelineBarrier2(depInfo);
		if (imageIndex < swapChainImageLayouts.size())
			swapChainImageLayouts[imageIndex] = presentToColor.newLayout;

		// Begin a dedicated render pass for ImGui (UI overlay)
		vk::RenderingAttachmentInfo imguiColorAttachment{
		    .imageView   = *swapChainImageViews[imageIndex],
		    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		    .loadOp      = vk::AttachmentLoadOp::eLoad,        // Load existing content
		    .storeOp     = vk::AttachmentStoreOp::eStore};
		vk::RenderingInfo imguiRenderingInfo{
		    .renderArea           = vk::Rect2D({0, 0}, swapChainExtent),
		    .layerCount           = 1,
		    .colorAttachmentCount = 1,
		    .pColorAttachments    = &imguiColorAttachment,
		    .pDepthAttachment     = nullptr};
		commandBuffers[currentFrame].beginRendering(imguiRenderingInfo);

		imguiSystem->Render(commandBuffers[currentFrame], currentFrame);

		commandBuffers[currentFrame].endRendering();

		// Transition swapchain back to PRESENT layout after ImGui renders
		vk::ImageMemoryBarrier2 colorToPresent{
		    .srcStageMask        = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		    .srcAccessMask       = vk::AccessFlagBits2::eColorAttachmentWrite,
		    .dstStageMask        = vk::PipelineStageFlagBits2::eBottomOfPipe,
		    .dstAccessMask       = vk::AccessFlagBits2::eNone,
		    .oldLayout           = vk::ImageLayout::eColorAttachmentOptimal,
		    .newLayout           = vk::ImageLayout::ePresentSrcKHR,
		    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .image               = swapChainImages[imageIndex],
		    .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
		vk::DependencyInfo depInfoBack{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &colorToPresent};
		commandBuffers[currentFrame].pipelineBarrier2(depInfoBack);
		if (imageIndex < swapChainImageLayouts.size())
			swapChainImageLayouts[imageIndex] = colorToPresent.newLayout;
	}

	commandBuffers[currentFrame].end();
	isRecordingCmd.store(false, std::memory_order_relaxed);

	// Submit and present (Synchronization 2)
	uint64_t uploadsValueToWait = uploadTimelineLastSubmitted.load(std::memory_order_relaxed);

	// Use acquireSemaphoreIndex for imageAvailable semaphore (same as we used in acquireNextImage)
	// Use imageIndex for renderFinished semaphore (matches the image being presented)

	std::array<vk::SemaphoreSubmitInfo, 2> waitInfos = {
	    vk::SemaphoreSubmitInfo{.semaphore   = *imageAvailableSemaphores[acquireSemaphoreIndex],
	                            .value       = 0,
	                            .stageMask   = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
	                            .deviceIndex = 0},
	    vk::SemaphoreSubmitInfo{.semaphore   = *uploadsTimeline,
	                            .value       = uploadsValueToWait,
	                            .stageMask   = vk::PipelineStageFlagBits2::eFragmentShader,
	                            .deviceIndex = 0}};

	vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = *commandBuffers[currentFrame], .deviceMask = 0};
	vk::SemaphoreSubmitInfo     signalInfo{.semaphore = *renderFinishedSemaphores[imageIndex], .value = 0, .stageMask = vk::PipelineStageFlagBits2::eAllGraphics, .deviceIndex = 0};
	vk::SubmitInfo2             submit2{
	                .waitSemaphoreInfoCount   = static_cast<uint32_t>(waitInfos.size()),
	                .pWaitSemaphoreInfos      = waitInfos.data(),
	                .commandBufferInfoCount   = 1,
	                .pCommandBufferInfos      = &cmdInfo,
	                .signalSemaphoreInfoCount = 1,
	                .pSignalSemaphoreInfos    = &signalInfo};

	if (framebufferResized.load(std::memory_order_relaxed))
	{
		vk::SubmitInfo2 emptySubmit2{};
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			graphicsQueue.submit2(emptySubmit2, *inFlightFences[currentFrame]);
		}
		recreateSwapChain();
		return;
	}

	// Update watchdog BEFORE queue submit because submit can block waiting for GPU
	// This proves frame CPU work is complete even if GPU queue is busy
	lastFrameUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);

	{
		std::lock_guard<std::mutex> lock(queueMutex);
		graphicsQueue.submit2(submit2, *inFlightFences[currentFrame]);
	}

	vk::PresentInfoKHR presentInfo{.waitSemaphoreCount = 1, .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex], .swapchainCount = 1, .pSwapchains = &*swapChain, .pImageIndices = &imageIndex};
	try
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		result.result = presentQueue.presentKHR(presentInfo);
	}
	catch (const vk::OutOfDateKHRError &)
	{
		framebufferResized.store(true, std::memory_order_relaxed);
	}
	if (result.result == vk::Result::eErrorOutOfDateKHR || result.result == vk::Result::eSuboptimalKHR || framebufferResized.load(std::memory_order_relaxed))
	{
		framebufferResized.store(false, std::memory_order_relaxed);
		recreateSwapChain();
	}
	else if (result.result != vk::Result::eSuccess)
	{
		throw std::runtime_error("Failed to present swap chain image");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Public toggle APIs for planar reflections (keyboard/UI)
void Renderer::SetPlanarReflectionsEnabled(bool enabled)
{
	// Flip mode and mark resources dirty so RTs are created/destroyed at the next safe point
	enablePlanarReflections  = enabled;
	reflectionResourcesDirty = true;
}

void Renderer::TogglePlanarReflections()
{
	SetPlanarReflectionsEnabled(!enablePlanarReflections);
}
