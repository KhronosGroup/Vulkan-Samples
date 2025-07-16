/*
* Copyright 2020 NVIDIA Corporation.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <vector>
#include <iostream>
#include <vulkan_interfaces.h>
#include "pattern.h"
#include <shaderc/shaderc.hpp>

#include "VulkanVideoUtils.h"
#include <nvidia_utils/vulkan/ycbcrvkinfo.h>

// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
      std::cerr << "VkVideoUtils: " << "File " << __FILE__ << "line " <<  __LINE__; \
    assert(false);                                                    \
  }

// A macro to check value is VK_SUCCESS
// Used also for non-vulkan functions but return VK_SUCCESS
#define VK_CHECK(x) CALL_VK(x)

#define VK_LITERAL_TO_STRING_INTERNAL(x)    #x
#define VK_LITERAL_TO_STRING(x) VK_LITERAL_TO_STRING_INTERNAL(x)

namespace vulkanVideoUtils {

using namespace Pattern;

void VulkanSwapchainInfo::CreateSwapChain(VkSwapchainKHR swapchain)
{
    if (mVerbose) std::cout << "VkVideoUtils: " << "Enter Function: " << __FUNCTION__ <<  "File " << __FILE__ << "line " <<  __LINE__ << std::endl;

    mInstance = VulkanDeviceContext::GetThe()->getInstance();

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    VkAndroidSurfaceCreateInfoKHR createInfo = VkAndroidSurfaceCreateInfoKHR();
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.window = platformWindow;
    CALL_VK(vk::CreateAndroidSurfaceKHR(mInstance,
                                      &createInfo, nullptr,
                                      &mSurface));
#endif // VK_USE_PLATFORM_ANDROID_KHR

    // **********************************************************
    // Get the surface capabilities because:
    //   - It contains the minimal and max length of the chain, we will need it
    //   - It's necessary to query the supported surface format (R8G8B8A8 for
    //   instance ...)
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VulkanDeviceContext::GetThe()->GetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanDeviceContext::GetThe()->getPhysicalDevice(),
            mSurface,
            &surfaceCapabilities);
    // Query the list of supported surface format and choose one we like
    uint32_t formatCount = 0;
    VulkanDeviceContext::GetThe()->GetPhysicalDeviceSurfaceFormatsKHR(VulkanDeviceContext::GetThe()->getPhysicalDevice(),
                                         mSurface,
                                         &formatCount, nullptr);
    VkSurfaceFormatKHR *formats = new VkSurfaceFormatKHR[formatCount];
    VulkanDeviceContext::GetThe()->GetPhysicalDeviceSurfaceFormatsKHR(VulkanDeviceContext::GetThe()->getPhysicalDevice(),
                                         mSurface,
                                         &formatCount, formats);
    std::cout << "VkVideoUtils: " << "VulkanSwapchainInfo - got " << formatCount << "surface formats";

    uint32_t chosenFormat;
    for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++) {
        if (formats[chosenFormat].format == VK_FORMAT_R8G8B8A8_UNORM) break;
    }
    assert(chosenFormat < formatCount);

    mDisplaySize = surfaceCapabilities.currentExtent;
    mDisplayFormat = formats[chosenFormat].format;

#if 0
    // **********************************************************
    // Create a swap chain (here we choose the minimum available number of surface
    // in the chain)
    VkSwapchainCreateInfoKHR swapchainCreateInfo = VkSwapchainCreateInfoKHR();
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.surface = mSurface;
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
    swapchainCreateInfo.imageFormat = formats[chosenFormat].format;
    swapchainCreateInfo.imageColorSpace = formats[chosenFormat].colorSpace;
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 1;
    swapchainCreateInfo.pQueueFamilyIndices = &vkDevCtx->GetGfxQueueFamilyIdx();
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    swapchainCreateInfo.clipped = VK_FALSE;
    CALL_VK(m_vkDevCtx->CreateSwapchainKHR(VulkanDeviceContext::GetThe()->getDevice(),
                                 &swapchainCreateInfo, nullptr,
                                 &mSwapchain));
#endif
    delete[] formats;

    mSwapchain = swapchain;

    // Get the length of the created swap chain
    CALL_VK(VulkanDeviceContext::GetThe()->GetSwapchainImagesKHR(
                VulkanDeviceContext::GetThe()->getDevice(), mSwapchain,
                &mSwapchainNumBufs, nullptr));

    mDisplayImages = new VkImage[mSwapchainNumBufs];
    CALL_VK(VulkanDeviceContext::GetThe()->GetSwapchainImagesKHR(
                VulkanDeviceContext::GetThe()->getDevice(), mSwapchain,
                &mSwapchainNumBufs, mDisplayImages));

    mPresentCompleteSemaphoresMem = new VkSemaphore[mSwapchainNumBufs + 1];
    mPresentCompleteSemaphores.resize(mSwapchainNumBufs, nullptr);

    for (uint32_t i = 0; i < (mSwapchainNumBufs + 1); i++) {
        VkSemaphoreCreateInfo semaphoreCreateInfo = VkSemaphoreCreateInfo();
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = nullptr;
        semaphoreCreateInfo.flags = 0;
        CALL_VK(VulkanDeviceContext::GetThe()->CreateSemaphore( VulkanDeviceContext::GetThe()->getDevice(), &semaphoreCreateInfo, nullptr,
                    &mPresentCompleteSemaphoresMem[i]));
    }

    for (uint32_t i = 0; i < mSwapchainNumBufs; i++) {
        mPresentCompleteSemaphores[i] = &mPresentCompleteSemaphoresMem[i];
    }

    mPresentCompleteSemaphoreInFly = &mPresentCompleteSemaphoresMem[mSwapchainNumBufs];
}

#ifdef VK_USE_PLATFORM_ANDROID_KHR
AHardwareBufferHandle ImageObject::ExportHandle()
{
    if (canBeExported) {
        AHardwareBufferHandle aHardwareBufferHandle;
        const VkMemoryGetAndroidHardwareBufferInfoANDROID getAndroidHardwareBufferInfo = {VK_STRUCTURE_TYPE_MEMORY_GET_ANDROID_HARDWARE_BUFFER_INFO_ANDROID, NULL, mem};
        CALL_VK(m_vkDevCtx->GetMemoryAndroidHardwareBufferANDROID(VulkanDeviceContext::GetThe()->getDevice(), &getAndroidHardwareBufferInfo, &aHardwareBufferHandle));
        return aHardwareBufferHandle;
    } else {
        return NULL;
    }
}
#endif // VK_USE_PLATFORM_ANDROID_KHR

int32_t ImageObject::GetImageSubresourceAndLayout(VkSubresourceLayout layouts[3]) const
{
    int numPlanes = 0;
    const VkMpFormatInfo *mpInfo =  YcbcrVkFormatInfo(imageFormat);
    VkImageSubresource subResource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    if (mpInfo) {
        switch (mpInfo->planesLayout.layout) {
        case YCBCR_SINGLE_PLANE_UNNORMALIZED:
        case YCBCR_SINGLE_PLANE_INTERLEAVED:
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[0]);
            numPlanes = 1;
            break;
        case YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED:
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[0]);
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[1]);
            numPlanes = 2;
            break;
        case YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED:
        case YCBCR_PLANAR_CBCR_BLOCK_JOINED:
        case YCBCR_PLANAR_STRIDE_PADDED:
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[0]);
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[1]);
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_2_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[2]);
            numPlanes = 3;
            break;
        default:
            assert(0);
        }
    } else {
        subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[0]);
        numPlanes = 1;
    }

    return numPlanes;
}

VkResult ImageObject::FillImageWithPattern(int pattern)
{
    const VkImageSubresource subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };

    VkSubresourceLayout layout;

    VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subres, &layout);

    uint8_t* mappedHostPtr = MapHostPtr();
    VkDeviceMemory devMemory = m_imageResource->GetDeviceMemory();

    const VkMpFormatInfo* mpInfo = YcbcrVkFormatInfo(imageFormat);
    if (mpInfo) {
        ImageData imageData =
            // 8/16-bit format and data. The format fields are updated based on the test format input.
            { imageFormat,          (uint32_t)imageWidth,  (uint32_t)imageHeight, (ColorPattern)pattern, {0xFF, 0x00, 0x00, 0xFF},  NULL };

        const VkSamplerYcbcrConversionCreateInfo ycbcrConversionInfo = { VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO, NULL,
                imageFormat, VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709, VK_SAMPLER_YCBCR_RANGE_ITU_FULL,
        {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
        VK_CHROMA_LOCATION_MIDPOINT, VK_CHROMA_LOCATION_MIDPOINT, VK_FILTER_LINEAR, false
                                                                              };
        VkFillYuv vkFillYuv;
        vkFillYuv.fillVkImage(image, &imageData, devMemory, mappedHostPtr, &ycbcrConversionInfo);
    } else {
        generateColorPatternRgba8888((ColorPattern)pattern, (uint8_t *)mappedHostPtr,
                                 imageWidth, imageHeight,
                                 (uint32_t)layout.rowPitch);
    }

    return VK_SUCCESS;
}

// Initialize the texture data, either directly into the texture itself
// or into buffer memory.
VkResult ImageObject::CopyYuvToVkImage(uint32_t numPlanes, const uint8_t* yuvPlaneData[3], const VkSubresourceLayout yuvPlaneLayouts[3])
{
    VkImageSubresource subResource = {};
    VkSubresourceLayout layouts[3];
    VkDeviceSize size   = 0;

    int cbimageHeight = imageHeight;

    // Clean it
    memset(layouts, 0x00, sizeof(layouts));

    const VkMpFormatInfo *mpInfo =  YcbcrVkFormatInfo(imageFormat);
    bool isUnnormalizedRgba = false;
    if (mpInfo && (mpInfo->planesLayout.layout == YCBCR_SINGLE_PLANE_UNNORMALIZED) && !(mpInfo->planesLayout.disjoint)) {
        isUnnormalizedRgba = true;
    }

    if (mpInfo && mpInfo->planesLayout.secondaryPlaneSubsampledY) {
        cbimageHeight /= 2;
    }

    if (mpInfo && !isUnnormalizedRgba) {
        VkMemoryRequirements memReqs = { };
        VulkanDeviceContext::GetThe()->GetImageMemoryRequirements(VulkanDeviceContext::GetThe()->getDevice(), image, &memReqs);
        size      = memReqs.size;
        // alignment = memReqs.alignment;
        switch (mpInfo->planesLayout.layout) {
        case YCBCR_SINGLE_PLANE_UNNORMALIZED:
        case YCBCR_SINGLE_PLANE_INTERLEAVED:
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[0]);
            break;
        case YCBCR_SEMI_PLANAR_CBCR_INTERLEAVED:
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[0]);
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[1]);
            break;
        case YCBCR_PLANAR_CBCR_STRIDE_INTERLEAVED:
        case YCBCR_PLANAR_CBCR_BLOCK_JOINED:
        case YCBCR_PLANAR_STRIDE_PADDED:
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[0]);
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[1]);
            subResource.aspectMask = VK_IMAGE_ASPECT_PLANE_2_BIT;
            VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[2]);
            break;
        default:
            assert(0);
        }

    } else {

        VulkanDeviceContext::GetThe()->GetImageSubresourceLayout(VulkanDeviceContext::GetThe()->getDevice(), image, &subResource, &layouts[0]);
        size = layouts[0].size;
    }

    uint8_t *ptr = MapHostPtr();

    for (uint32_t plane = 0; plane < numPlanes; plane++) {
        int copyHeight = plane ? cbimageHeight : imageHeight;
        uint8_t* pDst = ptr + layouts[plane].offset;
        const uint8_t* pSrc = yuvPlaneData[plane] + yuvPlaneLayouts[plane].offset;
        for (int height = 0; height < copyHeight; height++) {
            memcpy(pDst, pSrc, (size_t)layouts[plane].rowPitch);
            pDst += (size_t)layouts[plane].rowPitch;
            pSrc += (size_t)yuvPlaneLayouts[plane].rowPitch;
        }
    }

    m_imageResource->GetMemory()->FlushRange(0, size);

    return VK_SUCCESS;
}

VkResult VulkanFrameBuffer::CreateFrameBuffer(VkSwapchainKHR,
                                              const VkExtent2D *pExtent2D, const VkSurfaceFormatKHR *pSurfaceFormat, VkImage fbImage,
                                              VkRenderPass renderPass, VkImageView depthView)
{
    DestroyFrameBuffer();

    mFbImage = fbImage;

    VkImageViewCreateInfo viewCreateInfo = VkImageViewCreateInfo();
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.pNext = nullptr;
    viewCreateInfo.image = fbImage;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = pSurfaceFormat->format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    viewCreateInfo.flags = 0;
    VkResult result = VulkanDeviceContext::GetThe()->CreateImageView(VulkanDeviceContext::GetThe()->getDevice(), &viewCreateInfo, nullptr, &mImageView);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkImageView attachments[2] = { mImageView, depthView };
    VkFramebufferCreateInfo fbCreateInfo = VkFramebufferCreateInfo();
    fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCreateInfo.pNext = nullptr;
    fbCreateInfo.renderPass = renderPass;
    fbCreateInfo.layers = 1;
    fbCreateInfo.attachmentCount = 1;  // 2 if using depth
    fbCreateInfo.pAttachments = attachments;
    fbCreateInfo.width = pExtent2D->width;
    fbCreateInfo.height = pExtent2D->height;
    fbCreateInfo.attachmentCount = (depthView == VK_NULL_HANDLE ? 1 : 2);
    return VulkanDeviceContext::GetThe()->CreateFramebuffer(VulkanDeviceContext::GetThe()->getDevice(), &fbCreateInfo, nullptr, &mFramebuffer);
}

VkResult VulkanSyncPrimitives::CreateSyncPrimitives()
{
    DestroySyncPrimitives();

    // Create a fence to be able, in the main loop, to wait for the
    // draw command(s) to finish before swapping the framebuffers
    VkFenceCreateInfo fenceCreateInfo = VkFenceCreateInfo();
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
      // Create in signaled state so we don't wait on first render of each
      // command buffer
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkResult result = VulkanDeviceContext::GetThe()->CreateFence(VulkanDeviceContext::GetThe()->getDevice(), &fenceCreateInfo, nullptr, &mFence);
    if (result != VK_SUCCESS) {
        return result;
    }

    // We need to create a semaphore to be able to wait on, in the main loop, for
    // the framebuffer to be available before drawing.
    VkSemaphoreCreateInfo semaphoreCreateInfo = VkSemaphoreCreateInfo();
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    return VulkanDeviceContext::GetThe()->CreateSemaphore(VulkanDeviceContext::GetThe()->getDevice(), &semaphoreCreateInfo, nullptr, &mRenderCompleteSemaphore);
}

VkResult VulkanRenderPass::CreateRenderPass(VkFormat displayImageFormat)
{
    DestroyRenderPass();

    // -----------------------------------------------------------------
    // Create render pass
    VkAttachmentDescription attachmentDescriptions = VkAttachmentDescription();
    attachmentDescriptions.format = displayImageFormat;
    attachmentDescriptions.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colourReference = VkAttachmentReference();
    colourReference.attachment = 0;
    colourReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = VkSubpassDescription();
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.flags = 0;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colourReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    VkSubpassDependency dependencies[2];
    // First dependency at the start of the renderpass
    // Does the transition from final to initial layout
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                               // Producer of the dependency
    dependencies[0].dstSubpass = 0;                                                 // Consumer is our single subpass that will wait for the execution dependency
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Second dependency at the end the renderpass
    // Does the transition from the initial to the final layout
    dependencies[1].srcSubpass = 0;                                                 // Producer of the dependency is our single subpass
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;                               // Consumer are all commands outside of the renderpass
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = VkRenderPassCreateInfo();
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachmentDescriptions;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = sizeof(dependencies)/sizeof(dependencies[0]);
    renderPassCreateInfo.pDependencies = dependencies;

    return VulkanDeviceContext::GetThe()->CreateRenderPass(VulkanDeviceContext::GetThe()->getDevice(), &renderPassCreateInfo, nullptr, &renderPass);
}

// Create Graphics Pipeline
VkResult VulkanGraphicsPipeline::CreatePipeline(VkViewport *pViewport, VkRect2D *pScissor,
                                                VkRenderPass renderPass, VulkanDescriptorSetLayout *pBufferDescriptorSets)
{
    if (m_cache == VkPipelineCache(0)) {
        // Create the pipeline cache
        VkPipelineCacheCreateInfo pipelineCacheInfo = VkPipelineCacheCreateInfo();
        pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheInfo.pNext = nullptr;
        pipelineCacheInfo.initialDataSize = 0;
        pipelineCacheInfo.pInitialData = nullptr;
        pipelineCacheInfo.flags = 0;  // reserved, must be 0
        CALL_VK(VulkanDeviceContext::GetThe()->CreatePipelineCache(VulkanDeviceContext::GetThe()->getDevice(), &pipelineCacheInfo, nullptr, &m_cache));
    }

    // No dynamic state in that tutorial
    VkPipelineDynamicStateCreateInfo dynamicStateInfo = VkPipelineDynamicStateCreateInfo();
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.pNext = nullptr;
    dynamicStateInfo.dynamicStateCount = 0;
    dynamicStateInfo.pDynamicStates = nullptr;

    // See https://vkguide.dev/docs/chapter-3/push_constants/
    static char const vss[] =
        "#version 450 core\n"
        "layout(location = 0) in vec2 aVertex;\n"
        "layout(location = 1) in vec2 aTexCoord;\n"
        "layout(location = 0) out vec2 vTexCoord;\n"
        "\n"
        "layout( push_constant ) uniform constants\n"
        "{\n"
        "    mat4 posMatrix;\n"
        "    mat2 texMatrix;\n"
        "} transformPushConstants;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vTexCoord = transformPushConstants.texMatrix * aTexCoord;\n"
        "    gl_Position = vec4(aVertex, 0, 1);\n"
        "}\n"
        ;

    std::stringstream imageFss;
    const uint32_t setIds[] = {0};
    const uint32_t setIndex = 0;
    const uint32_t bindingIndex = 0;
    const uint32_t arrayIndex = 0;
    pBufferDescriptorSets->CreateFragmentShaderLayouts(setIds, sizeof(setIds)/sizeof(setIds[0]), imageFss);
    pBufferDescriptorSets->CreateFragmentShaderOutput(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, setIds[setIndex], bindingIndex, arrayIndex, imageFss);

    const bool verbose = false;

    if (false) printf("\nVertex shader output code:\n %s", vss);
    if (false) printf("\nFragment shader output code:\n %s", imageFss.str().c_str());

    const bool loadShadersFromFile = false;
    if (loadShadersFromFile)
	{
		DestroyVertexShaderModule();
		m_vertexShaderCache = m_vulkanShaderCompiler.BuildShaderFromFile("/sdcard/vulkan_video_demo/shaders/tri.vert",
		                                                                 VK_SHADER_STAGE_VERTEX_BIT);

		DestroyFragmentShaderModule();
		m_fragmentShaderCache = m_vulkanShaderCompiler.BuildShaderFromFile("/sdcard/vulkan_video_demo/shaders/tri.frag",
		                                                                   VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	if (m_vertexShaderCache == VkShaderModule(0))
	{
		m_vertexShaderCache = m_vulkanShaderCompiler.BuildGlslShader(vss, strlen(vss),
		                                                             VK_SHADER_STAGE_VERTEX_BIT);
	}

	if (m_fssCache.str() != imageFss.str())
	{
		DestroyFragmentShaderModule();
		m_fragmentShaderCache = m_vulkanShaderCompiler.BuildGlslShader(imageFss.str().c_str(), strlen(imageFss.str().c_str()),
		                                                               VK_SHADER_STAGE_FRAGMENT_BIT);

		m_fssCache.swap(imageFss);
		if (verbose)
			printf("\nFragment shader cache output code:\n %s", m_fssCache.str().c_str());
	}

	// Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[1].sType = shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    shaderStages[1].pNext = shaderStages[0].pNext = nullptr;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[1].pName = shaderStages[0].pName = "main";
    shaderStages[0].module = m_vertexShaderCache;
    shaderStages[0].pSpecializationInfo = nullptr;
    shaderStages[0].flags = 0;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = m_fragmentShaderCache;
    shaderStages[1].pSpecializationInfo = nullptr;
    shaderStages[1].flags = 0;

    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewportInfo = VkPipelineViewportStateCreateInfo();
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.pNext = nullptr;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = pViewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = pScissor;

    // Specify multisample info
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo = VkPipelineMultisampleStateCreateInfo();
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.pNext = nullptr;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.minSampleShading = 0;
    multisampleInfo.pSampleMask = &sampleMask;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.alphaToOneEnable = VK_FALSE;

    // Specify color blend state
    VkPipelineColorBlendAttachmentState attachmentStates = VkPipelineColorBlendAttachmentState();
    attachmentStates.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachmentStates.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendInfo = VkPipelineColorBlendStateCreateInfo();
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.pNext = nullptr;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &attachmentStates;
    colorBlendInfo.flags = 0;

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo rasterInfo = VkPipelineRasterizationStateCreateInfo();
    rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.pNext = nullptr;
    rasterInfo.depthClampEnable = VK_FALSE;
    rasterInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_NONE;
    rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterInfo.depthBiasEnable = VK_FALSE;
    rasterInfo.lineWidth = 1;

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = VkPipelineInputAssemblyStateCreateInfo();
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.pNext = nullptr;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings = VkVertexInputBindingDescription();
    vertex_input_bindings.binding = 0;
    vertex_input_bindings.stride = sizeof(vk::Vertex);
    vertex_input_bindings.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_input_attributes[2];
    vertex_input_attributes[0].location = 0;
    vertex_input_attributes[0].binding = 0;
    vertex_input_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attributes[0].offset = offsetof(vk::Vertex, position);
    vertex_input_attributes[1].location = 1;
    vertex_input_attributes[1].binding = 0;
    vertex_input_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attributes[1].offset = offsetof(vk::Vertex, texCoord);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = VkPipelineVertexInputStateCreateInfo();
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertex_input_bindings;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertex_input_attributes;

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = VkGraphicsPipelineCreateInfo();
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineCreateInfo.pTessellationState = nullptr;
    pipelineCreateInfo.pViewportState = &viewportInfo;
    pipelineCreateInfo.pRasterizationState = &rasterInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleInfo;
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
    pipelineCreateInfo.layout = pBufferDescriptorSets->GetPipelineLayout();
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = 0;

    // Make sure we destroy the existing pipeline, if it were to exist.
    DestroyPipeline();
    VkResult pipelineResult = VulkanDeviceContext::GetThe()->CreateGraphicsPipelines(VulkanDeviceContext::GetThe()->getDevice(), m_cache, 1,
                                                                  &pipelineCreateInfo,
                                                                  nullptr, &m_pipeline);

    return pipelineResult;
}

VkResult VulkanPerDrawContext::RecordCommandBuffer(VkCommandBuffer cmdBuffer,
                                                   VkRenderPass renderPass,
                                                   const ImageResourceInfo* inputImageToDrawFrom,
                                                   int32_t displayWidth, int32_t displayHeight,
                                                   VkImage displayImage, VkFramebuffer framebuffer, VkRect2D* pRenderArea,
                                                   VkPipeline pipeline,
                                                   const VulkanDescriptorSetLayout& descriptorSetLayoutBinding,
                                                   const VulkanSamplerYcbcrConversion& samplerYcbcrConversion,
                                                   const VulkanVertexBuffer& vertexBuffer)
{

    // We start by creating and declare the "beginning" our command buffer
    VkCommandBufferBeginInfo cmdBufferBeginInfo = VkCommandBufferBeginInfo();
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = nullptr;
    cmdBufferBeginInfo.flags = 0;
    cmdBufferBeginInfo.pInheritanceInfo = nullptr;
    VkResult result = VulkanDeviceContext::GetThe()->BeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);
    if (result != VK_SUCCESS) {
        return result;
    }
    // transition the buffer into color attachment
    setImageLayout( cmdBuffer, displayImage,
                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    const VkMpFormatInfo * pFormatInfo = YcbcrVkFormatInfo(inputImageToDrawFrom->imageFormat);
    if (pFormatInfo == NULL) {
        // Non-planar input image.
        setImageLayout(cmdBuffer, inputImageToDrawFrom->image,
                       VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_IMAGE_ASPECT_COLOR_BIT);
    } else {
        // Multi-planar input image.
        for (uint32_t planeIndx = 0; (planeIndx < (uint32_t)pFormatInfo->planesLayout.numberOfExtraPlanes + 1); planeIndx++) {
            setImageLayout(cmdBuffer, inputImageToDrawFrom->image,
                       VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       (VK_IMAGE_ASPECT_PLANE_0_BIT_KHR << planeIndx));

        }
    }
    // Now we start a renderpass. Any draw command has to be recorded in a renderpass
    VkClearValue clearVals = VkClearValue();
    clearVals.color.float32[0] = 0.0f;
    clearVals.color.float32[1] = 0.34f;
    clearVals.color.float32[2] = 0.90f;
    clearVals.color.float32[3] = 1.0f;

    VkRenderPassBeginInfo renderPassBeginInfo = VkRenderPassBeginInfo();
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea = *pRenderArea;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearVals;

    VulkanDeviceContext::GetThe()->CmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    // Bind what is necessary to the command buffer
    VulkanDeviceContext::GetThe()->CmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDescriptorSetLayoutCreateFlags layoutMode = descriptorSetLayoutBinding.GetDescriptorSetLayoutInfo().GetDescriptorLayoutMode();
    switch (layoutMode) {
        case VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR:
        case VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT:
        {
            const VkDescriptorImageInfo combinedImageSampler { samplerYcbcrConversion.GetSampler(),
                                                               inputImageToDrawFrom->view,
                                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

            const uint32_t numDescriptors = 1;
            std::array<VkWriteDescriptorSet, numDescriptors> writeDescriptorSets{};

            uint32_t set = 0;

            // Image
            writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[0].dstSet = VK_NULL_HANDLE;
            writeDescriptorSets[0].dstBinding = 0;
            writeDescriptorSets[0].descriptorCount = 1;
            writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[0].pImageInfo = &combinedImageSampler;

            if (layoutMode == VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR) {
                VulkanDeviceContext::GetThe()->CmdPushDescriptorSetKHR(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                    descriptorSetLayoutBinding.GetPipelineLayout(),
                                                    set, numDescriptors,
                                                    writeDescriptorSets.data());
            } else {

                VkDeviceOrHostAddressConstKHR imageDescriptorBufferDeviceAddress =
                        descriptorSetLayoutBinding.UpdateDescriptorBuffer(0, // always index 0
                                                                          set, numDescriptors,
                                                                          writeDescriptorSets.data());

                // Descriptor buffer bindings
                // Set 0 = Image
                VkDescriptorBufferBindingInfoEXT bindingInfo{};
                bindingInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
                bindingInfo.pNext = nullptr;
                bindingInfo.address = imageDescriptorBufferDeviceAddress.deviceAddress;
                bindingInfo.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
                                    VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
                VulkanDeviceContext::GetThe()->CmdBindDescriptorBuffersEXT(cmdBuffer, 1, &bindingInfo);

                // Image (set 0)
                uint32_t bufferIndexImage = 0;
                VkDeviceSize bufferOffset = 0;
                VulkanDeviceContext::GetThe()->CmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                             descriptorSetLayoutBinding.GetPipelineLayout(),
                                                             set, 1, &bufferIndexImage, &bufferOffset);
            }
        }
        break;
        default:
            VulkanDeviceContext::GetThe()->CmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                              descriptorSetLayoutBinding.GetPipelineLayout(),
                                              0, 1, descriptorSetLayoutBinding.GetDescriptorSet(),
                                              0, nullptr);
    }

    VkDeviceSize offset = 0;
    VkBuffer vertexBuff = vertexBuffer.GetBuffer();
    VulkanDeviceContext::GetThe()->CmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuff, &offset);

    bool scaleInput = true;
    vk::TransformPushConstants constants;
    if (scaleInput) {
        if (displayWidth && (displayWidth != inputImageToDrawFrom->imageWidth)) {
            constants.texMatrix[0] = vk::Vec2((float)displayWidth / inputImageToDrawFrom->imageWidth, 0.0f);
        }

        if (displayHeight && (displayHeight != inputImageToDrawFrom->imageHeight)) {
            constants.texMatrix[1] = vk::Vec2(.0f, (float)displayHeight /inputImageToDrawFrom->imageHeight);
        }
    }

    //upload the matrix to the GPU via push constants
    VulkanDeviceContext::GetThe()->CmdPushConstants(cmdBuffer, descriptorSetLayoutBinding.GetPipelineLayout(),
                                 VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vk::TransformPushConstants), &constants);

    // Draw the quad
    VulkanDeviceContext::GetThe()->CmdDraw(cmdBuffer, vertexBuffer.GetNumVertices(), 1, 0, 0);

    VulkanDeviceContext::GetThe()->CmdEndRenderPass(cmdBuffer);

    setImageLayout(cmdBuffer,
                   displayImage,
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);


    if (pFormatInfo == NULL) {
        // Non-planar input image.
        setImageLayout(cmdBuffer, inputImageToDrawFrom->image,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR,
                       VK_IMAGE_ASPECT_COLOR_BIT);
    } else {
        // Multi-planar input image.
        for (uint32_t planeIndx = 0; (planeIndx < (uint32_t)pFormatInfo->planesLayout.numberOfExtraPlanes + 1); planeIndx++) {
            setImageLayout(cmdBuffer, inputImageToDrawFrom->image,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR,
                       (VK_IMAGE_ASPECT_PLANE_0_BIT_KHR << planeIndx));

        }
    }

    return VulkanDeviceContext::GetThe()->EndCommandBuffer(cmdBuffer);
}

VkResult VulkanRenderInfo::UpdatePerDrawContexts(VulkanPerDrawContext* pPerDrawContext,
        VkViewport* pViewport, VkRect2D* pScissor, VkRenderPass renderPass,
        const VkSamplerCreateInfo* pSamplerCreateInfo,
        const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo)
{

    if (mVerbose) std::cout << "VkVideoUtils: " << "CreateVulkanSamplers " << pPerDrawContext->contextIndex << std::endl;
    VkResult result = pPerDrawContext->samplerYcbcrConversion.CreateVulkanSampler(pSamplerCreateInfo,
            pSamplerYcbcrConversionCreateInfo);
    if (result != VK_SUCCESS) {
        return result;
    }

    if (mVerbose) std::cout << "VkVideoUtils: " << "CreateDescriptorSet " << pPerDrawContext->contextIndex << std::endl;

    VkSampler immutableSampler = pPerDrawContext->samplerYcbcrConversion.GetSampler();
    const std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{
        //                        binding,  descriptorType,          descriptorCount, stageFlags, pImmutableSamplers;
        // Binding 0: Input image (read-only) RGBA or RGBA YCbCr sampler sampled
        VkDescriptorSetLayoutBinding{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &immutableSampler},
    };

    // setup push constants
    VkPushConstantRange push_constant;
    //this push constant range starts at the beginning
    push_constant.offset = 0;
    //this push constant range takes up the size of a MeshPushConstants struct
    push_constant.size = sizeof(vk::TransformPushConstants);
    //this push constant range is accessible only in the vertex shader
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    result = pPerDrawContext->descriptorSetLayoutBinding.CreateDescriptorSet(setLayoutBindings,
                                                                             VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
                                                                             1, &push_constant,
                                                                             &pPerDrawContext->samplerYcbcrConversion);
    if (result != VK_SUCCESS) {
        return result;
    }
    if (mVerbose) std::cout << "VkVideoUtils: " << "CreateGraphicsPipeline " << pPerDrawContext->contextIndex << std::endl;
    // Create graphics pipeline
    result = pPerDrawContext->gfxPipeline.CreatePipeline(pViewport,
	                                                     pScissor,
	                                                     renderPass,
	                                                     &pPerDrawContext->descriptorSetLayoutBinding);

    return result;
}


// Create per draw contexts.
VkResult VulkanRenderInfo::CreatePerDrawContexts(
        VkSwapchainKHR swapchain, const VkExtent2D* pFbExtent2D, VkViewport* pViewport,
        VkRect2D* pScissor, const VkSurfaceFormatKHR* pSurfaceFormat,
        VkRenderPass renderPass, const VkSamplerCreateInfo* pSamplerCreateInfo,
        const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo)
{
    std::vector<VkImage> fbImages;
    vk::get(VulkanDeviceContext::GetThe(), VulkanDeviceContext::GetThe()->getDevice(), swapchain, fbImages);
    int32_t numFbImages = (int32_t )fbImages.size();

    perDrawCtx.resize(numFbImages);


    VkResult result = VK_SUCCESS;
    for (int32_t ctxsIndx = 0; ctxsIndx < numFbImages; ctxsIndx++) {

        VulkanPerDrawContext* pPerDrawContext = GetDrawContext(ctxsIndx);
        pPerDrawContext->contextIndex = ctxsIndx;
        if (mVerbose) std::cout << "VkVideoUtils: " << "Init pPerDrawContext " << ctxsIndx << std::endl;

        if (mVerbose) std::cout << "VkVideoUtils: " << "CreateCommandBufferPool " << pPerDrawContext->contextIndex << std::endl;
        result = pPerDrawContext->commandBuffer.CreateCommandBufferPool( VulkanDeviceContext::GetThe()->GetGfxQueueFamilyIdx());
        if (result != VK_SUCCESS) {
            return result;
        }

        if (mVerbose) std::cout << "VkVideoUtils: " << "CreateFrameBuffer " << pPerDrawContext->contextIndex << std::endl;
        result = pPerDrawContext->frameBuffer.CreateFrameBuffer(swapchain, pFbExtent2D,
		                                                        pSurfaceFormat, fbImages[ctxsIndx],
		                                                        renderPass);
        if (result != VK_SUCCESS) {
            return result;
        }

        if (mVerbose) std::cout << "VkVideoUtils: " << "CreateSyncPrimitives " << pPerDrawContext->contextIndex << std::endl;
        result = pPerDrawContext->syncPrimitives.CreateSyncPrimitives();
        if (result != VK_SUCCESS) {
            return result;
        }

        result = UpdatePerDrawContexts(pPerDrawContext, pViewport, pScissor, renderPass,
                                       pSamplerCreateInfo, pSamplerYcbcrConversionCreateInfo);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    return result;
}

void setImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                    VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                    VkPipelineStageFlags srcStages,
                    VkPipelineStageFlags destStages, VkImageAspectFlags aspectMask)
{
    VkImageMemoryBarrier2KHR imageMemoryBarrier = VkImageMemoryBarrier2KHR();
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcStageMask  = srcStages;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstStageMask  = destStages;
    imageMemoryBarrier.dstAccessMask = 0;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    switch ((uint32_t)oldImageLayout) {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
        break;
    default:
        break;
    }

    switch ((uint32_t)newImageLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
        break;

    case VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR;
        break;

    case VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR;
        break;

    case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR | VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR;
        break;

    case VK_IMAGE_LAYOUT_GENERAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    default:
        break;
    }

    const VkDependencyInfoKHR dependencyInfo = {
            VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
            nullptr,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier,
    };

    VulkanDeviceContext::GetThe()->CmdPipelineBarrier2KHR(cmdBuffer, &dependencyInfo);
}


VkResult VkVideoAppCtx::CreateSamplerYcbcrConversions()
{

    return VK_SUCCESS;
}

} // namespace vulkanVideoUtils
