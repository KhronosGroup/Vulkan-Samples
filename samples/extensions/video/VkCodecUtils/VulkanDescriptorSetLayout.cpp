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

#include "VulkanDescriptorSetLayout.h"
#include "VkCodecUtils/Helpers.h" // for alignedSize

VkResult VulkanDescriptorSetLayout::CreateFragmentShaderLayouts(const uint32_t* setIds, uint32_t numSets, std::stringstream& imageFss)
{
    const VkDescriptorSetLayoutCreateInfo* pDescriptorSetEntries = m_descriptorSetLayoutInfo.GetCreateInfo();

    imageFss <<
           "#version 450 core\n"
           "layout(location = 0) in vec2 vTexCoord;\n"
           "layout(location = 0) out vec4 oFrag;\n";

    for (uint32_t setIndex = 0; setIndex < numSets; setIndex++) {
        const VkDescriptorSetLayoutCreateInfo * pDescriptorSetEntry = &pDescriptorSetEntries[setIndex];
        uint32_t setId = setIds[setIndex];

        for (uint32_t bindingIndex = 0; bindingIndex < pDescriptorSetEntry->bindingCount; bindingIndex++) {

            const VkDescriptorSetLayoutBinding* pBinding = &pDescriptorSetEntry->pBindings[bindingIndex];

            switch (pBinding->descriptorType) {
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                imageFss <<
                       "layout(set = " << setId << ", binding = " << pBinding->binding << ") uniform sampler2D tex" <<
                       setId << pBinding->binding << "[" << pBinding->descriptorCount << "];\n";
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                imageFss <<
                       "layout(std140, set = " << setId << ", binding = " << pBinding->binding << ") uniform ubodef" <<
                       setId << pBinding->binding << " { vec4 color; } ubo" <<
                       setId << pBinding->binding << "[" << pBinding->descriptorCount << "];\n";
                break;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                imageFss <<
                       "layout(std140, set = " << setId << ", binding = " << pBinding->binding << ") buffer ssbodef" <<
                       setId << pBinding->binding << " { vec4 color; } ssbo" <<
                       setId << pBinding->binding << "[" << pBinding->descriptorCount << "];\n";
                break;
            default:
                assert(0);
                break;
            }
        }
    }
    // printf("\nFragment shader layout code:\n %s", imageFss.str().c_str());

    return VK_SUCCESS;
}

