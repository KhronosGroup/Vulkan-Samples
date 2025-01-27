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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(__unix__)
#include <unistd.h>
#endif

#include <vector>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream

#ifndef __VULKANVIDEOUTILS__
#define __VULKANVIDEOUTILS__

#include <vulkan_interfaces.h>
#include "VkCodecUtils/VkBufferResource.h"
#include "VkCodecUtils/VkImageResource.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkCodecUtils/VulkanShaderCompiler.h"
#include "VkCodecUtils/VulkanDescriptorSetLayout.h"
#include "VkCodecUtils/VulkanCommandBuffersSet.h"
#include "VkCodecUtils/Helpers.h"

namespace vulkanVideoUtils {

struct ImageResourceInfo {
    VkFormat       imageFormat;
    int32_t        imageWidth;
    int32_t        imageHeight;
    uint32_t       arrayLayer;
    VkImageLayout  imageLayout;
    VkImage        image;
    VkImageView    view;

    ImageResourceInfo() : imageFormat(VK_FORMAT_UNDEFINED),
                          imageWidth(0),
                          imageHeight(0),
                          arrayLayer(0),
                          imageLayout(VK_IMAGE_LAYOUT_UNDEFINED),
                          image(),
                          view()
    {}

    ImageResourceInfo(VkImageResourceView* pView, VkImageLayout layout) {
        if (pView) {
            VkImageResource* pImage = pView->GetImageResource();
            imageFormat = pImage->GetImageCreateInfo().format;
            imageWidth = pImage->GetImageCreateInfo().extent.width;
            imageHeight = pImage->GetImageCreateInfo().extent.height;
            arrayLayer = pView->GetImageSubresourceRange().baseArrayLayer;
            imageLayout = layout;
            image = pImage->GetImage();
            view = pView->GetImageView();
        } else {
            imageFormat = VK_FORMAT_UNDEFINED;
            imageWidth = 0;
            imageHeight = 0;
            arrayLayer = 0;
            imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image = VK_NULL_HANDLE;
            view = VK_NULL_HANDLE;
        }
    }
};

class VulkanDisplayTiming  {
public:
    VulkanDisplayTiming()
    : vkGetRefreshCycleDurationGOOGLE(nullptr),
      vkGetPastPresentationTimingGOOGLE(nullptr)
    {
#ifdef VK_GOOGLE_display_timing
    vkGetRefreshCycleDurationGOOGLE =
            reinterpret_cast<PFN_vkGetRefreshCycleDurationGOOGLE>(
                    VulkanDeviceContext::GetThe()->GetDeviceProcAddr(VulkanDeviceContext::GetThe()->getDevice(), "vkGetRefreshCycleDurationGOOGLE"));
    vkGetPastPresentationTimingGOOGLE =
            reinterpret_cast<PFN_vkGetPastPresentationTimingGOOGLE>(
                    VulkanDeviceContext::GetThe()->GetDeviceProcAddr(VulkanDeviceContext::GetThe()->getDevice(), "vkGetPastPresentationTimingGOOGLE"));

#endif // VK_GOOGLE_display_timing
    }

    VkResult GetRefreshCycle(VkDevice device, VkSwapchainKHR swapchain, uint64_t* pRefreshDuration) {

        if (!vkGetRefreshCycleDurationGOOGLE) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        VkRefreshCycleDurationGOOGLE  displayTimingProperties = VkRefreshCycleDurationGOOGLE();
        VkResult result = vkGetRefreshCycleDurationGOOGLE(device, swapchain, &displayTimingProperties);
        if (VK_SUCCESS == result) {
            *pRefreshDuration = displayTimingProperties.refreshDuration;
        }
        return result;
    }

    bool DisplayTimingIsEnabled() {
        return (vkGetRefreshCycleDurationGOOGLE && vkGetPastPresentationTimingGOOGLE);
    }

