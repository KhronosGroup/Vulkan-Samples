/*
* Copyright 2023 NVIDIA Corporation.
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

#ifndef _VULKANDESCRIPTORSETLAYOUT_H_
#define _VULKANDESCRIPTORSETLAYOUT_H_

#include <sstream>
#include <vulkan_interfaces.h>
#include "VkCodecUtils/Helpers.h"
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkCodecUtils/VkBufferResource.h"
#include "VkCodecUtils/VulkanSamplerYcbcrConversion.h"

class VulkanDescriptorSet {

public:

    VulkanDescriptorSet()
        : m_descPool(), m_descSet() {}

    ~VulkanDescriptorSet()
    {
        DestroyDescriptorSets();
        DestroyDescriptorPool();
    }

    void DestroyDescriptorSets()
    {
        if (m_descSet) {
            VulkanDeviceContext::GetThe()->FreeDescriptorSets(VulkanDeviceContext::GetThe()->getDevice(), m_descPool, 1, &m_descSet);
            m_descSet = VkDescriptorSet(0);
        }
    }

    void DestroyDescriptorPool()
    {
        if (m_descPool) {
            VulkanDeviceContext::GetThe()->DestroyDescriptorPool(VulkanDeviceContext::GetThe()->getDevice(), m_descPool, nullptr);
            m_descPool = VkDescriptorPool(0);
        }
    }

    VkResult CreateDescriptorPool(uint32_t descriptorCount,
                                  VkDescriptorType descriptorType)
    {

        DestroyDescriptorPool();

        VkDescriptorPoolSize descriptorPoolSize = VkDescriptorPoolSize();
        descriptorPoolSize.type = descriptorType;
        descriptorPoolSize.descriptorCount = descriptorCount;

        VkDescriptorPoolCreateInfo descriptor_pool = VkDescriptorPoolCreateInfo();
        descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool.pNext = nullptr;
        descriptor_pool.maxSets = 1;
        descriptor_pool.poolSizeCount = 1;
        descriptor_pool.pPoolSizes = &descriptorPoolSize;
        return VulkanDeviceContext::GetThe()->CreateDescriptorPool(VulkanDeviceContext::GetThe()->getDevice(), &descriptor_pool, nullptr, &m_descPool);
    }

    VkResult AllocateDescriptorSets(uint32_t descriptorCount,
                                    VkDescriptorSetLayout* dscLayout)
    {
        DestroyDescriptorSets();

        VkDescriptorSetAllocateInfo alloc_info = VkDescriptorSetAllocateInfo();
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = m_descPool;
        alloc_info.descriptorSetCount = descriptorCount;
        alloc_info.pSetLayouts = dscLayout;
        VkResult result = VulkanDeviceContext::GetThe()->AllocateDescriptorSets(VulkanDeviceContext::GetThe()->getDevice(), &alloc_info, &m_descSet);

        return result;
    }

    const VkDescriptorSet* GetDescriptorSet() const {
        return &m_descSet;
    }

private:
    VkDescriptorPool m_descPool;
    VkDescriptorSet  m_descSet;
};

class VulkanDescriptorSetLayoutInfo {

public:

    VulkanDescriptorSetLayoutInfo()
    : m_descriptorSetLayoutBindings(),
      m_descriptorSetLayoutCreateInfo()
    {

    }

    VulkanDescriptorSetLayoutInfo(const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings,
                                  VkDescriptorSetLayoutCreateFlags flags)
    : m_descriptorSetLayoutBindings(setLayoutBindings),
      m_descriptorSetLayoutCreateInfo()
    {
        UpdateLayout(flags);
    }

    size_t SetDescriptorSetLayoutInfo(const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings,
                                      VkDescriptorSetLayoutCreateFlags flags)
    {
        m_descriptorSetLayoutBindings = setLayoutBindings;
        return UpdateLayout(flags);
    }

    const VkDescriptorSetLayoutCreateInfo* GetCreateInfo() const {
        return &m_descriptorSetLayoutCreateInfo;
    }

    VkDescriptorSetLayoutCreateFlags GetDescriptorLayoutMode() const {
        // VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR or
        // VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT
        return m_descriptorSetLayoutCreateInfo.flags;
    }

private:
    size_t UpdateLayout(VkDescriptorSetLayoutCreateFlags flags) {

        m_immutableSamplers.clear();
        // Make a local copy of the immutable samplers
        for (auto& descriptorSetLayoutBinding : m_descriptorSetLayoutBindings) {
            if(descriptorSetLayoutBinding.pImmutableSamplers) {
                const VkSampler immutableSampler = *descriptorSetLayoutBinding.pImmutableSamplers;
                m_immutableSamplers.emplace_back(immutableSampler);
                descriptorSetLayoutBinding.pImmutableSamplers = &m_immutableSamplers.back();
            }
        }

        m_descriptorSetLayoutCreateInfo = VkDescriptorSetLayoutCreateInfo();
        m_descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        m_descriptorSetLayoutCreateInfo.flags = flags;
        m_descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(m_descriptorSetLayoutBindings.size());
        m_descriptorSetLayoutCreateInfo.pBindings = m_descriptorSetLayoutBindings.data();

        return m_descriptorSetLayoutBindings.size();
    }
private:
    std::vector<VkDescriptorSetLayoutBinding> m_descriptorSetLayoutBindings;
    std::vector<VkSampler>                    m_immutableSamplers;
    VkDescriptorSetLayoutCreateInfo           m_descriptorSetLayoutCreateInfo;
};

class VulkanDescriptorSetLayout {

    enum { MAX_DESCRIPTOR_SET_POOLS = 4 };

public:
    VulkanDescriptorSetLayout()
     : m_descriptorSetLayoutInfo()
     , m_dscLayout()
     , m_pipelineLayout()
     , m_currentDescriptorSetPools(-1)
     , m_descSets{}
     , m_maxNumFrames(0)
     , m_descriptorLayoutSize(0)
     , m_descriptorBufferSize(0)
     , m_descriptorBufferProperties()
    { }

    ~VulkanDescriptorSetLayout()
    {
        DestroyPipelineLayout();
        DestroyDescriptorSetLayout();
    }

    void DestroyPipelineLayout()
    {
        if (m_pipelineLayout) {
            VulkanDeviceContext::GetThe()->DestroyPipelineLayout(VulkanDeviceContext::GetThe()->getDevice(), m_pipelineLayout, nullptr);
            m_pipelineLayout = VkPipelineLayout(0);
        }
    }

    void DestroyDescriptorSetLayout()
    {
        m_resourceDescriptorBuffer = nullptr;

        if (m_dscLayout) {
            VulkanDeviceContext::GetThe()->DestroyDescriptorSetLayout(VulkanDeviceContext::GetThe()->getDevice(), m_dscLayout, nullptr);
            m_dscLayout = VkDescriptorSetLayout(0);
        }
    }

    // initialize descriptor set
    VkResult CreateDescriptorSet(const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings,
                                 VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags = 0,
                                 uint32_t                   pushConstantRangeCount = 0,
                                 const VkPushConstantRange* pPushConstantRanges = nullptr,
                                 const VulkanSamplerYcbcrConversion* pSamplerYcbcrConversion = nullptr,
                                 uint32_t maxNumFrames = 1,
                                 bool autoSelectdescriptorSetLayoutCreateFlags = true);

    // initialize descriptor set
    VkResult CreateDescriptorSet(const VulkanDeviceContext* vkDevCtx, VkDescriptorPool allocPool, VkDescriptorSetLayout* dscLayouts, uint32_t descriptorSetCount = 1);

    VkResult WriteDescriptorSet(uint32_t descriptorWriteCount,
                                const VkWriteDescriptorSet* pDescriptorWrites);

    VkResult CreateFragmentShaderOutput(VkDescriptorType outMode, uint32_t outSet, uint32_t outBinding, uint32_t outArrayIndex, std::stringstream& imageFss);

    VkResult CreateFragmentShaderLayouts(const uint32_t* setIds, uint32_t numSets, std::stringstream& texFss);


    const VkDescriptorSet* GetDescriptorSet() const {
        if (m_currentDescriptorSetPools < 0) {
            return nullptr;
        }
        return m_descSets[m_currentDescriptorSetPools].GetDescriptorSet();
    }


    bool UsesDescriptorBuffer() const {
        return m_resourceDescriptorBuffer;
    }

    VkDeviceOrHostAddressConstKHR UpdateDescriptorBuffer(uint32_t bufferIdx, uint32_t set,
                                                         uint32_t descriptorWriteCount,
                                                         const VkWriteDescriptorSet* pDescriptorWrites) const {

        if (!(bufferIdx < m_maxNumFrames)) {
            assert(!"bufferIdx is out of range!");
            return VkDeviceOrHostAddressConstKHR();
        }

        const VkDeviceSize bufferIdxOffset = (bufferIdx * m_descriptorLayoutSize);
        assert(bufferIdxOffset < m_descriptorBufferSize);

        // Set descriptors for image
        VkDeviceSize dstBufferOffset = 0;
        VkDeviceSize maxSize = 0;
        uint8_t* imageDescriptorBufPtr = const_cast<uint8_t*>((const uint8_t*)m_resourceDescriptorBuffer->GetDataPtr(dstBufferOffset, maxSize));
        assert(imageDescriptorBufPtr);
        imageDescriptorBufPtr += bufferIdxOffset;

        for (uint32_t i = 0; i < descriptorWriteCount; i++) {
            const VkWriteDescriptorSet* pDescriptorWrite = &pDescriptorWrites[i];

            VkDeviceSize dstBindingOffset = 0;
            VulkanDeviceContext::GetThe()->GetDescriptorSetLayoutBindingOffsetEXT(VulkanDeviceContext::GetThe()->getDevice(), m_dscLayout,
                                                               pDescriptorWrite->dstBinding,
                                                               &dstBindingOffset);

            size_t descriptorSize = m_descriptorBufferProperties.storageImageDescriptorSize;
            switch (pDescriptorWrite->descriptorType) {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                descriptorSize = m_descriptorBufferProperties.samplerDescriptorSize;
                // fall through
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                descriptorSize = m_descriptorBufferProperties.combinedImageSamplerDescriptorSize;
                // fall through
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                descriptorSize = m_descriptorBufferProperties.sampledImageDescriptorSize;
                // fall through
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            {
                assert((dstBindingOffset + descriptorSize) < m_descriptorLayoutSize);

                VkDescriptorGetInfoEXT descriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
                descriptorInfo.type = pDescriptorWrite->descriptorType;
                descriptorInfo.data.pCombinedImageSampler = pDescriptorWrite->pImageInfo;

                VulkanDeviceContext::GetThe()->GetDescriptorEXT(VulkanDeviceContext::GetThe()->getDevice(), &descriptorInfo, descriptorSize,
                                             imageDescriptorBufPtr + dstBindingOffset);
            }
            break;
            default:
                assert(!"Unknown descriptor type");
            }
        }
        VkBuffer imageDescriptorBuffer = m_resourceDescriptorBuffer->GetBuffer();
        assert(imageDescriptorBuffer);
        VkDeviceOrHostAddressConstKHR imageDescriptorBufferDeviceAddress;
        VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
        bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bufferDeviceAddressInfo.buffer = imageDescriptorBuffer;
        imageDescriptorBufferDeviceAddress.deviceAddress = VulkanDeviceContext::GetThe()->GetBufferDeviceAddressKHR(VulkanDeviceContext::GetThe()->getDevice(), &bufferDeviceAddressInfo);
        imageDescriptorBufferDeviceAddress.deviceAddress += bufferIdxOffset;

        return imageDescriptorBufferDeviceAddress;
    }


    VulkanDescriptorSet* GetNextDescriptorSet() {
        m_currentDescriptorSetPools++;
        m_currentDescriptorSetPools = m_currentDescriptorSetPools % MAX_DESCRIPTOR_SET_POOLS;
        assert((m_currentDescriptorSetPools >= 0) && (m_currentDescriptorSetPools < MAX_DESCRIPTOR_SET_POOLS));
        return &m_descSets[m_currentDescriptorSetPools];
    }

    const VkDescriptorSetLayout* GetDescriptorSetLayout() const {
        return &m_dscLayout;
    }

    VkPipelineLayout GetPipelineLayout() const {
        return m_pipelineLayout;
    }

    const VulkanDescriptorSetLayoutInfo& GetDescriptorSetLayoutInfo() const {
        return m_descriptorSetLayoutInfo;
    }

private:
    VulkanDescriptorSetLayoutInfo      m_descriptorSetLayoutInfo;
    VkDescriptorSetLayout              m_dscLayout;
    VkPipelineLayout                   m_pipelineLayout;
    int32_t                            m_currentDescriptorSetPools;
    VulkanDescriptorSet                m_descSets[MAX_DESCRIPTOR_SET_POOLS];
    // If VK_EXT_descriptor_buffer is supported:
    uint32_t                           m_maxNumFrames;
    VkDeviceSize                       m_descriptorLayoutSize;
    VkDeviceSize                       m_descriptorBufferSize;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT
                                       m_descriptorBufferProperties;
    VkSharedBaseObj<VkBufferResource>  m_resourceDescriptorBuffer;
};

#endif /* _VULKANDESCRIPTORSETLAYOUT_H_ */