// initialize descriptor set
VkResult VulkanDescriptorSetLayout::CreateDescriptorSet(const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings,
                                                        VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags,
                                                        uint32_t                   pushConstantRangeCount,
                                                        const VkPushConstantRange* pPushConstantRanges,
                                                        const VulkanSamplerYcbcrConversion* pSamplerYcbcrConversion,
                                                        uint32_t maxNumFrames,
                                                        bool autoSelectdescriptorSetLayoutCreateFlags)
{
    DestroyPipelineLayout();
    DestroyDescriptorSetLayout();

    m_maxNumFrames = maxNumFrames;

    if (autoSelectdescriptorSetLayoutCreateFlags) {
        if (VulkanDeviceContext::GetThe()->FindRequiredDeviceExtension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)) {
            descriptorSetLayoutCreateFlags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        } else if (VulkanDeviceContext::GetThe()->FindRequiredDeviceExtension(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME)){
            descriptorSetLayoutCreateFlags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
        } else {
            descriptorSetLayoutCreateFlags = 0;
        }
    }
    m_descriptorSetLayoutInfo.SetDescriptorSetLayoutInfo(descriptorSetLayoutBindings, descriptorSetLayoutCreateFlags);

    VkResult result = VulkanDeviceContext::GetThe()->CreateDescriptorSetLayout(VulkanDeviceContext::GetThe()->getDevice(),
                                                            m_descriptorSetLayoutInfo.GetCreateInfo(),
                                                            nullptr,
                                                            &m_dscLayout);

    if (result != VK_SUCCESS) {
        return result;
    }

    if (m_descriptorSetLayoutInfo.GetDescriptorLayoutMode() == VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR) {

        VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProps{};
        VkPhysicalDeviceProperties2KHR deviceProps2{};
        pushDescriptorProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
        deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        deviceProps2.pNext = &pushDescriptorProps;
        VulkanDeviceContext::GetThe()->GetPhysicalDeviceProperties2(VulkanDeviceContext::GetThe()->getPhysicalDevice(), &deviceProps2);

    } else if (m_descriptorSetLayoutInfo.GetDescriptorLayoutMode() == VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT) {

        VkPhysicalDeviceProperties2KHR deviceProps2{};
        m_descriptorBufferProperties = VkPhysicalDeviceDescriptorBufferPropertiesEXT();
        m_descriptorBufferProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
        deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        deviceProps2.pNext = &m_descriptorBufferProperties;
        VulkanDeviceContext::GetThe()->GetPhysicalDeviceProperties2(VulkanDeviceContext::GetThe()->getPhysicalDevice(), &deviceProps2);

        VulkanDeviceContext::GetThe()->GetDescriptorSetLayoutSizeEXT(VulkanDeviceContext::GetThe()->getDevice(), m_dscLayout, &m_descriptorLayoutSize);

        m_descriptorLayoutSize = vk::alignedSize(m_descriptorLayoutSize,
                                                 m_descriptorBufferProperties.descriptorBufferOffsetAlignment);

        m_descriptorBufferSize = m_descriptorLayoutSize * m_maxNumFrames;

        result = VkBufferResource::Create(
                                          VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                                             VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
                                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          m_descriptorBufferSize,
                                          m_resourceDescriptorBuffer, 1,
                                          m_descriptorBufferProperties.descriptorBufferOffsetAlignment);

        if (result != VK_SUCCESS) {
            return result;
        }
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = VkPipelineLayoutCreateInfo();
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_dscLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRangeCount;
    pipelineLayoutCreateInfo.pPushConstantRanges = pPushConstantRanges;

    result = VulkanDeviceContext::GetThe()->CreatePipelineLayout(VulkanDeviceContext::GetThe()->getDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        return result;
    }

    if (m_descriptorSetLayoutInfo.GetDescriptorLayoutMode()) {
        // Don't need the descriptor pool and static descriptor sets
        return result;
    }

    uint32_t descriptorCount = (uint32_t)descriptorSetLayoutBindings.size();
    uint32_t maxCombinedImageSamplerDescriptorCount = 1;
    if (pSamplerYcbcrConversion) {
        maxCombinedImageSamplerDescriptorCount = pSamplerYcbcrConversion->GetCombinedImageSamplerDescriptorCount();
    }

    VulkanDescriptorSet* pDescriptorSet = GetNextDescriptorSet();
    result = pDescriptorSet->CreateDescriptorPool(descriptorCount * maxCombinedImageSamplerDescriptorCount,
                                                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    if (result != VK_SUCCESS) {
        return result;
    }

    return pDescriptorSet->AllocateDescriptorSets(descriptorCount, &m_dscLayout);
}

VkResult VulkanDescriptorSetLayout::WriteDescriptorSet(uint32_t descriptorWriteCount,
                                                       const VkWriteDescriptorSet* pDescriptorWrites)
{
    assert(m_descriptorSetLayoutInfo.GetDescriptorLayoutMode() == 0);

    VulkanDeviceContext::GetThe()->UpdateDescriptorSets(VulkanDeviceContext::GetThe()->getDevice(), descriptorWriteCount, pDescriptorWrites, 0, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanDescriptorSetLayout::CreateFragmentShaderOutput(VkDescriptorType outMode, uint32_t outSet, uint32_t outBinding, uint32_t outArrayIndex, std::stringstream& imageFss)
{
    switch (outMode) {
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        imageFss <<
               "void main()\n"
               "{\n"
               "    oFrag = texture(tex" << outSet << outBinding << "[" << outArrayIndex << "], vTexCoord);\n"
               "}\n";
        break;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        imageFss <<
               "void main()\n"
               "{\n"
               "    oFrag = ubo" << outSet << outBinding << "[" << outArrayIndex << "].color;\n"
               "}\n";
        break;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        imageFss <<
               "void main()\n"
               "{\n"
               "    oFrag = ssbo" << outSet << outBinding << "[" << outArrayIndex << "].color;\n"
               "}\n";
        break;
    default:
        assert(0);
        break;
    }

    // printf("\nFragment shader output code:\n %s", imageFss.str().c_str());

    return VK_SUCCESS;
}