    operator bool() {
        return DisplayTimingIsEnabled();
    }

private:
    PFN_vkGetRefreshCycleDurationGOOGLE   vkGetRefreshCycleDurationGOOGLE;
    PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE;

};

class VulkanSwapchainInfo {

public:
    VulkanSwapchainInfo()
      : mInstance(),
        mSurface(),
        mSwapchain(),
        mSwapchainNumBufs(),
        mDisplaySize(),
        mDisplayFormat(),
        mDisplayImages(nullptr),
        mPresentCompleteSemaphoresMem(nullptr),
        mPresentCompleteSemaphoreInFly(nullptr),
        mVerbose(false)
    { }

    void CreateSwapChain(VkSwapchainKHR swapchain);

    ~VulkanSwapchainInfo()
    {
        delete[] mDisplayImages;
        mDisplayImages = nullptr;

        if (mSwapchain) {
            VulkanDeviceContext::GetThe()->DestroySwapchainKHR(VulkanDeviceContext::GetThe()->getDevice(),
                              mSwapchain, nullptr);
        }

        if (mSurface) {
            VulkanDeviceContext::GetThe()->DestroySurfaceKHR(mInstance, mSurface, nullptr);
            mSurface = VkSurfaceKHR();
        }

        if (mPresentCompleteSemaphoresMem) {
            mPresentCompleteSemaphores.clear();
            mPresentCompleteSemaphoreInFly = nullptr;

            for (uint32_t i = 0; i < mSwapchainNumBufs + 1; i++) {
                VulkanDeviceContext::GetThe()->DestroySemaphore(VulkanDeviceContext::GetThe()->getDevice(), mPresentCompleteSemaphoresMem[i], nullptr);
            }

            delete[] mPresentCompleteSemaphoresMem;
            mPresentCompleteSemaphoresMem = nullptr;
        }

        mInstance = VkInstance();
        mSwapchain = VkSwapchainKHR();
        mSwapchainNumBufs = 0;
        mSurface = VkSurfaceKHR();
        mDisplaySize = VkExtent2D();
        mDisplayFormat = VkFormat();
    }

    VkImage GetImage(uint32_t fbImageIndex) const {
        if (fbImageIndex < mSwapchainNumBufs) {
            return mDisplayImages[fbImageIndex];
        };

        return VkImage(0);
    }

    VkFormat GetImageFormat() const {
        return mDisplayFormat;
    }

    const VkExtent2D GetExtent2D() const {
        return mDisplaySize;
    }

    VkSemaphore* GetPresentSemaphoreInFly() const {
        assert(mPresentCompleteSemaphoreInFly);

        return mPresentCompleteSemaphoreInFly;
    }

    void SetPresentSemaphoreInFly(uint32_t scIndex, const VkSemaphore * semaphore)
    {
        assert(mPresentCompleteSemaphoreInFly == semaphore);
        assert(scIndex < mSwapchainNumBufs);

        // Swap the semaphore on the fly with the one that is requested to be set.
        VkSemaphore* tempSem = mPresentCompleteSemaphores[scIndex];
        mPresentCompleteSemaphores[scIndex] = mPresentCompleteSemaphoreInFly;
        mPresentCompleteSemaphoreInFly = tempSem;
    }

    VkSemaphore* GetPresentSemaphore(uint32_t scIndex)
    {
        VkSemaphore* tempSem = mPresentCompleteSemaphores[scIndex];
        assert(tempSem);
        return tempSem;
    }

    VkResult GetDisplayRefreshCycle(uint64_t* pRefreshDuration) {
        return mDisplayTiming.GetRefreshCycle(VulkanDeviceContext::GetThe()->getDevice(), mSwapchain, pRefreshDuration);
    }

    VkInstance mInstance;
    
    VkSurfaceKHR mSurface;
    VkSwapchainKHR mSwapchain;
    uint32_t mSwapchainNumBufs;

    VkExtent2D mDisplaySize;
    VkFormat mDisplayFormat;

    // array of frame buffers and views
    VkImage *mDisplayImages;

    VkSemaphore* mPresentCompleteSemaphoresMem;
    VkSemaphore* mPresentCompleteSemaphoreInFly;
    std::vector <VkSemaphore*> mPresentCompleteSemaphores;

    VulkanDisplayTiming mDisplayTiming;
    uint32_t            mVerbose : 1;
};

class ImageObject : public ImageResourceInfo {
public:
    ImageObject ()
    :   ImageResourceInfo()
    { }

