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

#ifndef _VULKANCOMPUTEPIPELINE_H_
#define _VULKANCOMPUTEPIPELINE_H_

#include <vulkan_interfaces.h>
#include "VkCodecUtils/VulkanDeviceContext.h"
#include "VkCodecUtils/VulkanDescriptorSetLayout.h"
#include "VkCodecUtils/VulkanShaderCompiler.h"

class VulkanComputePipeline
{
public:
    VulkanComputePipeline()
     : m_pipelineCache(),
       m_pipeline(),
       m_shaderModule(0)
    { }

    VulkanComputePipeline(VulkanComputePipeline&& other)
        : m_pipelineCache(other.m_pipelineCache),
          m_pipeline(other.m_pipeline),
          m_shaderModule(other.m_shaderModule)
    {
        // Reset the other object's resources
        other.m_pipelineCache = VkPipelineCache(0);
        other.m_pipeline = VkPipeline(0);
        other.m_shaderModule = VkShaderModule();
    }

    ~VulkanComputePipeline()
    {
        DestroyPipeline();
        DestroyShaderModule();
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
        if (m_pipelineCache) {
            VulkanDeviceContext::GetThe()->DestroyPipelineCache(VulkanDeviceContext::GetThe()->getDevice(), m_pipelineCache, nullptr);
            m_pipelineCache = VkPipelineCache(0);
        }
    }

    void DestroyShaderModule()
    {
        if (m_shaderModule) {
            VulkanDeviceContext::GetThe()->DestroyShaderModule(VulkanDeviceContext::GetThe()->getDevice(), m_shaderModule, nullptr);
            m_shaderModule = VkShaderModule();
        }
    }


    // Create Compute Pipeline
    VkResult CreatePipeline(VulkanShaderCompiler& shaderCompiler,
                            const char* shaderCode, size_t shaderSize,
                            const char* pEntryName,
                            uint32_t workgroupSizeX, // usually 16
                            uint32_t workgroupSizeY, // usually 16
                            const VulkanDescriptorSetLayout* pBufferDescriptorSets);

    VkPipeline getPipeline() {
        return m_pipeline;
    }

private:
    VkPipelineCache            m_pipelineCache;
    VkPipeline                 m_pipeline;
    VkShaderModule             m_shaderModule;
};

#endif /* _VULKANCOMPUTEPIPELINE_H_ */