    VkResult CreateImage(
            const VkImageCreateInfo* pImageCreateInfo,
            VkMemoryPropertyFlags requiredMemProps = 0,
            int initWithPattern = -1) {

        DestroyImage();

        VkResult result = VkImageResource::Create(pImageCreateInfo,
                                                  requiredMemProps,
                                                  m_imageResource);
        if (result != VK_SUCCESS) {
            return result;
        }

        VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        result = VkImageResourceView::Create( m_imageResource,
                                             subresourceRange,
                                             m_imageView);

        if (result != VK_SUCCESS) {
            return result;
        }

        image       = m_imageResource->GetImage();
        view        = m_imageView->GetImageView();
        imageFormat = pImageCreateInfo->format;
        imageWidth =  pImageCreateInfo->extent.width;
        imageHeight = pImageCreateInfo->extent.height;
        imageLayout = pImageCreateInfo->initialLayout;

        if (initWithPattern && (requiredMemProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            FillImageWithPattern(initWithPattern);
        }

        return result;
    }

    VkResult CreateImage(
            VkSharedBaseObj<VkImageResourceView>& imageView,
            int initWithPattern = -1) {

        DestroyImage();

        m_imageView = imageView;
        m_imageResource  = imageView->GetImageResource();
        const VkImageCreateInfo& imageCreateInfo = m_imageResource->GetImageCreateInfo();

        image       = m_imageResource->GetImage();
        view        = m_imageView->GetImageView();
        imageFormat = imageCreateInfo.format;
        imageWidth =  imageCreateInfo.extent.width;
        imageHeight = imageCreateInfo.extent.height;
        imageLayout = imageCreateInfo.initialLayout;

        VkSharedBaseObj<VulkanDeviceMemoryImpl> deviceMemory = m_imageResource->GetMemory();

        if (initWithPattern && (deviceMemory->GetMemoryPropertyFlags() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            FillImageWithPattern(initWithPattern);
        }

        return VK_SUCCESS;
    }

    VkResult AllocMemoryAndBind(const VulkanDeviceContext* vkDevCtx, VkImage vkImage, VkDeviceMemory& imageDeviceMemory, VkMemoryPropertyFlags requiredMemProps,
            bool dedicated, VkExternalMemoryHandleTypeFlags exportMemHandleTypes, vk::NativeHandle& importHandle = vk::NativeHandle::InvalidNativeHandle);

    VkResult FillImageWithPattern(int pattern);
    VkResult CopyYuvToVkImage(uint32_t numPlanes, const uint8_t* yuvPlaneData[3], const VkSubresourceLayout yuvPlaneLayouts[3]);
    VkResult StageImage(const VulkanDeviceContext* vkDevCtx, VkImageUsageFlags usage, VkMemoryPropertyFlags requiredMemProps, bool needBlit);

    VkResult GetMemoryFd(int* pFd) const;
    int32_t GetImageSubresourceAndLayout(VkSubresourceLayout layouts[3]) const;

    ImageObject& operator= (const ImageObject&) = delete;
    ImageObject& operator= (ImageObject&&) = delete;

    uint8_t* MapHostPtr() {
        VkDeviceSize maxSize = VK_WHOLE_SIZE;
        return m_imageResource->GetMemory()->GetDataPtr(0, maxSize);
    }

    operator bool() {
        return (m_imageResource && (m_imageResource->GetImage() != VK_NULL_HANDLE));
    }

    ~ImageObject()
    {
        DestroyImage();
    }

    void DestroyImage()
    {
        m_imageView     = nullptr;
        m_imageResource = nullptr;
    }

    VkSharedBaseObj<VkImageResource>     m_imageResource;
    VkSharedBaseObj<VkImageResourceView> m_imageView;
};

class VulkanRenderPass {

public:
    VulkanRenderPass()
        : renderPass()
    {}

    VkResult CreateRenderPass(VkFormat displayImageFormat);

    void DestroyRenderPass() {
        if (renderPass) {
            VulkanDeviceContext::GetThe()->DestroyRenderPass(VulkanDeviceContext::GetThe()->getDevice(),
                            renderPass, nullptr);

            renderPass = VkRenderPass(0);
        }
    }

    ~VulkanRenderPass() {
        DestroyRenderPass();
    }

    VkRenderPass getRenderPass() {
        return renderPass;
    }

private:
    
    VkRenderPass renderPass;
};

class VulkanVertexBuffer {

public:
    VulkanVertexBuffer()
        : m_vertexBuffer(nullptr) { }

    VkBuffer GetBuffer() const {
        return m_vertexBuffer->GetBuffer();
    }

    VkResult CreateVertexBuffer(const float* pVertexData,
                                VkDeviceSize vertexDataSize, uint32_t)
    {
        DestroyVertexBuffer();

        uint32_t queueFamilyIndex = VulkanDeviceContext::GetThe()->GetGfxQueueFamilyIdx();
        return VkBufferResource::Create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                        vertexDataSize,
                                        m_vertexBuffer, 1, 1,
                                        vertexDataSize, pVertexData,
                                        1, &queueFamilyIndex);
    }


    void DestroyVertexBuffer()
    {
        m_vertexBuffer   = nullptr;
    }

    ~VulkanVertexBuffer()
    {
        DestroyVertexBuffer();
    }

    uint32_t GetNumVertices() const {
        return 4;
    }

private:
    VkSharedBaseObj<VkBufferResource>  m_vertexBuffer;
};

class VulkanFrameBuffer {

public:
    VulkanFrameBuffer()
       : mFbImage(),
         mImageView(),
         mFramebuffer() {}

    ~VulkanFrameBuffer()
    {
        DestroyFrameBuffer();
    }

    void DestroyFrameBuffer()
    {
        if (mFramebuffer) {
            VulkanDeviceContext::GetThe()->DestroyFramebuffer(VulkanDeviceContext::GetThe()->getDevice(), mFramebuffer, nullptr);
            mFramebuffer = VkFramebuffer(0);
        }

        if (mImageView) {
            VulkanDeviceContext::GetThe()->DestroyImageView(VulkanDeviceContext::GetThe()->getDevice(), mImageView, nullptr);
            mImageView = VkImageView(0);
        }

        mFbImage = VkImage();
    }

    VkFramebuffer GetFrameBuffer()
    {
        return mFramebuffer;
    }

    VkImage GetFbImage()
    {
        return mFbImage;
    }

    VkResult CreateFrameBuffer(VkSwapchainKHR    swapchain,
	                           const VkExtent2D *pExtent2D, const VkSurfaceFormatKHR *pSurfaceFormat, VkImage fbImage,
	                           VkRenderPass renderPass, VkImageView depthView = VK_NULL_HANDLE);

    
    VkImage       mFbImage;
    VkImageView   mImageView;
    VkFramebuffer mFramebuffer;
};

class VulkanSyncPrimitives {

public:
    VulkanSyncPrimitives()
       : mRenderCompleteSemaphore(),
         mFence() {}

    ~VulkanSyncPrimitives()
    {
        DestroySyncPrimitives();
    }

    void DestroySyncPrimitives()
    {
        if (mFence) {
            VulkanDeviceContext::GetThe()->DestroyFence(VulkanDeviceContext::GetThe()->getDevice(), mFence, nullptr);
            mFence = VkFence();
        }

        if (mRenderCompleteSemaphore) {
            VulkanDeviceContext::GetThe()->DestroySemaphore(VulkanDeviceContext::GetThe()->getDevice(), mRenderCompleteSemaphore, nullptr);
            mRenderCompleteSemaphore = VkSemaphore();
        }
    }

    VkResult CreateSyncPrimitives();

    
    VkSemaphore   mRenderCompleteSemaphore;
    VkFence       mFence;
};


class VulkanGraphicsPipeline {

public:
    VulkanGraphicsPipeline()
     : m_cache(),
       m_pipeline(),
       m_vulkanShaderCompiler(),
       m_fssCache(),
       m_vertexShaderCache(0),
       m_fragmentShaderCache(0)
    { }


    VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other)
        : m_cache(other.m_cache),
          m_pipeline(other.m_pipeline),
          m_vulkanShaderCompiler(std::move(other.m_vulkanShaderCompiler)),
          m_fssCache(std::move(other.m_fssCache)),
          m_vertexShaderCache(other.m_vertexShaderCache),
          m_fragmentShaderCache(other.m_fragmentShaderCache)
    {
        // Reset the other object's resources
        other.m_cache = VkPipelineCache(0);
        other.m_pipeline = VkPipeline(0);
        other.m_vertexShaderCache = VkShaderModule();
        other.m_fragmentShaderCache = VkShaderModule();
    }

    ~VulkanGraphicsPipeline()
    {
        DestroyPipeline();
        DestroyVertexShaderModule();
        DestroyFragmentShaderModule();
        DestroyPipelineCache();
    }

    // Destroy Graphics Pipeline
    void DestroyPipeline()
    {
        if (m_pipeline) {
            VulkanDeviceContext::GetThe()->DestroyPipeline(VulkanDeviceContext::GetThe()->getDevice(), m_pipeline, nullptr);
            m_pipeline = VkPipeline(0);
        }
    }

    void DestroyPipelineCache()
    {
        if (m_cache) {
            VulkanDeviceContext::GetThe()->DestroyPipelineCache(VulkanDeviceContext::GetThe()->getDevice(), m_cache, nullptr);
            m_cache = VkPipelineCache(0);
        }
    }

    void DestroyVertexShaderModule()
    {
        if (m_vertexShaderCache) {
            VulkanDeviceContext::GetThe()->DestroyShaderModule(VulkanDeviceContext::GetThe()->getDevice(), m_vertexShaderCache, nullptr);
            m_vertexShaderCache = VkShaderModule();
        }
    }

    void DestroyFragmentShaderModule()
    {
        if (m_fragmentShaderCache) {
            VulkanDeviceContext::GetThe()->DestroyShaderModule(VulkanDeviceContext::GetThe()->getDevice(), m_fragmentShaderCache, nullptr);
            m_fragmentShaderCache = VkShaderModule();
        }
    }

    // Create Graphics Pipeline
    VkResult CreatePipeline(VkViewport *pViewport, VkRect2D *pScissor,
	                        VkRenderPass renderPass, VulkanDescriptorSetLayout *pBufferDescriptorSets);

    VkPipeline getPipeline() {
        return m_pipeline;
    }

private:
    
    VkPipelineCache            m_cache;
    VkPipeline                 m_pipeline;
    VulkanShaderCompiler       m_vulkanShaderCompiler;
    std::stringstream          m_fssCache;
    VkShaderModule             m_vertexShaderCache;
    VkShaderModule             m_fragmentShaderCache;
};

class VulkanPerDrawContext {

public:
    VulkanPerDrawContext()
    : contextIndex(-1),
      frameBuffer(),
      syncPrimitives(),
      samplerYcbcrConversion(),
      descriptorSetLayoutBinding(),
      commandBuffer(),
      gfxPipeline(),
      lastVideoFormatUpdate((uint32_t)-1)
    {
    }

    VulkanPerDrawContext(VulkanPerDrawContext&& other)
        : contextIndex(other.contextIndex),
          frameBuffer(std::move(other.frameBuffer)),
          syncPrimitives(std::move(other.syncPrimitives)),
          samplerYcbcrConversion(std::move(other.samplerYcbcrConversion)),
          descriptorSetLayoutBinding(std::move(other.descriptorSetLayoutBinding)),
          commandBuffer(std::move(other.commandBuffer)),
          gfxPipeline(std::move(other.gfxPipeline)),
          lastVideoFormatUpdate(other.lastVideoFormatUpdate)
    {
        // Set the moved-from object's members to a valid but indeterminate state
        other.contextIndex = -1;
        other.lastVideoFormatUpdate = static_cast<uint32_t>(-1);
    }

    ~VulkanPerDrawContext() {
    }

    bool IsFormatOutOfDate(uint32_t formatUpdateCounter) {
        if (formatUpdateCounter != lastVideoFormatUpdate) {
            lastVideoFormatUpdate = formatUpdateCounter;
            return true;
        }
        return false;
    }

    VkResult RecordCommandBuffer(VkCommandBuffer cmdBuffer,
                                 VkRenderPass renderPass,
                                 const ImageResourceInfo* inputImageToDrawFrom,
                                 int32_t displayWidth, int32_t displayHeight,
                                 VkImage displayImage, VkFramebuffer framebuffer, VkRect2D* pRenderArea,
                                 VkPipeline pipeline,
                                 const VulkanDescriptorSetLayout& descriptorSetLayoutBinding,
                                 const VulkanSamplerYcbcrConversion& samplerYcbcrConversion,
                                 const VulkanVertexBuffer& vertexBuffer);

    
    int32_t contextIndex;
    VulkanFrameBuffer frameBuffer;
    VulkanSyncPrimitives syncPrimitives;
    VulkanSamplerYcbcrConversion samplerYcbcrConversion;
    VulkanDescriptorSetLayout descriptorSetLayoutBinding;
    VulkanCommandBuffersSet commandBuffer;
    VulkanGraphicsPipeline gfxPipeline;
    uint32_t lastVideoFormatUpdate;
};

class VulkanRenderInfo {

public:

    VulkanRenderInfo()
      : mVerbose()
        {}


    // Create per draw contexts.
    VkResult CreatePerDrawContexts(
            VkSwapchainKHR swapchain, const VkExtent2D* pFbExtent2D,
            VkViewport* pViewport, VkRect2D* pScissor, const VkSurfaceFormatKHR* pSurfaceFormat,
            VkRenderPass renderPass, const VkSamplerCreateInfo* pSamplerCreateInfo = nullptr,
            const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo = nullptr);

    VkResult UpdatePerDrawContexts(VulkanPerDrawContext* pPerDrawContext,
            VkViewport* pViewport, VkRect2D* pScissor, VkRenderPass renderPass,
            const VkSamplerCreateInfo* pSamplerCreateInfo = nullptr,
            const VkSamplerYcbcrConversionCreateInfo* pSamplerYcbcrConversionCreateInfo = nullptr);

    uint32_t GetNumDrawContexts() const {
        return (uint32_t)perDrawCtx.size();
    }

    VulkanPerDrawContext* GetDrawContext(int32_t scIndx) {
        return ((size_t)scIndx < perDrawCtx.size()) ? &perDrawCtx[scIndx] : nullptr;
    }

    ~VulkanRenderInfo() {

        perDrawCtx.clear();
    }

private:
    uint32_t mVerbose : 1;
    std::vector<VulkanPerDrawContext> perDrawCtx;

};

class VkVideoAppCtx {

public:
    bool                       m_initialized;
    // WindowInfo window;
    bool                       m_useTestImage;
    ImageObject                m_testFrameImage;
    VulkanRenderPass           m_renderPass;
    VulkanVertexBuffer         m_vertexBuffer;
    VulkanRenderInfo           m_renderInfo;

    ~VkVideoAppCtx()
    {
        if (!m_initialized) {
            return;
        }

        m_initialized = false;
    }

    VkVideoAppCtx(bool testVk)
        : m_initialized(false),
          // window(),
          m_useTestImage(testVk),
          // swapchain_(),
          m_renderPass(),
          m_vertexBuffer(),
          m_renderInfo()
    {
        CreateSamplerYcbcrConversions();
    }

    VkResult CreateSamplerYcbcrConversions();

    void ContextIsReady() {
        m_initialized = true;
    }

    bool IsContextReady() const {
        return m_initialized;
    }

};

// A helper functions
// A helper function to map required memory property into a VK memory type
// memory type is an index into the array of 32 entries; or the bit index
// for the memory type ( each BIT of an 32 bit integer is a type ).
VkResult AllocateMemoryTypeFromProperties(
        uint32_t typeBits,
        VkFlags requirements_mask,
        uint32_t *typeIndex);

void setImageLayout(
                    VkCommandBuffer cmdBuffer, VkImage image,
                    VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                    VkPipelineStageFlags srcStages,
                    VkPipelineStageFlags destStages, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

} // End of namespace vulkanVideoUtils

#endif // __VULKANVIDEOUTILS__
